/*
 * Render/Renderer3D.cpp
 * ---------------------
 * 3D 轨道视角软件渲染实现。
 * 详细说明见 Renderer3D.h。
 */

#include "Renderer3D.h"
#include "CollisionRender.h"
#include "DepthRaster.h"
#include "GpuRenderer.h"
#include "NavViewTextureBridge.h"
#include "PainterSort.h"
#include "Primitives.h"

#include <cmath>
#include <cstring>
#include <algorithm>
#include <vector>

// Recast
#include "Recast.h"

// Detour
#include "DetourNavMesh.h"
#include "DetourStatus.h"

namespace Renderer3D
{
namespace
{
constexpr int kGroundOcclusionSubdivDepth = 2;

// =============================================================================
// 世界坐标轴 Widget（左下角小窗口）
//   - 跟随相机旋转：使用相机 view 矩阵的旋转分量，把世界 +X/+Y/+Z 投到屏幕；
//   - 不受平移与缩放影响：常驻面板左下角；
//   - 三轴用 R/G/B + 末端圆点 + 字母 X/Y/Z 标识；
//   - 按当前朝向远近排序绘制：靠近相机的轴覆盖在远端轴之上。
// =============================================================================
void DrawWorldAxisGizmo(ImDrawList* dl, ImVec2 panelMin, ImVec2 panelSize,
                        const OrbitCamera& cam)
{
    constexpr float kRadius      = 32.0f;     ///< 三轴长度（像素）
    constexpr float kPadding     = 14.0f;     ///< 距离面板左下的内边距
    constexpr float kLineThick   = 2.5f;
    constexpr float kCapRadius   = 5.0f;
    constexpr float kBgRadius    = kRadius + 14.0f;

    if (panelSize.x < kBgRadius * 2.5f || panelSize.y < kBgRadius * 2.5f) return;

    const ImVec2 center = ImVec2(panelMin.x + kBgRadius + kPadding,
                                 panelMin.y + panelSize.y - kBgRadius - kPadding);

    // 相机视图矩阵的旋转分量（与 DrawCanvas3D 完全同一来源，保证 widget 与场景同步）。
    // MakeLookAtRH 的 m[0..2][0..2] 行向量 = 视空间基的世界坐标分量。
    // 把世界基(1,0,0)/(0,1,0)/(0,0,1)用 view 旋转后取 (x,y) 即可投影到屏幕：
    //   screen.x =  view_x（向右为正）；screen.y = -view_y（屏幕 y 向下为正）
    const Vec3 eye  = cam.Eye;
    const Mat4 view = MakeLookAtRH(eye, cam.Target, V3(0, 1, 0));

    struct Axis
    {
        Vec3   world;
        ImU32  col;
        const char* lbl;
    };
    const Axis axes[3] = {
        { V3(1, 0, 0), IM_COL32(232,  72,  72, 255), "X" },
        { V3(0, 1, 0), IM_COL32( 88, 204,  92, 255), "Y" },
        { V3(0, 0, 1), IM_COL32( 76, 144, 255, 255), "Z" },
    };

    struct Proj { ImVec2 end; ImU32 col; const char* lbl; float depth; };
    Proj projs[3];
    for (int i = 0; i < 3; ++i)
    {
        // 只用旋转：用 view 的 3x3 上左部分对方向向量（w=0）变换。
        const Vec3& w = axes[i].world;
        const float vx =  view.m[0][0] * w.x + view.m[0][1] * w.y + view.m[0][2] * w.z;
        const float vy =  view.m[1][0] * w.x + view.m[1][1] * w.y + view.m[1][2] * w.z;
        const float vz =  view.m[2][0] * w.x + view.m[2][1] * w.y + view.m[2][2] * w.z;
        projs[i].end   = ImVec2(center.x + vx * kRadius, center.y - vy * kRadius);
        projs[i].col   = axes[i].col;
        projs[i].lbl   = axes[i].lbl;
        // RH 视空间中 -z 朝相机：vz 越小（更负）越靠近相机，越大越远；
        // 用 vz 作为远近 key，远的先画。
        projs[i].depth = vz;
    }

    int order[3] = { 0, 1, 2 };
    std::sort(order, order + 3, [&](int a, int b) { return projs[a].depth > projs[b].depth; });

    // 背板：半透明圆 + 边框，提升可读性
    dl->AddCircleFilled(center, kBgRadius, IM_COL32(20, 22, 28, 170), 32);
    dl->AddCircle      (center, kBgRadius, IM_COL32(120, 120, 130, 200), 32, 1.0f);

    // 三轴：先线、再末端圆点、最后字母（外缘描黑提升对比度）
    for (int k = 0; k < 3; ++k)
    {
        const Proj& p = projs[order[k]];
        dl->AddLine    (center, p.end, p.col, kLineThick);
        dl->AddCircleFilled(p.end, kCapRadius,    p.col, 16);
        dl->AddCircle      (p.end, kCapRadius + 1, IM_COL32(0, 0, 0, 200), 16, 1.0f);

        const ImVec2 ts = ImGui::CalcTextSize(p.lbl);
        const ImVec2 tp = ImVec2(p.end.x - ts.x * 0.5f, p.end.y - ts.y * 0.5f);
        dl->AddText(ImVec2(tp.x + 1, tp.y + 1), IM_COL32(0, 0, 0, 200), p.lbl);
        dl->AddText(tp,                          IM_COL32(255, 255, 255, 255), p.lbl);
    }
}


/// 对障碍做视线遮挡时，在三角内多点采样 + 细分，减轻障碍投影边界处的锯齿
void DrawGroundTriWithOcclusion(ImDrawList* dl, const Mat4& vp, ImVec2 panelMin, ImVec2 panelSize,
                                const Vec3& eye, const std::vector<Obstacle>* obstacles, ImU32 col,
                                const Vec3& a, const Vec3& b, const Vec3& c, int depth)
{
    const bool cullObs = obstacles && !obstacles->empty();
    if (!cullObs)
    {
        Render3D::TriFilled3D(dl, vp, panelMin, panelSize, a, b, c, col);
        return;
    }
    const std::vector<Obstacle>& obs = *obstacles;
    const bool                   oa  = CollisionRender::SegmentEyeToPointOccluded(eye, a, obs);
    const bool                   ob  = CollisionRender::SegmentEyeToPointOccluded(eye, b, obs);
    const bool                   oc  = CollisionRender::SegmentEyeToPointOccluded(eye, c, obs);
    const Vec3                   g   = (a + b + c) * (1.0f / 3.0f);
    const bool                   og  = CollisionRender::SegmentEyeToPointOccluded(eye, g, obs);
    if (oa && ob && oc && og)
        return;
    if (!oa && !ob && !oc && !og)
    {
        Render3D::TriFilled3D(dl, vp, panelMin, panelSize, a, b, c, col);
        return;
    }
    if (depth >= kGroundOcclusionSubdivDepth)
    {
        Render3D::TriFilled3D(dl, vp, panelMin, panelSize, a, b, c, col);
        return;
    }
    const Vec3 mab = (a + b) * 0.5f;
    const Vec3 mbc = (b + c) * 0.5f;
    const Vec3 mca = (c + a) * 0.5f;
    DrawGroundTriWithOcclusion(dl, vp, panelMin, panelSize, eye, obstacles, col, a, mab, mca, depth + 1);
    DrawGroundTriWithOcclusion(dl, vp, panelMin, panelSize, eye, obstacles, col, b, mbc, mab, depth + 1);
    DrawGroundTriWithOcclusion(dl, vp, panelMin, panelSize, eye, obstacles, col, c, mca, mbc, depth + 1);
    DrawGroundTriWithOcclusion(dl, vp, panelMin, panelSize, eye, obstacles, col, mab, mbc, mca, depth + 1);
}

// =============================================================================
// 世界 XZ 平面网格
//   - 经过 Render3D::Line3D 走当前 3D 渲染后端，参与真实深度测试（CpuZ / GpuShader / PainterSort 都正确遮挡）；
//   - 网格中心吸附到 majorStep 的整数倍，避免相机平移时网格"飘"；
//   - 每 1 单位画一条次级线，每 10 单位画一条主线，X 轴（z=0）红色，Z 轴（x=0）蓝色；
//   - halfExt 由调用方决定（自适应：clamp(worldDiag*0.6, 25, 100)；手动：UI 滑块）。
// =============================================================================
void DrawWorldGridXZ(ImDrawList* dl, const Mat4& vp, ImVec2 panelMin, ImVec2 panelSize,
                     const Vec3& sceneCenter, float halfExt, float thicknessScale)
{
    const float minorStep = 1.0f;
    const float majorStep = 10.0f;

    const float cx = std::round(sceneCenter.x / majorStep) * majorStep;
    const float cz = std::round(sceneCenter.z / majorStep) * majorStep;
    const float yPlane = 0.0f;

    const ImU32 colMinor = IM_COL32( 70,  76,  88, 200);
    const ImU32 colMajor = IM_COL32(120, 128, 142, 220);
    const ImU32 colXAxis = IM_COL32(220,  92,  92, 240);  // 沿 X 方向的轴线（z=0）
    const ImU32 colZAxis = IM_COL32( 92, 144, 230, 240);  // 沿 Z 方向的轴线（x=0）
    const float ts         = std::max(thicknessScale, 0.1f);
    const float thickMinor = 1.0f * ts;
    const float thickMajor = 1.5f * ts;
    const float thickAxis  = 2.0f * ts;

    const int nCells = static_cast<int>(halfExt / minorStep);

    auto isMajorCoord = [majorStep](float v) {
        const float r = std::fmod(std::fabs(v) + 1e-3f, majorStep);
        return r < 0.02f || r > majorStep - 0.02f;
    };

    // 沿 X 方向的网格线（一条线对应一个 z 值）
    for (int i = -nCells; i <= nCells; ++i)
    {
        const float z       = cz + i * minorStep;
        const bool  isAxis  = std::fabs(z) < 0.01f;
        const bool  isMajor = !isAxis && isMajorCoord(z);
        const ImU32 col     = isAxis ? colXAxis : (isMajor ? colMajor : colMinor);
        const float thick   = isAxis ? thickAxis : (isMajor ? thickMajor : thickMinor);
        Render3D::Line3D(dl, vp, panelMin, panelSize,
                         V3(cx - halfExt, yPlane, z),
                         V3(cx + halfExt, yPlane, z), col, thick);
    }
    // 沿 Z 方向的网格线（一条线对应一个 x 值）
    for (int i = -nCells; i <= nCells; ++i)
    {
        const float x       = cx + i * minorStep;
        const bool  isAxis  = std::fabs(x) < 0.01f;
        const bool  isMajor = !isAxis && isMajorCoord(x);
        const ImU32 col     = isAxis ? colZAxis : (isMajor ? colMajor : colMinor);
        const float thick   = isAxis ? thickAxis : (isMajor ? thickMajor : thickMinor);
        Render3D::Line3D(dl, vp, panelMin, panelSize,
                         V3(x, yPlane, cz - halfExt),
                         V3(x, yPlane, cz + halfExt), col, thick);
    }
}
} // namespace

Map3D DrawCanvas3D(ImDrawList* dl, ImVec2 panelMin, ImVec2 panelSize,
                   const OrbitCamera& cam, const Draw3DParams& p)
{
    Map3D result{};   // bValid = false

    const ImVec2 panelMax(panelMin.x + panelSize.x, panelMin.y + panelSize.y);
    dl->PushClipRect(panelMin, panelMax, true);

    // -------------------------------------------------------------------------
    // 摄像机变换
    // -------------------------------------------------------------------------
    const Vec3  eye    = cam.Eye;
    const float aspect = panelSize.x / std::max(1.0f, panelSize.y);
    const Mat4  view   = MakeLookAtRH(eye, cam.Target, V3(0, 1, 0));
    const Mat4  proj   = MakePerspectiveRH(cam.Fovy, aspect, 0.1f, 5000.0f);
    const Mat4  vp     = MatMul(proj, view);

    // 写入 Map3D 缓存
    result.bValid       = true;
    result.ViewportMin  = panelMin;
    result.ViewportSize = panelSize;
    result.EyeWorld     = eye;
    result.View         = view;
    result.Proj         = proj;
    // InvVP 不在此计算（原始代码注释：拣取用 eye + 反向射线即可）

    float worldDiag = 100.0f;
    if (p.Geom)
    {
        const float* bmn = p.Geom->Bounds + 0;
        const float* bmx = p.Geom->Bounds + 3;
        const float  dx  = bmx[0] - bmn[0];
        const float  dy  = bmx[1] - bmn[1];
        const float  dz  = bmx[2] - bmn[2];
        worldDiag        = std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    const View3DDepthMode depthMode =
        p.View ? p.View->DepthMode3D : View3DDepthMode::None;
    const bool            useCpuZ =
        (depthMode == View3DDepthMode::CpuZBuffer) && NavViewTextureBridgeAvailable();
    const bool            useGpu =
        (depthMode == View3DDepthMode::GpuShader) && GpuRenderer::Available();
    const bool            usePainter = (depthMode == View3DDepthMode::PainterSort);
    // 软件光栅成本是 O(像素)：可降采样渲染再由 GL 线性放大显示。
    //   N=1 原生（最清晰，最慢）；N=2 半分辨率（默认）；N=3/4 更快但更糊。
    // GPU 模式无此瓶颈，恒走原生分辨率。
    const int kDownscale = useCpuZ
        ? std::clamp(p.View ? p.View->CpuZBufferDownscale : 2, 1, 8)
        : 1;
    const int             rw         = std::max(1, static_cast<int>(panelSize.x) / kDownscale);
    const int             rh         = std::max(1, static_cast<int>(panelSize.y) / kDownscale);

    if (useCpuZ)
        DepthRaster::Begin(rw, rh, IM_COL32(28, 30, 36, 255), vp, cam.Fovy, eye, worldDiag);
    else if (useGpu)
    {
        const int gpuSamples = std::clamp(p.View ? p.View->GpuMsaaSamples : 4, 1, 16);

        // ---- 把 UI 端 LightSettings 折成 GpuRenderer::LightParams ----
        // Directional：vec = 用户填的方向（shader 内 normalize）
        // Point      ：vec = normalize(dir) * Distance（=世界坐标位置；同时其模长充当衰减半径）
        GpuRenderer::LightParams lp{};
        if (p.View)
        {
            const LightSettings& ls = p.View->Light;
            lp.type      = (ls.Type == 1) ? 1 : 0;
            Vec3 dir     = V3(ls.DirX, ls.DirY, ls.DirZ);
            const float dl = Length(dir);
            if (dl < 1e-6f) dir = V3(0.0f, 1.0f, 0.0f);
            else            dir = dir * (1.0f / dl);
            lp.vec       = (lp.type == 1) ? (dir * std::max(ls.Distance, 0.01f)) : dir;
            lp.color     = V3(ls.Color[0], ls.Color[1], ls.Color[2]);
            lp.intensity = std::max(ls.Intensity, 0.0f);
            lp.ambient   = std::clamp(ls.Ambient, 0.0f, 1.0f);

            // ---- Shadow 配置 ----
            lp.shadowsOn      = ls.bShadowsOn;
            lp.shadowMapSize  = std::clamp(ls.ShadowMapSize, 64, 4096);
            lp.shadowStrength = std::clamp(ls.ShadowStrength, 0.0f, 1.0f);
            lp.shadowBias     = std::max(ls.ShadowBias, 0.0f);
            lp.shadowPcf      = (ls.ShadowPcf != 0) ? 1 : 0;

            // 场景中心 / 半径：取包围盒的 (center, half-diagonal)，用于 ortho frustum
            if (p.Geom)
            {
                const float* bmn = p.Geom->Bounds + 0;
                const float* bmx = p.Geom->Bounds + 3;
                lp.sceneCenter = V3((bmn[0] + bmx[0]) * 0.5f,
                                    (bmn[1] + bmx[1]) * 0.5f,
                                    (bmn[2] + bmx[2]) * 0.5f);
                const float dx = bmx[0] - bmn[0];
                const float dy = bmx[1] - bmn[1];
                const float dz = bmx[2] - bmn[2];
                lp.sceneRadius = 0.5f * std::sqrt(dx * dx + dy * dy + dz * dz);
            }
            else
            {
                lp.sceneCenter = V3(0, 0, 0);
                lp.sceneRadius = std::max(worldDiag * 0.5f, 1.0f);
            }
        }

        GpuRenderer::Begin(static_cast<int>(panelSize.x), static_cast<int>(panelSize.y),
                           IM_COL32(28, 30, 36, 255), vp, cam.Fovy, eye, worldDiag,
                           gpuSamples, lp);
    }
    else
    {
        dl->AddRectFilled(panelMin, panelMax, IM_COL32(28, 30, 36, 255));
        if (usePainter)
            PainterSort::Begin(eye, vp, panelMin, panelSize);
    }

    // ---------------------------------------------------------------------
    // 世界 XZ 平面网格：在 Begin() 之后绘制以参与深度测试，被几何正确遮挡。
    // 即使没有加载几何也显示，便于初始视角参考；与 2D 视图共用 bShowGrid 开关。
    // ---------------------------------------------------------------------
    if (!p.View || p.View->bShowGrid)
    {
        Vec3  gridCenter = V3(0, 0, 0);
        if (p.Geom)
        {
            const float* bmn = p.Geom->Bounds + 0;
            const float* bmx = p.Geom->Bounds + 3;
            gridCenter = V3((bmn[0] + bmx[0]) * 0.5f, 0.0f, (bmn[2] + bmx[2]) * 0.5f);
        }
        // halfExt：自适应取 [25,100] 内的 0.6×对角线；手动则使用 UI 设置（下限 1m 防越界）。
        float halfExt;
        if (p.View && !p.View->bWorldGridAuto)
            halfExt = std::max(1.0f, p.View->WorldGridHalfExt);
        else
            halfExt = std::clamp(worldDiag * 0.6f, 25.0f, 100.0f);
        const float gridThick = p.View ? p.View->WorldGridLineThickness : 1.0f;
        DrawWorldGridXZ(dl, vp, panelMin, panelSize, gridCenter, halfExt, gridThick);
    }

    if (!p.Geom)
    {
        if (useCpuZ)
        {
            DepthRaster::End();
            const uint8_t* px = DepthRaster::PixelsRGBA();
            if (px)
            {
                NavViewTextureBridgeUpload(DepthRaster::BufferWidth(),
                                           DepthRaster::BufferHeight(), px);
                const ImTextureID tid = NavViewTextureBridgeTextureId();
                if (tid)
                    dl->AddImage(tid, panelMin, panelMax, ImVec2(0, 0), ImVec2(1, 1));
            }
        }
        else if (useGpu)
        {
            GpuRenderer::End();
            const ImTextureID tid = GpuRenderer::TextureId();
            if (tid)
            {
#if defined(_WIN32)
                // D3D12：纹理已是“v=0 在顶部”的 D3D 约定，直接 (0,0)-(1,1) 显示
                dl->AddImage(tid, panelMin, panelMax, ImVec2(0, 0), ImVec2(1, 1));
#else
                // OpenGL FBO：v=0 在底部，需要翻转
                dl->AddImage(tid, panelMin, panelMax, ImVec2(0, 1), ImVec2(1, 0));
#endif
            }
        }
        else if (usePainter)
            PainterSort::Flush(dl);
        dl->AddRect(panelMin, panelMax, IM_COL32(90, 90, 100, 255));
        dl->PopClipRect();
        return result;
    }

    const float* bmin = p.Geom->Bounds + 0;
    const float* bmax = p.Geom->Bounds + 3;

    // -------------------------------------------------------------------------
    // 碰撞地面：输入三角网填充（与 PhysX 地形一致）；障碍与相机之间者剔除
    // -------------------------------------------------------------------------
    if (p.View && p.View->bShowCollisionTint && !p.Geom->Triangles.empty())
    {
        const ImU32 colBase = Renderer2D::ColU32(p.View->GroundCollisionTint);
        // 仅遍历"纯地面"三角；剩下的障碍实体网格三角交给下面的 obstacle pass
        // 用 ObstacleTopTint / ObstacleSideTint 单独着色，避免颜色混淆。
        const int   totalTris  = static_cast<int>(p.Geom->Triangles.size() / 3);
        const int   triCount   = (p.Geom->GroundTriCount >= 0)
                                 ? std::min(p.Geom->GroundTriCount, totalTris)
                                 : totalTris;
        const int   stride     = (triCount > 40000) ? (triCount / 40000 + 1) : 1;
        // CpuZBuffer / GpuShader 都有真深度测试：地面被障碍遮挡的部分会被自然剔除，
        // 再做 CPU 射线 vs 障碍 + 三角细分纯属重复，性能损失非常大。
        const std::vector<Obstacle>* obsPtr =
            (useCpuZ || useGpu || p.Geom->Obstacles.empty()) ? nullptr : &p.Geom->Obstacles;
        for (int t = 0; t < triCount; t += stride)
        {
            const int*   tri = &p.Geom->Triangles[t * 3];
            const float* va  = &p.Geom->Vertices[tri[0] * 3];
            const float* vb  = &p.Geom->Vertices[tri[1] * 3];
            const float* vc  = &p.Geom->Vertices[tri[2] * 3];
            const Vec3 a = V3(va[0], va[1], va[2]);
            const Vec3 b = V3(vb[0], vb[1], vb[2]);
            const Vec3 c = V3(vc[0], vc[1], vc[2]);
            if (obsPtr)
                DrawGroundTriWithOcclusion(dl, vp, panelMin, panelSize, eye, obsPtr, colBase, a, b, c, 0);
            else
                Render3D::TriFilled3D(dl, vp, panelMin, panelSize, a, b, c, colBase);
        }
    }

    // 注：早先这里有一段按场景包围盒等分 12 段的简易网格；现已迁移到 Begin() 之后的
    // DrawWorldGridXZ()——按世界单位画，且无几何时也可见，更接近 Blender / Maya 风格。

    // -------------------------------------------------------------------------
    // 输入几何线框（仅 OBJ 模式；大模型稀疏采样）
    // -------------------------------------------------------------------------
    if (p.View && p.View->bShowInputMesh
        && p.Geom->Source == GeomSource::ObjFile
        && !p.Geom->Triangles.empty())
    {
        const ImU32 inputCol = IM_COL32(180, 200, 220, 110);
        const int triCount = static_cast<int>(p.Geom->Triangles.size() / 3);
        const int stride   = (triCount > 40000) ? (triCount / 40000 + 1) : 1;
        for (int t = 0; t < triCount; t += stride)
        {
            const int*   tri = &p.Geom->Triangles[t * 3];
            const float* va  = &p.Geom->Vertices[tri[0] * 3];
            const float* vb  = &p.Geom->Vertices[tri[1] * 3];
            const float* vc  = &p.Geom->Vertices[tri[2] * 3];
            const Vec3 a = V3(va[0], va[1], va[2]);
            const Vec3 b = V3(vb[0], vb[1], vb[2]);
            const Vec3 c = V3(vc[0], vc[1], vc[2]);
            Render3D::Line3D(dl, vp, panelMin, panelSize, a, b, inputCol);
            Render3D::Line3D(dl, vp, panelMin, panelSize, b, c, inputCol);
            Render3D::Line3D(dl, vp, panelMin, panelSize, c, a, inputCol);
        }
    }

    // -------------------------------------------------------------------------
    // 障碍（Box / Cylinder，高度独立）
    // -------------------------------------------------------------------------
    if (p.View && p.View->bShowObstacles)
    {
        auto shadedQuad = [&](const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& v3,
                              const Vec3& nOut, ImU32 topCol, ImU32 sideFrontCol, ImU32 sideBackCol,
                              bool useTopColor)
        {
            const Vec3 ctr = (v0 + v1 + v2 + v3) * 0.25f;
            // 无深度缓冲时剔除背向面；有 Z-Buffer 时两面都画，由深度决定可见性
            if (!useTopColor && !DepthRaster::IsActive() && !GpuRenderer::IsActive()
                && Dot(nOut, eye - ctr) <= 1e-5f)
                return;
            const ImU32 col = useTopColor ? topCol
                                          : (Dot(nOut, eye - ctr) > 0.0f ? sideFrontCol : sideBackCol);
            Render3D::TriFilled3D(dl, vp, panelMin, panelSize, v0, v1, v2, col);
            Render3D::TriFilled3D(dl, vp, panelMin, panelSize, v0, v2, v3, col);
        };

        for (size_t i = 0; i < p.Geom->Obstacles.size(); ++i)
        {
            const Obstacle& o = p.Geom->Obstacles[i];
            const bool      selected = (p.SelectedObstacle == (int)i);
            const bool      dragging = (p.MoveStateIndex == (int)i);
            ImU32           fill{};
            ImU32           edge{};
            ImU32           topU{};
            ImU32           sideFU{};
            ImU32           sideBU{};
            if (p.View->bShowCollisionTint)
            {
                ImVec4 topC = p.View->ObstacleTopTint;
                ImVec4 sf   = p.View->ObstacleSideTint;
                ImVec4 sb   = p.View->ObstacleSideBackTint;
                if (dragging)
                {
                    topC.w = std::min(1.0f, topC.w * 1.12f + 0.06f);
                    sf.w   = std::min(1.0f, sf.w * 1.10f + 0.04f);
                    sb.w   = std::min(1.0f, sb.w * 1.08f + 0.03f);
                }
                else if (selected)
                {
                    topC.w = std::min(1.0f, topC.w * 1.06f + 0.04f);
                    sf.w   = std::min(1.0f, sf.w * 1.05f + 0.02f);
                }
                topU   = Renderer2D::ColU32(topC);
                sideFU = Renderer2D::ColU32(sf);
                sideBU = Renderer2D::ColU32(sb);
                edge   = selected ? IM_COL32(255, 235, 120, 255)
                                  : Renderer2D::ColU32(p.View->ObstacleCollisionEdge);
            }
            else
            {
                fill = IM_COL32(220, 105, 95, dragging ? 235 : (selected ? 215 : 175));
                edge = selected ? IM_COL32(255, 235, 120, 255) : IM_COL32(255, 160, 140, 245);
            }
            const float H = o.Height;

            if (o.Shape == ObstacleShape::Box)
            {
                const float minX = o.CX - o.SX, maxX = o.CX + o.SX;
                const float minZ = o.CZ - o.SZ, maxZ = o.CZ + o.SZ;
                Vec3        c000{ minX, 0, minZ }, c100{ maxX, 0, minZ };
                Vec3        c110{ maxX, 0, maxZ }, c010{ minX, 0, maxZ };
                Vec3        c001{ minX, H, minZ }, c101{ maxX, H, minZ };
                Vec3        c111{ maxX, H, maxZ }, c011{ minX, H, maxZ };

                if (p.View->bShowCollisionTint)
                {
                    shadedQuad(c000, c010, c011, c001, V3(-1, 0, 0), topU, sideFU, sideBU, false); // -X
                    shadedQuad(c100, c101, c111, c110, V3(1, 0, 0), topU, sideFU, sideBU, false); // +X
                    shadedQuad(c000, c001, c101, c100, V3(0, 0, -1), topU, sideFU, sideBU, false); // -Z
                    shadedQuad(c010, c110, c111, c011, V3(0, 0, 1), topU, sideFU, sideBU, false);  // +Z
                    shadedQuad(c000, c100, c110, c010, V3(0, -1, 0), topU, sideFU, sideBU, false); // bottom
                    shadedQuad(c001, c011, c111, c101, V3(0, 1, 0), topU, sideFU, sideBU, true);  // top
                }
                else
                {
                    Render3D::TriFilled3D(dl, vp, panelMin, panelSize, c001, c101, c111, fill);
                    Render3D::TriFilled3D(dl, vp, panelMin, panelSize, c001, c111, c011, fill);
                }
                Render3D::Line3D(dl, vp, panelMin, panelSize, c000, c100, edge);
                Render3D::Line3D(dl, vp, panelMin, panelSize, c100, c110, edge);
                Render3D::Line3D(dl, vp, panelMin, panelSize, c110, c010, edge);
                Render3D::Line3D(dl, vp, panelMin, panelSize, c010, c000, edge);
                Render3D::Line3D(dl, vp, panelMin, panelSize, c001, c101, edge);
                Render3D::Line3D(dl, vp, panelMin, panelSize, c101, c111, edge);
                Render3D::Line3D(dl, vp, panelMin, panelSize, c111, c011, edge);
                Render3D::Line3D(dl, vp, panelMin, panelSize, c011, c001, edge);
                Render3D::Line3D(dl, vp, panelMin, panelSize, c000, c001, edge);
                Render3D::Line3D(dl, vp, panelMin, panelSize, c100, c101, edge);
                Render3D::Line3D(dl, vp, panelMin, panelSize, c110, c111, edge);
                Render3D::Line3D(dl, vp, panelMin, panelSize, c010, c011, edge);
            }
            else
            {
                if (p.View->bShowCollisionTint)
                {
                    Render3D::DrawCylinderObstacleShaded3D(dl, vp, panelMin, panelSize, eye,
                                                           o.CX, o.CZ, o.Radius, H,
                                                           topU, sideFU, sideBU, edge);
                }
                else
                {
                    Render3D::DrawCylinderObstacle3D(dl, vp, panelMin, panelSize,
                                                     o.CX, o.CZ, o.Radius, H, fill, edge);
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // Build Volume（自定义生成区域）— 橙色线框盒（12 条棱线）
    // -------------------------------------------------------------------------
    if (p.BV && p.BV->bActive && p.View && p.View->bShowBuildVolume)
    {
        const ImU32 bvCol = IM_COL32(255, 160, 40, 230);
        const float* mn = p.BV->Min;
        const float* mx = p.BV->Max;
        // 8 个顶点
        Vec3 v000 = V3(mn[0], mn[1], mn[2]), v100 = V3(mx[0], mn[1], mn[2]);
        Vec3 v110 = V3(mx[0], mn[1], mx[2]), v010 = V3(mn[0], mn[1], mx[2]);
        Vec3 v001 = V3(mn[0], mx[1], mn[2]), v101 = V3(mx[0], mx[1], mn[2]);
        Vec3 v111 = V3(mx[0], mx[1], mx[2]), v011 = V3(mn[0], mx[1], mx[2]);
        // 底面
        Render3D::Line3D(dl, vp, panelMin, panelSize, v000, v100, bvCol);
        Render3D::Line3D(dl, vp, panelMin, panelSize, v100, v110, bvCol);
        Render3D::Line3D(dl, vp, panelMin, panelSize, v110, v010, bvCol);
        Render3D::Line3D(dl, vp, panelMin, panelSize, v010, v000, bvCol);
        // 顶面
        Render3D::Line3D(dl, vp, panelMin, panelSize, v001, v101, bvCol);
        Render3D::Line3D(dl, vp, panelMin, panelSize, v101, v111, bvCol);
        Render3D::Line3D(dl, vp, panelMin, panelSize, v111, v011, bvCol);
        Render3D::Line3D(dl, vp, panelMin, panelSize, v011, v001, bvCol);
        // 四条竖棱
        Render3D::Line3D(dl, vp, panelMin, panelSize, v000, v001, bvCol);
        Render3D::Line3D(dl, vp, panelMin, panelSize, v100, v101, bvCol);
        Render3D::Line3D(dl, vp, panelMin, panelSize, v110, v111, bvCol);
        Render3D::Line3D(dl, vp, panelMin, panelSize, v010, v011, bvCol);
    }

    // -------------------------------------------------------------------------
    // PolyMesh（微抬以避免 z-fighting）
    // -------------------------------------------------------------------------
    const float polyY = 0.05f;
    if (p.Nav && p.Nav->bBuilt && p.Nav->PolyMesh
        && p.View && (p.View->bFillPolygons || p.View->bShowPolyEdges))
    {
        const rcPolyMesh* pm  = p.Nav->PolyMesh;
        const int         nvp = pm->nvp;
        const float       cs  = pm->cs;
        const float       ch  = pm->ch;
        for (int i = 0; i < pm->npolys; ++i)
        {
            const unsigned short* poly = &pm->polys[i * nvp * 2];
            std::vector<Vec3> pts;
            pts.reserve(nvp);
            for (int j = 0; j < nvp; ++j)
            {
                if (poly[j] == RC_MESH_NULL_IDX) break;
                const unsigned short* v = &pm->verts[poly[j] * 3];
                const float wx = pm->bmin[0] + v[0] * cs;
                const float wy = pm->bmin[1] + v[1] * ch + polyY;
                const float wz = pm->bmin[2] + v[2] * cs;
                pts.push_back(V3(wx, wy, wz));
            }
            if (pts.size() < 3) continue;
            if (p.View->bFillPolygons)
            {
                const ImU32 col = p.View->bRegionColors
                    ? Renderer2D::HashColor(pm->regs ? pm->regs[i] : (unsigned)i, 0.55f)
                    : IM_COL32(80, 200, 100, 130);
                for (size_t k = 1; k + 1 < pts.size(); ++k)
                    Render3D::TriFilled3D(dl, vp, panelMin, panelSize, pts[0], pts[k], pts[k + 1], col);
            }
            if (p.View->bShowPolyEdges)
            {
                for (size_t k = 0; k < pts.size(); ++k)
                    Render3D::Line3D(dl, vp, panelMin, panelSize,
                                     pts[k], pts[(k + 1) % pts.size()],
                                     IM_COL32(80, 200, 100, 230));
            }
        }
    }

    // -------------------------------------------------------------------------
    // Detail Mesh
    // -------------------------------------------------------------------------
    if (p.Nav && p.Nav->bBuilt && p.Nav->PolyMeshDetail
        && p.View && p.View->bShowDetailMesh)
    {
        const rcPolyMeshDetail* dm = p.Nav->PolyMeshDetail;
        for (int m = 0; m < dm->nmeshes; ++m)
        {
            const unsigned int vertBase = dm->meshes[m * 4 + 0];
            const unsigned int triBase  = dm->meshes[m * 4 + 2];
            const unsigned int triCount = dm->meshes[m * 4 + 3];
            for (unsigned int i = 0; i < triCount; ++i)
            {
                const unsigned char* t = &dm->tris[(triBase + i) * 4];
                Vec3 v3[3];
                for (int k = 0; k < 3; ++k)
                {
                    const float* v = &dm->verts[(vertBase + t[k]) * 3];
                    v3[k] = V3(v[0], v[1] + polyY * 0.5f, v[2]);
                }
                Render3D::Line3D(dl, vp, panelMin, panelSize, v3[0], v3[1], IM_COL32(255, 255, 255, 70));
                Render3D::Line3D(dl, vp, panelMin, panelSize, v3[1], v3[2], IM_COL32(255, 255, 255, 70));
                Render3D::Line3D(dl, vp, panelMin, panelSize, v3[2], v3[0], IM_COL32(255, 255, 255, 70));
            }
        }
    }

    // -------------------------------------------------------------------------
    // 路径走廊 + 直线路径
    // -------------------------------------------------------------------------
    const ImU32 pathColU32     = Renderer2D::ColU32(p.PathColor);
    const ImU32 corridorColU32 = Renderer2D::ColU32(p.CorridorColor);

    if (p.Nav && p.Nav->bBuilt && p.Nav->NavMesh
        && p.View && p.View->bShowPathCorridor
        && p.PathCorridor && !p.PathCorridor->empty())
    {
        for (dtPolyRef ref : *p.PathCorridor)
        {
            const dtMeshTile* tile = nullptr;
            const dtPoly*     poly = nullptr;
            if (dtStatusFailed(p.Nav->NavMesh->getTileAndPolyByRef(ref, &tile, &poly))) continue;
            if (!tile || !poly) continue;
            std::vector<Vec3> pts;
            pts.reserve(poly->vertCount);
            for (int j = 0; j < poly->vertCount; ++j)
            {
                const float* v = &tile->verts[poly->verts[j] * 3];
                pts.push_back(V3(v[0], v[1] + polyY * 0.6f, v[2]));
            }
            if (pts.size() < 3) continue;
            for (size_t k = 1; k + 1 < pts.size(); ++k)
                Render3D::TriFilled3D(dl, vp, panelMin, panelSize, pts[0], pts[k], pts[k + 1], corridorColU32);
        }
    }

    // 若平滑路径可用且开启，则优先绘制平滑路径；否则绘制原始直线路径
    const std::vector<float>* pathToDraw3 =
        (p.View && p.View->bSmoothPath && p.SmoothedPath && p.SmoothedPath->size() >= 6)
        ? p.SmoothedPath
        : p.StraightPath;

    if (pathToDraw3 && pathToDraw3->size() >= 6)
    {
        const size_t n = pathToDraw3->size() / 3;
        for (size_t i = 0; i + 1 < n; ++i)
        {
            const Vec3 a = V3((*pathToDraw3)[i * 3 + 0],
                              (*pathToDraw3)[i * 3 + 1] + 0.2f,
                              (*pathToDraw3)[i * 3 + 2]);
            const Vec3 b = V3((*pathToDraw3)[(i + 1) * 3 + 0],
                              (*pathToDraw3)[(i + 1) * 3 + 1] + 0.2f,
                              (*pathToDraw3)[(i + 1) * 3 + 2]);
            Render3D::Line3D(dl, vp, panelMin, panelSize, a, b, pathColU32, 2.5f);
        }
    }

    // -------------------------------------------------------------------------
    // 起/终点（胶囊体）
    // -------------------------------------------------------------------------
    if (p.Start && p.End)
    {
        Render3D::DrawCapsule3D(dl, vp, panelMin, panelSize,
                                p.Start[0], p.Start[2], p.AgentRadius, p.AgentHeight,
                                IM_COL32(80, 160, 255, 255), p.Start[1]);
        Render3D::DrawCapsule3D(dl, vp, panelMin, panelSize,
                                p.End[0],   p.End[2],   p.AgentRadius, p.AgentHeight,
                                IM_COL32(255, 80, 80, 255), p.End[1]);
    }

    // -------------------------------------------------------------------------
    // OffMeshLink（3D：两端竖线 + 地面连线）
    // -------------------------------------------------------------------------
    if (p.Geom && !p.Geom->OffMeshLinks.empty() && p.View && p.View->bShowObstacles)
    {
        constexpr ImU32 colStart = IM_COL32(220, 180,  60, 255);
        constexpr ImU32 colEnd   = IM_COL32( 60, 220, 180, 255);
        constexpr ImU32 colArc   = IM_COL32(220, 220,  80, 200);
        for (const OffMeshLink& lk : p.Geom->OffMeshLinks)
        {
            const Vec3 s  = V3(lk.Start[0], lk.Start[1],            lk.Start[2]);
            const Vec3 e  = V3(lk.End  [0], lk.End  [1],            lk.End  [2]);
            const Vec3 s1 = V3(lk.Start[0], lk.Start[1] + 1.5f,     lk.Start[2]);
            const Vec3 e1 = V3(lk.End  [0], lk.End  [1] + 1.5f,     lk.End  [2]);
            const Vec3 mid = V3((s.x + e.x) * 0.5f,
                                std::max(s.y, e.y) + 1.0f,
                                (s.z + e.z) * 0.5f);
            // 两端竖线
            Render3D::Line3D(dl, vp, panelMin, panelSize, s,  s1, colStart);
            Render3D::Line3D(dl, vp, panelMin, panelSize, e,  e1, colEnd);
            // 抛物弧（3 段折线近似）
            Render3D::Line3D(dl, vp, panelMin, panelSize, s1, mid, colArc);
            Render3D::Line3D(dl, vp, panelMin, panelSize, mid, e1, colArc);
            // Render3D 也有 RingXZ 可画端点标记
            Render3D::RingXZ(dl, vp, panelMin, panelSize,
                             lk.Start[0], lk.Start[2], lk.Start[1], lk.Radius, 16, colStart);
            Render3D::RingXZ(dl, vp, panelMin, panelSize,
                             lk.End  [0], lk.End  [2], lk.End  [1], lk.Radius, 16, colEnd);
        }
    }

    // -------------------------------------------------------------------------
    // AutoNavLinks（自动生成的边界 NavLink，紫色 / 品红色）
    // -------------------------------------------------------------------------
    if (p.AutoNavLinks && !p.AutoNavLinks->empty() && p.View && p.View->bShowAutoNavLinks)
    {
        constexpr ImU32 colBidir  = IM_COL32(200,  80, 255, 255);  // 品红（双向）
        constexpr ImU32 colOneWay = IM_COL32(255,  80, 200, 255);  // 洋红（单向）
        constexpr ImU32 colArc    = IM_COL32(180,  60, 240, 200);
        constexpr ImU32 colArcO   = IM_COL32(240,  60, 180, 200);

        for (const OffMeshLink& lk : *p.AutoNavLinks)
        {
            const bool bidir = (lk.Dir == 1);
            const ImU32 colS = colBidir;
            const ImU32 colE = bidir ? colBidir : colOneWay;
            const ImU32 colA = bidir ? colArc   : colArcO;

            const Vec3 s  = V3(lk.Start[0], lk.Start[1],        lk.Start[2]);
            const Vec3 e  = V3(lk.End  [0], lk.End  [1],        lk.End  [2]);
            const Vec3 s1 = V3(lk.Start[0], lk.Start[1] + 1.2f, lk.Start[2]);
            const Vec3 e1 = V3(lk.End  [0], lk.End  [1] + 1.2f, lk.End  [2]);
            const Vec3 mid = V3((s.x + e.x) * 0.5f,
                                std::max(s.y, e.y) + 0.8f,
                                (s.z + e.z) * 0.5f);

            // 竖线
            Render3D::Line3D(dl, vp, panelMin, panelSize, s,  s1, colS);
            Render3D::Line3D(dl, vp, panelMin, panelSize, e,  e1, colE);

            // 连接弧
            Render3D::Line3D(dl, vp, panelMin, panelSize, s1, mid, colA);
            Render3D::Line3D(dl, vp, panelMin, panelSize, mid, e1, colA);

            // 端点环
            Render3D::RingXZ(dl, vp, panelMin, panelSize,
                             lk.Start[0], lk.Start[2], lk.Start[1], lk.Radius, 12, colS);
            Render3D::RingXZ(dl, vp, panelMin, panelSize,
                             lk.End  [0], lk.End  [2], lk.End  [1], lk.Radius, 12, colE);
        }
    }

    // -------------------------------------------------------------------------
    // 障碍创建草稿（Box 框线 / Cylinder 圆环）
    // -------------------------------------------------------------------------
    if (p.CurrentEditMode == EditMode::CreateBox
        && p.CreateDraft && p.CreateDraft->bActive)
    {
        const ImU32 col = IM_COL32(255, 230, 100, 230);
        const float h   = p.DefaultObstacleHeight;
        if (p.DefaultObstacleShape == ObstacleShape::Box)
        {
            const float lo[2] = { std::min(p.CreateDraft->StartX, p.CreateDraft->CurX),
                                  std::min(p.CreateDraft->StartZ, p.CreateDraft->CurZ) };
            const float hi[2] = { std::max(p.CreateDraft->StartX, p.CreateDraft->CurX),
                                  std::max(p.CreateDraft->StartZ, p.CreateDraft->CurZ) };
            Vec3 a{ lo[0], h, lo[1] }, b{ hi[0], h, lo[1] }, c{ hi[0], h, hi[1] }, d{ lo[0], h, hi[1] };
            Vec3 a0{ lo[0], 0, lo[1] }, b0{ hi[0], 0, lo[1] }, c0{ hi[0], 0, hi[1] }, d0{ lo[0], 0, hi[1] };
            Render3D::Line3D(dl, vp, panelMin, panelSize, a,  b,  col);
            Render3D::Line3D(dl, vp, panelMin, panelSize, b,  c,  col);
            Render3D::Line3D(dl, vp, panelMin, panelSize, c,  d,  col);
            Render3D::Line3D(dl, vp, panelMin, panelSize, d,  a,  col);
            Render3D::Line3D(dl, vp, panelMin, panelSize, a0, b0, col);
            Render3D::Line3D(dl, vp, panelMin, panelSize, b0, c0, col);
            Render3D::Line3D(dl, vp, panelMin, panelSize, c0, d0, col);
            Render3D::Line3D(dl, vp, panelMin, panelSize, d0, a0, col);
            Render3D::Line3D(dl, vp, panelMin, panelSize, a0, a,  col);
            Render3D::Line3D(dl, vp, panelMin, panelSize, b0, b,  col);
            Render3D::Line3D(dl, vp, panelMin, panelSize, c0, c,  col);
            Render3D::Line3D(dl, vp, panelMin, panelSize, d0, d,  col);
        }
        else
        {
            const float dx = (p.CreateDraft->CurX - p.CreateDraft->StartX);
            const float dz = (p.CreateDraft->CurZ - p.CreateDraft->StartZ);
            const float r  = std::sqrt(dx * dx + dz * dz);
            if (r > 0.05f)
            {
                Render3D::RingXZ(dl, vp, panelMin, panelSize,
                                 p.CreateDraft->StartX, p.CreateDraft->StartZ, 0.0f, r, 24, col);
                Render3D::RingXZ(dl, vp, panelMin, panelSize,
                                 p.CreateDraft->StartX, p.CreateDraft->StartZ, h,    r, 24, col);
                for (int i = 0; i < 8; ++i)
                {
                    const float a = (i / 8.0f) * 6.28318530718f;
                    const float x = p.CreateDraft->StartX + r * std::cos(a);
                    const float z = p.CreateDraft->StartZ + r * std::sin(a);
                    Render3D::Line3D(dl, vp, panelMin, panelSize, V3(x, 0, z), V3(x, h, z), col);
                }
            }
        }
    }

    if (useCpuZ)
    {
        DepthRaster::End();
        const uint8_t* px = DepthRaster::PixelsRGBA();
        if (px)
        {
            NavViewTextureBridgeUpload(DepthRaster::BufferWidth(), DepthRaster::BufferHeight(), px);
            const ImTextureID tid = NavViewTextureBridgeTextureId();
            if (tid)
                dl->AddImage(tid, panelMin, panelMax, ImVec2(0, 0), ImVec2(1, 1));
        }
    }
    else if (useGpu)
    {
        GpuRenderer::End();
        const ImTextureID tid = GpuRenderer::TextureId();
        if (tid)
        {
#if defined(_WIN32)
            // D3D12：纹理顶部对应 NDC y=+1（v=0 在顶部，与 ImGui 一致）
            dl->AddImage(tid, panelMin, panelMax, ImVec2(0, 0), ImVec2(1, 1));
#else
            // OpenGL FBO：v=0 在底部，反转 V 才能正立显示
            dl->AddImage(tid, panelMin, panelMax, ImVec2(0, 1), ImVec2(1, 0));
#endif
        }
    }
    else if (usePainter)
        PainterSort::Flush(dl);

    // 世界坐标轴 widget（屏幕空间叠绘，永远位于场景纹理之上、面板边框之下）
    DrawWorldAxisGizmo(dl, panelMin, panelSize, cam);

    dl->AddRect(panelMin, panelMax, IM_COL32(90, 90, 100, 255));
    dl->PopClipRect();
    return result;
}

} // namespace Renderer3D
