/*
 * Render/NavStepDebug.cpp
 * -----------------------
 * NavStepBuilder 各阶段中间数据可视化叠绘实现。
 */

#include "NavStepDebug.h"
#include "Primitives.h"
#include "Renderer2D.h"   // HashColor / ColU32

#include "Recast.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace NavStepDebug
{

namespace
{

// ---- 工具：根据世界 cell 坐标得到 XZ 平面四角 ----
inline void CellCornerXZ(float bminX, float bminZ, float cs, int x, int z,
                         Vec3 out[4], float yPlane)
{
    const float x0 = bminX + x * cs;
    const float z0 = bminZ + z * cs;
    const float x1 = x0 + cs;
    const float z1 = z0 + cs;
    out[0] = V3(x0, yPlane, z0);
    out[1] = V3(x1, yPlane, z0);
    out[2] = V3(x1, yPlane, z1);
    out[3] = V3(x0, yPlane, z1);
}

inline ImU32 RegionColor(unsigned short reg, unsigned char alpha = 200)
{
    if (reg == 0) return IM_COL32(110, 110, 110, alpha);
    // 借用 Renderer2D 的 HashColor，与 NavMesh 的 region 着色风格一致
    const ImU32 c = Renderer2D::HashColor(reg);
    const unsigned char r = (c >>  0) & 0xFF;
    const unsigned char g = (c >>  8) & 0xFF;
    const unsigned char b = (c >> 16) & 0xFF;
    return IM_COL32(r, g, b, alpha);
}

inline ImU32 AreaColor(unsigned char area, unsigned char alpha = 180)
{
    if (area == RC_NULL_AREA)     return IM_COL32(85, 85, 95, alpha);
    if (area == RC_WALKABLE_AREA) return IM_COL32(80, 200, 110, alpha);
    return IM_COL32(220, 180, 60, alpha);  // 自定义区域
}

inline ImU32 DistColor(unsigned short d, unsigned short maxD, unsigned char alpha = 180)
{
    const float t = maxD > 0 ? std::min(1.0f, static_cast<float>(d) / static_cast<float>(maxD)) : 0.0f;
    // 蓝紫(远) -> 黄(近边界)：t=1 离边界最远；t=0 紧贴边界
    const float r = 1.0f - t;
    const float g = 0.6f * t + 0.2f;
    const float b = t * 0.9f;
    return IM_COL32(static_cast<int>(r * 255), static_cast<int>(g * 255),
                    static_cast<int>(b * 255), alpha);
}

// =============================================================================
// Step 1 (Config) - 仅 BV/Bbox 与体素网格在底面的轮廓
// =============================================================================
void DrawStep1Config(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
                     const NavStepBuilder::StepBuilder& sb)
{
    const rcConfig& c = sb.Cfg;
    if (c.cs <= 0.0f || c.width <= 0 || c.height <= 0) return;

    const float bminX = c.bmin[0], bminY = c.bmin[1], bminZ = c.bmin[2];
    const float bmaxX = c.bmax[0], bmaxY = c.bmax[1], bmaxZ = c.bmax[2];

    const ImU32 colBox = IM_COL32(255, 220, 90, 220);
    const ImU32 colGrid= IM_COL32(120, 180, 255, 100);

    // 体素 AABB 12 条棱
    const Vec3 c0 = V3(bminX, bminY, bminZ);
    const Vec3 c1 = V3(bmaxX, bminY, bminZ);
    const Vec3 c2 = V3(bmaxX, bminY, bmaxZ);
    const Vec3 c3 = V3(bminX, bminY, bmaxZ);
    const Vec3 c4 = V3(bminX, bmaxY, bminZ);
    const Vec3 c5 = V3(bmaxX, bmaxY, bminZ);
    const Vec3 c6 = V3(bmaxX, bmaxY, bmaxZ);
    const Vec3 c7 = V3(bminX, bmaxY, bmaxZ);
    Render3D::Line3D(dl, vp, vMin, vSize, c0, c1, colBox, 2.0f);
    Render3D::Line3D(dl, vp, vMin, vSize, c1, c2, colBox, 2.0f);
    Render3D::Line3D(dl, vp, vMin, vSize, c2, c3, colBox, 2.0f);
    Render3D::Line3D(dl, vp, vMin, vSize, c3, c0, colBox, 2.0f);
    Render3D::Line3D(dl, vp, vMin, vSize, c4, c5, colBox, 2.0f);
    Render3D::Line3D(dl, vp, vMin, vSize, c5, c6, colBox, 2.0f);
    Render3D::Line3D(dl, vp, vMin, vSize, c6, c7, colBox, 2.0f);
    Render3D::Line3D(dl, vp, vMin, vSize, c7, c4, colBox, 2.0f);
    Render3D::Line3D(dl, vp, vMin, vSize, c0, c4, colBox, 2.0f);
    Render3D::Line3D(dl, vp, vMin, vSize, c1, c5, colBox, 2.0f);
    Render3D::Line3D(dl, vp, vMin, vSize, c2, c6, colBox, 2.0f);
    Render3D::Line3D(dl, vp, vMin, vSize, c3, c7, colBox, 2.0f);

    // 体素栅格底面：每 stride 条线绘制一次（避免太密）
    const int stride = std::max(1, std::max(c.width, c.height) / 64);
    for (int x = 0; x <= c.width; x += stride)
    {
        const float wx = bminX + x * c.cs;
        Render3D::Line3D(dl, vp, vMin, vSize,
                         V3(wx, bminY + 0.001f, bminZ),
                         V3(wx, bminY + 0.001f, bmaxZ),
                         colGrid, 1.0f);
    }
    for (int z = 0; z <= c.height; z += stride)
    {
        const float wz = bminZ + z * c.cs;
        Render3D::Line3D(dl, vp, vMin, vSize,
                         V3(bminX, bminY + 0.001f, wz),
                         V3(bmaxX, bminY + 0.001f, wz),
                         colGrid, 1.0f);
    }
}

// =============================================================================
// Step 2/3 (Heightfield) - 体素 spans 顶面四边形
// =============================================================================
void DrawStep2Or3Heightfield(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
                             const NavStepBuilder::StepBuilder& sb,
                             bool walkableOnly,
                             int  maxCells)
{
    if (!sb.Solid) return;
    const rcHeightfield& hf = *sb.Solid;
    if (hf.width <= 0 || hf.height <= 0) return;

    const int  totalCells = hf.width * hf.height;
    const int  stride     = std::max(1, static_cast<int>(std::ceil(
                              std::sqrt(static_cast<double>(totalCells) /
                                        std::max(1, maxCells / 8)))));
    const float bminX = hf.bmin[0], bminY = hf.bmin[1], bminZ = hf.bmin[2];

    int drawn = 0;
    for (int z = 0; z < hf.height; z += stride)
    {
        for (int x = 0; x < hf.width; x += stride)
        {
            for (rcSpan* s = hf.spans[x + z * hf.width]; s; s = s->next)
            {
                const unsigned char area = static_cast<unsigned char>(s->area);
                if (walkableOnly && area == RC_NULL_AREA) continue;

                const float yTop = bminY + s->smax * hf.ch;
                Vec3 q[4];
                CellCornerXZ(bminX, bminZ, hf.cs * stride, x / stride, z / stride, q, yTop);
                // 但 cellsize 在 stride>1 时仍按 hf.cs * stride 显示一个粗块
                const ImU32 col = AreaColor(area, walkableOnly ? 200 : 160);
                Render3D::TriFilled3D(dl, vp, vMin, vSize, q[0], q[1], q[2], col);
                Render3D::TriFilled3D(dl, vp, vMin, vSize, q[0], q[2], q[3], col);
                if (++drawn >= maxCells) return;
            }
        }
    }
}

// =============================================================================
// Step 4/5/6 (CompactHeightfield) - 紧凑 spans 顶面四边形 + 着色
//
// mode:
//   0 = Step4 (CompactHF): 按 area 着色
//   1 = Step5 (Erode/DistField): 按 dist 着色
//   2 = Step6 (Regions):   按 reg id 着色
// =============================================================================
void DrawCompactHeightfield(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
                            const NavStepBuilder::StepBuilder& sb,
                            int mode,
                            int maxCells)
{
    if (!sb.Chf) return;
    const rcCompactHeightfield& chf = *sb.Chf;
    if (chf.spanCount <= 0) return;

    const float bminX = chf.bmin[0], bminY = chf.bmin[1], bminZ = chf.bmin[2];
    const int   stride= std::max(1, static_cast<int>(std::ceil(
                          std::sqrt(static_cast<double>(chf.spanCount) /
                                    std::max(1, maxCells / 8)))));
    int drawn = 0;
    for (int z = 0; z < chf.height; z += stride)
    {
        for (int x = 0; x < chf.width; x += stride)
        {
            const rcCompactCell& cc = chf.cells[x + z * chf.width];
            const int begin = static_cast<int>(cc.index);
            const int end   = begin + static_cast<int>(cc.count);
            for (int i = begin; i < end; ++i)
            {
                const rcCompactSpan& cs = chf.spans[i];
                const unsigned char  ar = chf.areas[i];

                ImU32 col = IM_COL32(85, 85, 95, 180);
                if (mode == 0)
                    col = AreaColor(ar);
                else if (mode == 1)
                {
                    if (ar == RC_NULL_AREA)
                        col = IM_COL32(45, 45, 50, 120);
                    else if (chf.dist)
                        col = DistColor(chf.dist[i], chf.maxDistance);
                    else
                        col = AreaColor(ar);
                }
                else /*mode == 2*/
                {
                    col = (ar == RC_NULL_AREA)
                        ? IM_COL32(45, 45, 50, 120)
                        : RegionColor(cs.reg);
                }

                const float yTop = bminY + cs.y * chf.ch + 0.005f;
                Vec3 q[4];
                CellCornerXZ(bminX, bminZ, chf.cs * stride, x / stride, z / stride, q, yTop);
                Render3D::TriFilled3D(dl, vp, vMin, vSize, q[0], q[1], q[2], col);
                Render3D::TriFilled3D(dl, vp, vMin, vSize, q[0], q[2], q[3], col);
                if (++drawn >= maxCells) return;
            }
        }
    }
}

// =============================================================================
// Step 7 (Contours) - 简化轮廓多边形折线
// =============================================================================
void DrawContours(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
                  const NavStepBuilder::StepBuilder& sb)
{
    if (!sb.Cset) return;
    const rcContourSet& cs = *sb.Cset;
    if (cs.nconts <= 0) return;

    for (int i = 0; i < cs.nconts; ++i)
    {
        const rcContour& cont = cs.conts[i];
        if (cont.nverts < 2) continue;
        const ImU32 col = RegionColor(cont.reg, 245);

        for (int j = 0, k = cont.nverts - 1; j < cont.nverts; k = j++)
        {
            const int* va = &cont.verts[k * 4];
            const int* vb = &cont.verts[j * 4];
            const Vec3 a = V3(cs.bmin[0] + va[0] * cs.cs,
                              cs.bmin[1] + va[1] * cs.ch + 0.02f,
                              cs.bmin[2] + va[2] * cs.cs);
            const Vec3 b = V3(cs.bmin[0] + vb[0] * cs.cs,
                              cs.bmin[1] + vb[1] * cs.ch + 0.02f,
                              cs.bmin[2] + vb[2] * cs.cs);
            Render3D::Line3D(dl, vp, vMin, vSize, a, b, col, 2.0f);
        }
    }
}

// =============================================================================
// Step 8/9 (PolyMesh / DetailMesh) - 在 NavRuntime 已经渲染过；
// 此处为 StepBuilder 自有的 PolyMesh 副本提供轻量描边（区分主流程）
// =============================================================================
void DrawPolyMesh(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
                  const NavStepBuilder::StepBuilder& sb,
                  bool fillFaces)
{
    if (!sb.Pmesh) return;
    const rcPolyMesh& pm = *sb.Pmesh;
    if (pm.npolys <= 0) return;

    const float bminX = pm.bmin[0], bminY = pm.bmin[1], bminZ = pm.bmin[2];
    const int   nvp   = pm.nvp;

    auto VertWorld = [&](int idx) -> Vec3
    {
        const unsigned short* v = &pm.verts[idx * 3];
        return V3(bminX + v[0] * pm.cs,
                  bminY + v[1] * pm.ch + 0.04f,
                  bminZ + v[2] * pm.cs);
    };

    for (int i = 0; i < pm.npolys; ++i)
    {
        const unsigned short* p = &pm.polys[i * nvp * 2];
        unsigned short reg = pm.regs ? pm.regs[i] : static_cast<unsigned short>(i + 1);
        const ImU32 fillCol = RegionColor(reg, 110);
        const ImU32 edgeCol = RegionColor(reg, 230);

        // 折线收尾闭合
        int n = 0;
        for (; n < nvp; ++n) { if (p[n] == RC_MESH_NULL_IDX) break; }
        if (n < 3) continue;

        if (fillFaces)
        {
            // 三角扇填充
            const Vec3 a = VertWorld(p[0]);
            for (int k = 1; k < n - 1; ++k)
            {
                const Vec3 b = VertWorld(p[k]);
                const Vec3 c = VertWorld(p[k + 1]);
                Render3D::TriFilled3D(dl, vp, vMin, vSize, a, b, c, fillCol);
            }
        }
        for (int k = 0; k < n; ++k)
        {
            const Vec3 va = VertWorld(p[k]);
            const Vec3 vb = VertWorld(p[(k + 1) % n]);
            Render3D::Line3D(dl, vp, vMin, vSize, va, vb, edgeCol, 1.5f);
        }
    }
}

void DrawDetailMesh(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
                    const NavStepBuilder::StepBuilder& sb)
{
    if (!sb.Dmesh) return;
    const rcPolyMeshDetail& dm = *sb.Dmesh;
    if (dm.nmeshes <= 0) return;

    const ImU32 col = IM_COL32(140, 220, 255, 180);
    for (int i = 0; i < dm.nmeshes; ++i)
    {
        const unsigned int* m   = &dm.meshes[i * 4];
        const int           bv  = static_cast<int>(m[0]);
        const int           btr = static_cast<int>(m[2]);
        const int           ntr = static_cast<int>(m[3]);
        for (int j = 0; j < ntr; ++j)
        {
            const unsigned char* tri = &dm.tris[(btr + j) * 4];
            const float* va = &dm.verts[(bv + tri[0]) * 3];
            const float* vb = &dm.verts[(bv + tri[1]) * 3];
            const float* vc = &dm.verts[(bv + tri[2]) * 3];
            const Vec3 a = V3(va[0], va[1] + 0.05f, va[2]);
            const Vec3 b = V3(vb[0], vb[1] + 0.05f, vb[2]);
            const Vec3 c = V3(vc[0], vc[1] + 0.05f, vc[2]);
            Render3D::Line3D(dl, vp, vMin, vSize, a, b, col, 1.0f);
            Render3D::Line3D(dl, vp, vMin, vSize, b, c, col, 1.0f);
            Render3D::Line3D(dl, vp, vMin, vSize, c, a, col, 1.0f);
        }
    }
}

} // anonymous namespace

// =============================================================================
// 入口
// =============================================================================
void Draw3D(ImDrawList* dl, ImVec2 vMin, ImVec2 vSize, const DrawParams& p)
{
    if (!p.bEnabled || !p.Map || !p.Map->bValid || !p.Step) return;
    const NavStepBuilder::StepBuilder& sb = *p.Step;
    if (sb.LastCompleted == NavStepBuilder::Step::None &&
        !NavStepBuilder::IsActive(sb))
        return;

    const Mat4 vp = MatMul(p.Map->Proj, p.Map->View);

    const ImVec2 vMax(vMin.x + vSize.x, vMin.y + vSize.y);
    dl->PushClipRect(vMin, vMax, true);

    using NavStepBuilder::Step;
    switch (sb.LastCompleted)
    {
        case Step::None:
            // 进行中但还没完成任何 step：什么都不画
            break;
        case Step::Config:
            DrawStep1Config(dl, vp, vMin, vSize, sb);
            break;
        case Step::Rasterize:
            DrawStep1Config(dl, vp, vMin, vSize, sb);  // 同时显示 BV，便于对照
            DrawStep2Or3Heightfield(dl, vp, vMin, vSize, sb, /*walkableOnly*/ false, p.MaxCellsToDraw);
            break;
        case Step::Filter:
            DrawStep1Config(dl, vp, vMin, vSize, sb);
            DrawStep2Or3Heightfield(dl, vp, vMin, vSize, sb, /*walkableOnly*/ true, p.MaxCellsToDraw);
            break;
        case Step::CompactHF:
            DrawCompactHeightfield(dl, vp, vMin, vSize, sb, /*mode*/ 0, p.MaxCellsToDraw);
            break;
        case Step::Erode:
            DrawCompactHeightfield(dl, vp, vMin, vSize, sb, /*mode*/ 1, p.MaxCellsToDraw);
            break;
        case Step::Regions:
            DrawCompactHeightfield(dl, vp, vMin, vSize, sb, /*mode*/ 2, p.MaxCellsToDraw);
            break;
        case Step::Contours:
            DrawCompactHeightfield(dl, vp, vMin, vSize, sb, /*mode*/ 2, p.MaxCellsToDraw / 4);
            DrawContours(dl, vp, vMin, vSize, sb);
            break;
        case Step::PolyMesh:
            DrawPolyMesh(dl, vp, vMin, vSize, sb, /*fillFaces*/ true);
            break;
        case Step::DetailMesh:
            DrawPolyMesh(dl, vp, vMin, vSize, sb, /*fillFaces*/ false);
            DrawDetailMesh(dl, vp, vMin, vSize, sb);
            break;
        case Step::DetourNav:
            // Step10 完成后 Pmesh/Dmesh 已转交给 NavRuntime；
            // 此时主流程渲染器 (Renderer3D) 会绘制最终 NavMesh，无需重复。
            break;
    }

    dl->PopClipRect();
}

} // namespace NavStepDebug
