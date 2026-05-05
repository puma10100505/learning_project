/*
 * Render/DepthRaster.cpp
 * ----------------------
 * 软件 Z-Buffer 实现（透视校正深度）。
 *
 * 多线程模型：
 *   - DrawTriangleWorld / DrawLineWorld 只做世界空间剔除 + clip 变换，把结果
 *     压入 deferred 列表（无锁，单线程提交）。
 *   - End() 时按屏幕高度切成 N 个水平 band，开 N 个 std::thread 并行光栅；
 *     每个像素只属于一个 band → 无竞争、无原子。
 *   - 同一 band 内严格按提交顺序处理三角，半透明 blend 行为与单线程一致。
 */

#include "DepthRaster.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <thread>
#include <vector>

namespace DepthRaster
{
namespace
{
struct DeferredTri
{
    float c0[4];
    float c1[4];
    float c2[4];
    ImU32 col;
};

struct Active
{
    bool              on = false;
    int               rw = 0, rh = 0;
    Mat4              vp{};
    float             fovy = 0.0f;
    Vec3              eye{};
    float             worldDiag = 1.0f;
    std::vector<float>      depth;
    std::vector<uint32_t>   rgba;     // packed RGBA8 per pixel (R in low byte 顺序)
    std::vector<uint8_t>    upload;   // linear RGBA for GL
    std::vector<DeferredTri> tris;    // 收集到的 clip-space 三角，End() 时并行光栅
};

Active g_Active;

inline float Orient2D(float ax, float ay, float bx, float by, float cx, float cy)
{
    return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
}

inline void UnpackCol(ImU32 col, float& r, float& g, float& b, float& a)
{
    r = static_cast<float>((col >> IM_COL32_R_SHIFT) & 0xFF) * (1.0f / 255.0f);
    g = static_cast<float>((col >> IM_COL32_G_SHIFT) & 0xFF) * (1.0f / 255.0f);
    b = static_cast<float>((col >> IM_COL32_B_SHIFT) & 0xFF) * (1.0f / 255.0f);
    a = static_cast<float>((col >> IM_COL32_A_SHIFT) & 0xFF) * (1.0f / 255.0f);
}

inline ImU32 PackRGBAf(float r, float g, float b, float a)
{
    const int R = static_cast<int>(std::lround(std::clamp(r, 0.0f, 1.0f) * 255.0f));
    const int G = static_cast<int>(std::lround(std::clamp(g, 0.0f, 1.0f) * 255.0f));
    const int B = static_cast<int>(std::lround(std::clamp(b, 0.0f, 1.0f) * 255.0f));
    const int A = static_cast<int>(std::lround(std::clamp(a, 0.0f, 1.0f) * 255.0f));
    return IM_COL32(R, G, B, A);
}

/// 仅写本 band [yLo, yHi) 内的像素（band 之间无重叠 → 无锁安全）
inline void PutPixelBand(int x, int y, float z01, ImU32 col, int yLo, int yHi)
{
    if (y < yLo || y >= yHi) return;
    if (x < 0 || x >= g_Active.rw) return;
    const int    idx  = y * g_Active.rw + x;
    constexpr float zEps = 1e-7f;
    if (z01 >= g_Active.depth[idx] - zEps) return;

    float sr, sg, sb, sa;
    UnpackCol(col, sr, sg, sb, sa);
    if (sa >= 0.999f)
    {
        g_Active.depth[idx] = z01;
        g_Active.rgba[idx]  = col;
        return;
    }
    float dr, dg, db, da;
    UnpackCol(g_Active.rgba[idx], dr, dg, db, da);
    const float outA = sa + da * (1.0f - sa);
    if (outA < 1e-4f) return;
    const float t  = sa / outA;
    const float nr = sr * t + dr * (1.0f - t);
    const float ng = sg * t + dg * (1.0f - t);
    const float nb = sb * t + db * (1.0f - t);
    g_Active.depth[idx] = z01;
    g_Active.rgba[idx]  = PackRGBAf(nr, ng, nb, outA);
}

/// 光栅一个三角到 [yLo, yHi) 行带内的像素
void RasterizeTriBand(const DeferredTri& T, int yLo, int yHi)
{
    const float* c0 = T.c0;
    const float* c1 = T.c1;
    const float* c2 = T.c2;
    if (c0[3] <= 1e-4f || c1[3] <= 1e-4f || c2[3] <= 1e-4f) return;

    const float rw = static_cast<float>(g_Active.rw);
    const float rh = static_cast<float>(g_Active.rh);

    const float sx0 = (c0[0] / c0[3] * 0.5f + 0.5f) * rw;
    const float sy0 = (1.0f - (c0[1] / c0[3] * 0.5f + 0.5f)) * rh;
    const float sx1 = (c1[0] / c1[3] * 0.5f + 0.5f) * rw;
    const float sy1 = (1.0f - (c1[1] / c1[3] * 0.5f + 0.5f)) * rh;
    const float sx2 = (c2[0] / c2[3] * 0.5f + 0.5f) * rw;
    const float sy2 = (1.0f - (c2[1] / c2[3] * 0.5f + 0.5f)) * rh;

    const float denom = Orient2D(sx0, sy0, sx1, sy1, sx2, sy2);
    if (std::fabs(denom) < 1e-8f) return;

    int xmin = static_cast<int>(std::floor(std::min({ sx0, sx1, sx2 })));
    int ymin = static_cast<int>(std::floor(std::min({ sy0, sy1, sy2 })));
    int xmax = static_cast<int>(std::ceil(std::max({ sx0, sx1, sx2 })));
    int ymax = static_cast<int>(std::ceil(std::max({ sy0, sy1, sy2 })));
    xmin = std::max(0, xmin);
    xmax = std::min(g_Active.rw - 1, xmax);
    // 把 y 范围裁到当前 band（其余像素本线程不负责）
    ymin = std::max(ymin, yLo);
    ymax = std::min(ymax, yHi - 1);
    if (ymin > ymax || xmin > xmax) return;

    constexpr float margin = -1e-4f;
    for (int y = ymin; y <= ymax; ++y)
    {
        for (int x = xmin; x <= xmax; ++x)
        {
            const float px = static_cast<float>(x) + 0.5f;
            const float py = static_cast<float>(y) + 0.5f;
            const float l0 = Orient2D(px, py, sx1, sy1, sx2, sy2) / denom;
            const float l1 = Orient2D(px, py, sx2, sy2, sx0, sy0) / denom;
            const float l2 = Orient2D(px, py, sx0, sy0, sx1, sy1) / denom;
            if (l0 < margin || l1 < margin || l2 < margin) continue;

            const float zcl = l0 * c0[2] + l1 * c1[2] + l2 * c2[2];
            const float wl  = l0 * c0[3] + l1 * c1[3] + l2 * c2[3];
            if (wl <= 1e-6f) continue;
            const float z_ndc = zcl / wl;
            const float z01   = z_ndc * 0.5f + 0.5f;
            if (z01 < 0.0f || z01 > 1.0f) continue;

            PutPixelBand(x, y, z01, T.col, yLo, yHi);
        }
    }
}

/// 选择多线程 worker 数：受硬件并发与帧高度限制，上限 8 防过度调度
int PickWorkerCount(int rh)
{
    int n = static_cast<int>(std::thread::hardware_concurrency());
    if (n <= 0) n = 2;
    n = std::min(n, 8);
    // 每个 band 至少 32 行，避免极小窗口下线程开销大于收益
    n = std::min(n, std::max(1, rh / 32));
    return std::max(1, n);
}

} // namespace

bool IsActive() { return g_Active.on; }

void Begin(int rw, int rh, ImU32 clearColor, const Mat4& vp, float fovyRad,
           const Vec3& eyeWorld, float worldBoundsDiagonal)
{
    rw = std::max(1, rw);
    rh = std::max(1, rh);
    constexpr int kMaxDim = 2048;
    rw = std::min(rw, kMaxDim);
    rh = std::min(rh, kMaxDim);

    g_Active.on        = true;
    g_Active.rw        = rw;
    g_Active.rh        = rh;
    g_Active.vp        = vp;
    g_Active.fovy      = fovyRad;
    g_Active.eye       = eyeWorld;
    g_Active.worldDiag = std::max(worldBoundsDiagonal, 1.0f);

    const size_t n = static_cast<size_t>(rw) * static_cast<size_t>(rh);
    g_Active.depth.assign(n, 1.0f);
    g_Active.rgba.assign(n, clearColor);
    g_Active.tris.clear();
}

void End()
{
    if (!g_Active.on) return;

    // ---- 并行光栅：按屏幕高度切 band，每线程负责自己 [yLo, yHi) 行段 ----
    if (!g_Active.tris.empty())
    {
        const int rh    = g_Active.rh;
        const int N     = PickWorkerCount(rh);
        if (N <= 1)
        {
            // 单线程 fallback（极小窗口/单核机器）
            for (const auto& T : g_Active.tris)
                RasterizeTriBand(T, 0, rh);
        }
        else
        {
            std::vector<std::thread> workers;
            workers.reserve(N);
            for (int i = 0; i < N; ++i)
            {
                const int yLo = (i       * rh) / N;
                const int yHi = ((i + 1) * rh) / N;
                workers.emplace_back([yLo, yHi]
                {
                    for (const auto& T : g_Active.tris)
                        RasterizeTriBand(T, yLo, yHi);
                });
            }
            for (auto& w : workers) w.join();
        }
    }

    // ---- 打包成线性 RGBA8 供 GL 上传 ----
    const size_t n = static_cast<size_t>(g_Active.rw) * static_cast<size_t>(g_Active.rh);
    g_Active.upload.resize(n * 4);
    // 行序与光栅/Project 一致（首行=画面上方），与 ImGui 屏幕坐标一致。
    // ImGui::Image 默认 uv (0,0)-(1,1) 即可正确显示，不需要任何翻转。
    for (size_t i = 0; i < n; ++i)
    {
        const ImU32 c = g_Active.rgba[i];
        g_Active.upload[i * 4 + 0] = static_cast<uint8_t>((c >> IM_COL32_R_SHIFT) & 0xFF);
        g_Active.upload[i * 4 + 1] = static_cast<uint8_t>((c >> IM_COL32_G_SHIFT) & 0xFF);
        g_Active.upload[i * 4 + 2] = static_cast<uint8_t>((c >> IM_COL32_B_SHIFT) & 0xFF);
        g_Active.upload[i * 4 + 3] = static_cast<uint8_t>((c >> IM_COL32_A_SHIFT) & 0xFF);
    }

    g_Active.tris.clear();
    g_Active.on = false;
}

int BufferWidth()  { return g_Active.rw; }
int BufferHeight() { return g_Active.rh; }

const uint8_t* PixelsRGBA()
{
    return g_Active.upload.empty() ? nullptr : g_Active.upload.data();
}

void DrawTriangleWorld(const Mat4& vp, Vec3 a, Vec3 b, Vec3 c, ImU32 col, bool cullBackface)
{
    if (!g_Active.on) return;
    if (cullBackface)
    {
        const Vec3  e1 = b - a;
        const Vec3  e2 = c - a;
        Vec3        n  = Cross(e1, e2);
        const float nl = Length(n);
        if (nl < 1e-8f) return;
        n                 = n * (1.0f / nl);
        const Vec3 ctr    = (a + b + c) * (1.0f / 3.0f);
        const Vec3 toEye  = Normalize(g_Active.eye - ctr);
        if (Dot(n, toEye) <= 0.0f) return;
    }
    DeferredTri T;
    T.col = col;
    TransformPoint(vp, a, T.c0);
    TransformPoint(vp, b, T.c1);
    TransformPoint(vp, c, T.c2);
    g_Active.tris.push_back(T);
}

void DrawLineWorld(const Mat4& vp, Vec3 a, Vec3 b, ImU32 col, float thicknessPixels)
{
    if (!g_Active.on) return;
    const Vec3  ab  = b - a;
    const float len = Length(ab);
    if (len < 1e-6f) return;

    // 每端各按自身到相机的距离算 halfWidth → 梯形，屏幕上是均匀 1 像素宽。
    // 旧实现取 max(distA,distB) 让 quad 整体加粗：长线段（如大半径世界网格）
    // 因为远端 dist 大，近端会变得非常粗。
    const float distA = std::max(Length(a - g_Active.eye), 0.1f);
    const float distB = std::max(Length(b - g_Active.eye), 0.1f);
    const float halfH = static_cast<float>(g_Active.rh);
    const float k     =
        (2.0f * std::tan(g_Active.fovy * 0.5f)) / std::max(halfH, 1.0f);
    const float floorW   = g_Active.worldDiag * 1e-4f;
    const float halfWidA = std::max(thicknessPixels * k * 0.5f * distA, floorW);
    const float halfWidB = std::max(thicknessPixels * k * 0.5f * distB, floorW);

    const Vec3      eab   = ab * (1.0f / len);
    const Vec3      mid   = (a + b) * 0.5f;
    Vec3            toEye = g_Active.eye - mid;
    if (Length(toEye) < 1e-4f) toEye = V3(0, 1, 0);
    toEye = Normalize(toEye);
    Vec3 side = Cross(eab, toEye);
    if (Length(side) < 1e-4f) side = Cross(eab, V3(0, 1, 0));
    if (Length(side) < 1e-4f) return;
    side = Normalize(side);

    const Vec3 sA = side * halfWidA;
    const Vec3 sB = side * halfWidB;
    const Vec3 p0 = a + sA;
    const Vec3 p1 = a - sA;
    const Vec3 p2 = b - sB;
    const Vec3 p3 = b + sB;

    DrawTriangleWorld(vp, p0, p1, p2, col, false);
    DrawTriangleWorld(vp, p0, p2, p3, col, false);
}

} // namespace DepthRaster
