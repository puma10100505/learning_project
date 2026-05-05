/*
 * Render/Primitives.cpp
 * ---------------------
 * 3D 软件光栅化绘制原语实现。
 * 详细说明见 Primitives.h。
 */

#include "Primitives.h"
#include "DepthRaster.h"
#include "GpuRenderer.h"
#include "PainterSort.h"

#include <cmath>
#include <algorithm>

namespace Render3D
{

// =============================================================================
// 基础投影
// =============================================================================

ProjectedPoint Project(const Mat4& vp, Vec3 wp, ImVec2 vMin, ImVec2 vSize)
{
    float c[4];
    TransformPoint(vp, wp, c);
    ProjectedPoint r{};
    r.w = c[3];
    if (c[3] <= 1e-3f) { r.ok = false; r.px = ImVec2(0, 0); return r; }
    const float ndcX = c[0] / c[3];
    const float ndcY = c[1] / c[3];
    r.px.x = vMin.x + (ndcX * 0.5f + 0.5f) * vSize.x;
    r.px.y = vMin.y + (1.0f - (ndcY * 0.5f + 0.5f)) * vSize.y;
    r.ok = true;
    return r;
}

void Line3D_DrawImmediate(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
                            Vec3 a, Vec3 b, ImU32 col, float thickness)
{
    ProjectedPoint pa = Project(vp, a, vMin, vSize);
    ProjectedPoint pb = Project(vp, b, vMin, vSize);
    if (pa.ok && pb.ok) dl->AddLine(pa.px, pb.px, col, thickness);
}

void TriFilled3D_DrawImmediate(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
                               Vec3 a, Vec3 b, Vec3 c, ImU32 col)
{
    ProjectedPoint pa = Project(vp, a, vMin, vSize);
    ProjectedPoint pb = Project(vp, b, vMin, vSize);
    ProjectedPoint pc = Project(vp, c, vMin, vSize);
    if (pa.ok && pb.ok && pc.ok) dl->AddTriangleFilled(pa.px, pb.px, pc.px, col);
}

void Line3D(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
            Vec3 a, Vec3 b, ImU32 col, float thickness)
{
    if (PainterSort::IsCollecting())
    {
        PainterSort::RecordLine(vp, vMin, vSize, a, b, col, thickness);
        return;
    }
    if (GpuRenderer::IsActive())
    {
        GpuRenderer::DrawLineWorld(vp, a, b, col, thickness);
        return;
    }
    if (DepthRaster::IsActive())
    {
        DepthRaster::DrawLineWorld(vp, a, b, col, thickness);
        return;
    }
    Line3D_DrawImmediate(dl, vp, vMin, vSize, a, b, col, thickness);
}

void TriFilled3D(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
                 Vec3 a, Vec3 b, Vec3 c, ImU32 col)
{
    if (PainterSort::IsCollecting())
    {
        PainterSort::RecordTri(vp, vMin, vSize, a, b, c, col);
        return;
    }
    if (GpuRenderer::IsActive())
    {
        GpuRenderer::DrawTriangleWorld(vp, a, b, c, col);
        return;
    }
    if (DepthRaster::IsActive())
    {
        DepthRaster::DrawTriangleWorld(vp, a, b, c, col);
        return;
    }
    TriFilled3D_DrawImmediate(dl, vp, vMin, vSize, a, b, c, col);
}

// =============================================================================
// 复合几何原语
// =============================================================================

void RingXZ(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
            float cx, float cz, float y, float r, int segs, ImU32 col)
{
    Vec3 prev = V3(cx + r, y, cz);
    for (int i = 1; i <= segs; ++i)
    {
        const float a   = (i / (float)segs) * 6.28318530718f;
        const Vec3  cur = V3(cx + r * std::cos(a), y, cz + r * std::sin(a));
        Line3D(dl, vp, vMin, vSize, prev, cur, col);
        prev = cur;
    }
}

void DiscXZFilled(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
                  float cx, float cz, float y, float r, int segs, ImU32 col)
{
    // 顶点序 (center, cur, prev)：从 +Y 上方俯视为顺时针，
    // 经 cross(e1,e2) 算出的面法线指向 +Y（朝上），与"圆柱顶面朝上"一致。
    // 旧顺序 (center, prev, cur) 法线朝 −Y，会被深度模式 (CpuZBuffer / GpuShader)
    // 的背面剔除丢掉，导致圆柱顶面不封闭、能看穿到内壁。
    Vec3 prev = V3(cx + r, y, cz);
    for (int i = 1; i <= segs; ++i)
    {
        const float a   = (i / (float)segs) * 6.28318530718f;
        const Vec3  cur = V3(cx + r * std::cos(a), y, cz + r * std::sin(a));
        TriFilled3D(dl, vp, vMin, vSize, V3(cx, y, cz), cur, prev, col);
        prev = cur;
    }
}

void DrawCylinderObstacle3D(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
                             float cx, float cz, float r, float h,
                             ImU32 fillTop, ImU32 edgeCol)
{
    constexpr int segs = 24;
    // 顶面填充
    DiscXZFilled(dl, vp, vMin, vSize, cx, cz, h, r, segs, fillTop);
    // 顶/底圆环
    RingXZ(dl, vp, vMin, vSize, cx, cz, 0.0f, r, segs, edgeCol);
    RingXZ(dl, vp, vMin, vSize, cx, cz, h,    r, segs, edgeCol);
    // 8 条立柱
    for (int i = 0; i < 8; ++i)
    {
        const float a = (i / 8.0f) * 6.28318530718f;
        const float x = cx + r * std::cos(a);
        const float z = cz + r * std::sin(a);
        Line3D(dl, vp, vMin, vSize, V3(x, 0, z), V3(x, h, z), edgeCol);
    }
}

void DrawCylinderObstacleShaded3D(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
                                const Vec3& eyeWorld,
                                float cx, float cz, float r, float h,
                                ImU32 topCol, ImU32 sideFrontCol, ImU32 sideBackCol, ImU32 edgeCol)
{
    constexpr int segs = 24;
    DiscXZFilled(dl, vp, vMin, vSize, cx, cz, h, r, segs, topCol);

    for (int i = 0; i < segs; ++i)
    {
        const float a0 = (i / (float)segs) * 6.28318530718f;
        const float a1 = ((i + 1) / (float)segs) * 6.28318530718f;
        const Vec3  p00(cx + r * std::cos(a0), 0.0f, cz + r * std::sin(a0));
        const Vec3  p01(cx + r * std::cos(a0), h, cz + r * std::sin(a0));
        const Vec3  p10(cx + r * std::cos(a1), 0.0f, cz + r * std::sin(a1));
        const Vec3  p11(cx + r * std::cos(a1), h, cz + r * std::sin(a1));
        const float amid = (a0 + a1) * 0.5f;
        const Vec3  n(std::cos(amid), 0.0f, std::sin(amid));
        const Vec3  ctr = (p00 + p01 + p10 + p11) * 0.25f;
        const bool  front = Dot(n, eyeWorld - ctr) > 1e-5f;
        if (!front && !DepthRaster::IsActive() && !GpuRenderer::IsActive())
            continue; // 无深度缓冲时剔除远侧；有 Z-Buffer / GPU 深度时两面都画
        const ImU32 col = front ? sideFrontCol : sideBackCol;
        TriFilled3D(dl, vp, vMin, vSize, p00, p01, p11, col);
        TriFilled3D(dl, vp, vMin, vSize, p00, p11, p10, col);
    }

    RingXZ(dl, vp, vMin, vSize, cx, cz, 0.0f, r, segs, edgeCol);
    RingXZ(dl, vp, vMin, vSize, cx, cz, h, r, segs, edgeCol);
    for (int i = 0; i < 8; ++i)
    {
        const float a = (i / 8.0f) * 6.28318530718f;
        const float x = cx + r * std::cos(a);
        const float z = cz + r * std::sin(a);
        Line3D(dl, vp, vMin, vSize, V3(x, 0, z), V3(x, h, z), edgeCol);
    }
}

void DrawCapsule3D(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
                   float cx, float cz, float r, float height, ImU32 col,
                   float groundY)
{
    if (r <= 0.0f) return;
    const float h    = std::max(height, 2.0f * r);
    const float yBot = groundY + r;          // 底半球与圆柱的接缝
    const float yTop = groundY + h - r;      // 圆柱与顶半球的接缝
    constexpr int ringSeg = 18;
    constexpr int hemiSeg = 6;
    constexpr int meridi  = 4;               // 经线条数

    // 三条圆环（底/中/顶）
    RingXZ(dl, vp, vMin, vSize, cx, cz, yBot,               r, ringSeg, col);
    RingXZ(dl, vp, vMin, vSize, cx, cz, groundY + h * 0.5f, r, ringSeg, col);
    RingXZ(dl, vp, vMin, vSize, cx, cz, yTop,               r, ringSeg, col);

    // 圆柱侧立柱
    for (int i = 0; i < meridi; ++i)
    {
        const float a = (i / (float)meridi) * 6.28318530718f;
        const float x = cx + r * std::cos(a);
        const float z = cz + r * std::sin(a);
        Line3D(dl, vp, vMin, vSize, V3(x, yBot, z), V3(x, yTop, z), col);
    }
    // 顶半球经线弧
    for (int i = 0; i < meridi; ++i)
    {
        const float a  = (i / (float)meridi) * 6.28318530718f;
        const float ca = std::cos(a), sa = std::sin(a);
        Vec3 prev = V3(cx + r * ca, yTop, cz + r * sa);
        for (int k = 1; k <= hemiSeg; ++k)
        {
            const float phi = (k / (float)hemiSeg) * 1.57079632679f;
            const float rr  = r * std::cos(phi);
            const float yy  = yTop + r * std::sin(phi);
            Vec3 cur = V3(cx + rr * ca, yy, cz + rr * sa);
            Line3D(dl, vp, vMin, vSize, prev, cur, col);
            prev = cur;
        }
    }
    // 底半球经线弧
    for (int i = 0; i < meridi; ++i)
    {
        const float a  = (i / (float)meridi) * 6.28318530718f;
        const float ca = std::cos(a), sa = std::sin(a);
        Vec3 prev = V3(cx + r * ca, yBot, cz + r * sa);
        for (int k = 1; k <= hemiSeg; ++k)
        {
            const float phi = (k / (float)hemiSeg) * 1.57079632679f;
            const float rr  = r * std::cos(phi);
            const float yy  = yBot - r * std::sin(phi);
            Vec3 cur = V3(cx + rr * ca, yy, cz + rr * sa);
            Line3D(dl, vp, vMin, vSize, prev, cur, col);
            prev = cur;
        }
    }
}

} // namespace Render3D
