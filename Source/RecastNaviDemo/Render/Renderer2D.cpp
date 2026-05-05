/*
 * Render/Renderer2D.cpp
 * ---------------------
 * 2D 俯视图软件渲染实现。
 * 详细说明见 Renderer2D.h。
 */

#include "Renderer2D.h"

#include <cmath>
#include <cstring>
#include <algorithm>
#include <vector>

// Recast
#include "Recast.h"

// Detour
#include "DetourNavMesh.h"
#include "DetourStatus.h"

namespace Renderer2D
{

// =============================================================================
// 颜色工具
// =============================================================================

ImU32 HashColor(unsigned int id, float alpha)
{
    unsigned int h = id * 2654435761u;
    const float r = ((h >> 16) & 0xFF) / 255.0f;
    const float g = ((h >> 8)  & 0xFF) / 255.0f;
    const float b = ((h)       & 0xFF) / 255.0f;
    const float boost = 0.35f;
    return IM_COL32(
        (int)((r * (1.0f - boost) + boost) * 255),
        (int)((g * (1.0f - boost) + boost) * 255),
        (int)((b * (1.0f - boost) + boost) * 255),
        (int)(alpha * 255));
}

ImU32 ColU32(const ImVec4& c, float alphaOverride)
{
    const float a = alphaOverride >= 0.0f ? alphaOverride : c.w;
    return IM_COL32(static_cast<int>(c.x * 255),
                    static_cast<int>(c.y * 255),
                    static_cast<int>(c.z * 255),
                    static_cast<int>(a   * 255));
}

// =============================================================================
// 内部工具
// =============================================================================

// 世界坐标 (wx, wz) → 画布屏幕坐标
static ImVec2 World2D(float wx, float wz,
                      ImVec2 cmin, ImVec2 csz,
                      const float bmin[3], const float bmax[3])
{
    const float fx = (wx - bmin[0]) / (bmax[0] - bmin[0]);
    const float fz = (wz - bmin[2]) / (bmax[2] - bmin[2]);
    return ImVec2(cmin.x + fx * csz.x, cmin.y + (1.0f - fz) * csz.y);
}

static bool AnyObstacleContainsXZ(float x, float z, const std::vector<Obstacle>& obs)
{
    for (const Obstacle& o : obs)
        if (PointInsideObstacle(x, z, o))
            return true;
    return false;
}

/// 俯视碰撞地面：障碍 footprint 内不填充；细分三角减轻边界锯齿
static void DrawGroundTriFootprint2D(ImDrawList* dl,
                                     ImVec2 cmin, ImVec2 csz,
                                     const float bmin[3], const float bmax[3],
                                     const std::vector<Obstacle>* obstacles, ImU32 col,
                                     const float* va, const float* vb, const float* vc, int depth)
{
    if (!obstacles || obstacles->empty())
    {
        const ImVec2 pa = World2D(va[0], va[2], cmin, csz, bmin, bmax);
        const ImVec2 pb = World2D(vb[0], vb[2], cmin, csz, bmin, bmax);
        const ImVec2 pc = World2D(vc[0], vc[2], cmin, csz, bmin, bmax);
        dl->AddTriangleFilled(pa, pb, pc, col);
        return;
    }
    const std::vector<Obstacle>& obs = *obstacles;
    const auto                   inV = [&](const float* v) { return AnyObstacleContainsXZ(v[0], v[2], obs); };
    const bool                   oa  = inV(va);
    const bool                   ob  = inV(vb);
    const bool                   oc  = inV(vc);
    const float                  gx  = (va[0] + vb[0] + vc[0]) * (1.0f / 3.0f);
    const float                  gz  = (va[2] + vb[2] + vc[2]) * (1.0f / 3.0f);
    const bool                   og  = AnyObstacleContainsXZ(gx, gz, obs);
    if (oa && ob && oc && og)
        return;
    if (!oa && !ob && !oc && !og)
    {
        const ImVec2 pa = World2D(va[0], va[2], cmin, csz, bmin, bmax);
        const ImVec2 pb = World2D(vb[0], vb[2], cmin, csz, bmin, bmax);
        const ImVec2 pc = World2D(vc[0], vc[2], cmin, csz, bmin, bmax);
        dl->AddTriangleFilled(pa, pb, pc, col);
        return;
    }
    constexpr int kMaxDepth = 2;
    if (depth >= kMaxDepth)
    {
        const ImVec2 pa = World2D(va[0], va[2], cmin, csz, bmin, bmax);
        const ImVec2 pb = World2D(vb[0], vb[2], cmin, csz, bmin, bmax);
        const ImVec2 pc = World2D(vc[0], vc[2], cmin, csz, bmin, bmax);
        dl->AddTriangleFilled(pa, pb, pc, col);
        return;
    }
    float mab[3], mbc[3], mca[3];
    for (int i = 0; i < 3; ++i)
    {
        mab[i] = 0.5f * (va[i] + vb[i]);
        mbc[i] = 0.5f * (vb[i] + vc[i]);
        mca[i] = 0.5f * (vc[i] + va[i]);
    }
    DrawGroundTriFootprint2D(dl, cmin, csz, bmin, bmax, obstacles, col, va, mab, mca, depth + 1);
    DrawGroundTriFootprint2D(dl, cmin, csz, bmin, bmax, obstacles, col, vb, mbc, mab, depth + 1);
    DrawGroundTriFootprint2D(dl, cmin, csz, bmin, bmax, obstacles, col, vc, mca, mbc, depth + 1);
    DrawGroundTriFootprint2D(dl, cmin, csz, bmin, bmax, obstacles, col, mab, mbc, mca, depth + 1);
}

// =============================================================================
// 主绘制函数
// =============================================================================

Map2D DrawCanvas2D(ImDrawList* dl, ImVec2 panelMin, ImVec2 panelSize,
                   const Draw2DParams& p)
{
    Map2D result{};   // bValid = false

    const ImVec2 panelMax(panelMin.x + panelSize.x, panelMin.y + panelSize.y);
    dl->AddRectFilled(panelMin, panelMax, IM_COL32(40, 42, 48, 255));
    dl->AddRect(panelMin, panelMax, IM_COL32(90, 90, 100, 255));

    if (!p.Geom) return result;

    const float* bmin   = p.Geom->Bounds + 0;
    const float* bmax   = p.Geom->Bounds + 3;
    const float  worldW = bmax[0] - bmin[0];
    const float  worldH = bmax[2] - bmin[2];
    if (worldW <= 0 || worldH <= 0) return result;

    constexpr float padding = 8.0f;
    const float maxW  = std::max(40.0f, panelSize.x - padding * 2.0f);
    const float maxH  = std::max(40.0f, panelSize.y - padding * 2.0f);
    const float scale = std::min(maxW / worldW, maxH / worldH);
    const ImVec2 csz(worldW * scale, worldH * scale);
    const ImVec2 cmin(panelMin.x + (panelSize.x - csz.x) * 0.5f,
                      panelMin.y + (panelSize.y - csz.y) * 0.5f);
    const ImVec2 cmax(cmin.x + csz.x, cmin.y + csz.y);

    // 写入坐标缓存
    result.bValid     = true;
    result.CanvasMin  = cmin;
    result.CanvasSize = csz;
    std::memcpy(result.Bmin, bmin, sizeof(float) * 3);
    std::memcpy(result.Bmax, bmax, sizeof(float) * 3);

    // 像素 / 世界单位（X 方向）
    const float pxPerWorld = csz.x / (bmax[0] - bmin[0]);

    // -------------------------------------------------------------------------
    // 碰撞地面：输入三角网 XZ 投影填充（先于网格，避免盖住辅助线）
    // -------------------------------------------------------------------------
    if (p.View && p.View->bShowCollisionTint && !p.Geom->Triangles.empty())
    {
        const ImU32 col = Renderer2D::ColU32(p.View->GroundCollisionTint);
        // 仅遍历"纯地面"三角，障碍 footprint 由下方 obstacle pass 用障碍色单独绘制
        const int   totalTris  = static_cast<int>(p.Geom->Triangles.size() / 3);
        const int   triCount   = (p.Geom->GroundTriCount >= 0)
                                 ? std::min(p.Geom->GroundTriCount, totalTris)
                                 : totalTris;
        const int   stride     = (triCount > 40000) ? (triCount / 40000 + 1) : 1;
        const std::vector<Obstacle>* obsPtr =
            p.Geom->Obstacles.empty() ? nullptr : &p.Geom->Obstacles;
        for (int t = 0; t < triCount; t += stride)
        {
            const int*   tri = &p.Geom->Triangles[t * 3];
            const float* va  = &p.Geom->Vertices[tri[0] * 3];
            const float* vb  = &p.Geom->Vertices[tri[1] * 3];
            const float* vc  = &p.Geom->Vertices[tri[2] * 3];
            DrawGroundTriFootprint2D(dl, cmin, csz, bmin, bmax, obsPtr, col, va, vb, vc, 0);
        }
    }

    // -------------------------------------------------------------------------
    // 网格辅助线
    // -------------------------------------------------------------------------
    if (p.View && p.View->bShowGrid)
    {
        for (int i = 0; i <= 6; ++i)
        {
            const float t = i / 6.0f;
            const ImVec2 a(cmin.x + t * csz.x, cmin.y);
            const ImVec2 b(cmin.x + t * csz.x, cmax.y);
            const ImVec2 c(cmin.x, cmin.y + t * csz.y);
            const ImVec2 d(cmax.x, cmin.y + t * csz.y);
            dl->AddLine(a, b, IM_COL32(70, 70, 80, 200));
            dl->AddLine(c, d, IM_COL32(70, 70, 80, 200));
        }
    }
    dl->AddRect(cmin, cmax, IM_COL32(120, 120, 130, 255));

    // -------------------------------------------------------------------------
    // 输入几何 XZ 投影线框（仅 OBJ 模式）
    // -------------------------------------------------------------------------
    if (p.View && p.View->bShowInputMesh
        && p.Geom->Source == GeomSource::ObjFile
        && !p.Geom->Triangles.empty())
    {
        const ImU32 inputCol = IM_COL32(180, 200, 220, 140);
        const int triCount = static_cast<int>(p.Geom->Triangles.size() / 3);
        const int stride   = (triCount > 40000) ? (triCount / 40000 + 1) : 1;
        for (int t = 0; t < triCount; t += stride)
        {
            const int*   tri = &p.Geom->Triangles[t * 3];
            const float* va  = &p.Geom->Vertices[tri[0] * 3];
            const float* vb  = &p.Geom->Vertices[tri[1] * 3];
            const float* vc  = &p.Geom->Vertices[tri[2] * 3];
            const ImVec2 pa = World2D(va[0], va[2], cmin, csz, bmin, bmax);
            const ImVec2 pb = World2D(vb[0], vb[2], cmin, csz, bmin, bmax);
            const ImVec2 pc = World2D(vc[0], vc[2], cmin, csz, bmin, bmax);
            dl->AddLine(pa, pb, inputCol);
            dl->AddLine(pb, pc, inputCol);
            dl->AddLine(pc, pa, inputCol);
        }
    }

    // -------------------------------------------------------------------------
    // 障碍（Box / Cylinder）
    // -------------------------------------------------------------------------
    if (p.View && p.View->bShowObstacles)
    {
        for (size_t i = 0; i < p.Geom->Obstacles.size(); ++i)
        {
            const Obstacle& o = p.Geom->Obstacles[i];
            const bool selected = (p.SelectedObstacle == (int)i);
            const bool dragging = (p.MoveStateIndex   == (int)i);
            ImU32      fill;
            ImU32      edge;
            if (p.View->bShowCollisionTint)
            {
                ImVec4 fc = p.View->ObstacleTopTint;
                if (dragging) fc.w = std::min(1.0f, fc.w * 1.12f + 0.06f);
                else if (selected) fc.w = std::min(1.0f, fc.w * 1.06f + 0.04f);
                fill = ColU32(fc);
                edge = selected ? IM_COL32(255, 235, 120, 255) : ColU32(p.View->ObstacleCollisionEdge);
            }
            else
            {
                fill = IM_COL32(220, 105, 95, dragging ? 245 : (selected ? 225 : 195));
                edge = selected ? IM_COL32(255, 235, 120, 255) : IM_COL32(255, 150, 130, 255);
            }
            const float lw   = selected ? 2.0f : 1.0f;
            if (o.Shape == ObstacleShape::Box)
            {
                const ImVec2 p0 = World2D(o.CX - o.SX, o.CZ - o.SZ, cmin, csz, bmin, bmax);
                const ImVec2 p1 = World2D(o.CX + o.SX, o.CZ + o.SZ, cmin, csz, bmin, bmax);
                const ImVec2 a(std::min(p0.x, p1.x), std::min(p0.y, p1.y));
                const ImVec2 b(std::max(p0.x, p1.x), std::max(p0.y, p1.y));
                dl->AddRectFilled(a, b, fill);
                dl->AddRect(a, b, edge, 0.0f, 0, lw);
            }
            else
            {
                const ImVec2 c = World2D(o.CX, o.CZ, cmin, csz, bmin, bmax);
                const float  r = o.Radius * pxPerWorld;
                dl->AddCircleFilled(c, r, fill, 24);
                dl->AddCircle(c, r, edge, 24, lw);
            }
        }
    }

    // -------------------------------------------------------------------------
    // Build Volume（自定义生成区域）— 橙色矩形轮廓（XZ 投影）
    // -------------------------------------------------------------------------
    if (p.BV && p.BV->bActive && p.View && p.View->bShowBuildVolume)
    {
        const ImVec2 p0 = World2D(p.BV->Min[0], p.BV->Min[2], cmin, csz, bmin, bmax);
        const ImVec2 p1 = World2D(p.BV->Max[0], p.BV->Max[2], cmin, csz, bmin, bmax);
        const ImVec2 lo(std::min(p0.x, p1.x), std::min(p0.y, p1.y));
        const ImVec2 hi(std::max(p0.x, p1.x), std::max(p0.y, p1.y));
        dl->AddRectFilled(lo, hi, IM_COL32(255, 160, 40, 18));
        dl->AddRect(lo, hi, IM_COL32(255, 160, 40, 230), 0.0f, 0, 2.0f);
    }

    // -------------------------------------------------------------------------
    // PolyMesh
    // -------------------------------------------------------------------------
    if (p.Nav && p.Nav->bBuilt && p.Nav->PolyMesh
        && p.View && (p.View->bFillPolygons || p.View->bShowPolyEdges))
    {
        const rcPolyMesh* pm  = p.Nav->PolyMesh;
        const int         nvp = pm->nvp;
        const float       cs  = pm->cs;

        for (int i = 0; i < pm->npolys; ++i)
        {
            const unsigned short* poly = &pm->polys[i * nvp * 2];
            std::vector<ImVec2> pts;
            pts.reserve(nvp);
            for (int j = 0; j < nvp; ++j)
            {
                if (poly[j] == RC_MESH_NULL_IDX) break;
                const unsigned short* v  = &pm->verts[poly[j] * 3];
                const float           wx = pm->bmin[0] + v[0] * cs;
                const float           wz = pm->bmin[2] + v[2] * cs;
                pts.push_back(World2D(wx, wz, cmin, csz, bmin, bmax));
            }
            if (pts.size() < 3) continue;

            if (p.View->bFillPolygons)
            {
                const ImU32 col = p.View->bRegionColors
                    ? HashColor(pm->regs ? pm->regs[i] : (unsigned)i, 0.45f)
                    : IM_COL32(80, 200, 100, 90);
                for (size_t k = 1; k + 1 < pts.size(); ++k)
                    dl->AddTriangleFilled(pts[0], pts[k], pts[k + 1], col);
            }
            if (p.View->bShowPolyEdges)
            {
                for (size_t k = 0; k < pts.size(); ++k)
                    dl->AddLine(pts[k], pts[(k + 1) % pts.size()], IM_COL32(80, 200, 100, 230));
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
            const unsigned int vertBase  = dm->meshes[m * 4 + 0];
            const unsigned int triBase   = dm->meshes[m * 4 + 2];
            const unsigned int triCount  = dm->meshes[m * 4 + 3];
            for (unsigned int i = 0; i < triCount; ++i)
            {
                const unsigned char* t = &dm->tris[(triBase + i) * 4];
                ImVec2 verts[3];
                for (int k = 0; k < 3; ++k)
                {
                    const float* v = &dm->verts[(vertBase + t[k]) * 3];
                    verts[k] = World2D(v[0], v[2], cmin, csz, bmin, bmax);
                }
                dl->AddLine(verts[0], verts[1], IM_COL32(255, 255, 255, 60));
                dl->AddLine(verts[1], verts[2], IM_COL32(255, 255, 255, 60));
                dl->AddLine(verts[2], verts[0], IM_COL32(255, 255, 255, 60));
            }
        }
    }

    // -------------------------------------------------------------------------
    // 路径走廊高亮 + 直线路径
    // -------------------------------------------------------------------------
    const ImU32 pathColU32     = ColU32(p.PathColor);
    const ImU32 corridorColU32 = ColU32(p.CorridorColor);

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
            std::vector<ImVec2> pts;
            pts.reserve(poly->vertCount);
            for (int j = 0; j < poly->vertCount; ++j)
            {
                const float* v = &tile->verts[poly->verts[j] * 3];
                pts.push_back(World2D(v[0], v[2], cmin, csz, bmin, bmax));
            }
            if (pts.size() < 3) continue;
            for (size_t k = 1; k + 1 < pts.size(); ++k)
                dl->AddTriangleFilled(pts[0], pts[k], pts[k + 1], corridorColU32);
        }
    }

    // 若平滑路径可用且开启，则优先绘制平滑路径；否则绘制原始直线路径
    const std::vector<float>* pathToDraw =
        (p.View && p.View->bSmoothPath && p.SmoothedPath && p.SmoothedPath->size() >= 6)
        ? p.SmoothedPath
        : p.StraightPath;

    if (pathToDraw && pathToDraw->size() >= 6)
    {
        const size_t n = pathToDraw->size() / 3;
        for (size_t i = 0; i + 1 < n; ++i)
        {
            const ImVec2 a = World2D((*pathToDraw)[i * 3 + 0],
                                     (*pathToDraw)[i * 3 + 2],
                                     cmin, csz, bmin, bmax);
            const ImVec2 b = World2D((*pathToDraw)[(i + 1) * 3 + 0],
                                     (*pathToDraw)[(i + 1) * 3 + 2],
                                     cmin, csz, bmin, bmax);
            dl->AddLine(a, b, pathColU32, 2.5f);
        }
    }

    // -------------------------------------------------------------------------
    // OffMeshLink（彩色两端圆 + 连线）
    // -------------------------------------------------------------------------
    if (p.Geom && !p.Geom->OffMeshLinks.empty() && p.View && p.View->bShowObstacles)
    {
        constexpr ImU32 colStart = IM_COL32(220, 180,  60, 255);
        constexpr ImU32 colEnd   = IM_COL32( 60, 220, 180, 255);
        constexpr ImU32 colLine  = IM_COL32(220, 220,  80, 200);
        for (const OffMeshLink& lk : p.Geom->OffMeshLinks)
        {
            const ImVec2 sp = World2D(lk.Start[0], lk.Start[2], cmin, csz, bmin, bmax);
            const ImVec2 ep = World2D(lk.End  [0], lk.End  [2], cmin, csz, bmin, bmax);
            const float  rp = std::max(3.0f, lk.Radius * pxPerWorld);
            dl->AddLine(sp, ep, colLine, 2.0f);
            dl->AddCircleFilled(sp, rp, IM_COL32(220, 180, 60, 70), 16);
            dl->AddCircle(sp, rp, colStart, 16, 2.0f);
            dl->AddCircleFilled(ep, rp, IM_COL32( 60, 220, 180, 70), 16);
            dl->AddCircle(ep, rp, colEnd,   16, 2.0f);
        }
    }

    // -------------------------------------------------------------------------
    // AutoNavLinks（自动生成的边界 NavLink，紫色 / 品红色）
    // -------------------------------------------------------------------------
    if (p.AutoNavLinks && !p.AutoNavLinks->empty() && p.View && p.View->bShowAutoNavLinks)
    {
        constexpr ImU32 colBidir  = IM_COL32(200,  80, 255, 255);  // 品红（双向）
        constexpr ImU32 colOneWay = IM_COL32(255,  80, 200, 255);  // 洋红（单向）
        constexpr ImU32 colLineB  = IM_COL32(180,  60, 240, 200);
        constexpr ImU32 colLineO  = IM_COL32(240,  60, 180, 200);

        for (const OffMeshLink& lk : *p.AutoNavLinks)
        {
            const ImVec2 sp = World2D(lk.Start[0], lk.Start[2], cmin, csz, bmin, bmax);
            const ImVec2 ep = World2D(lk.End  [0], lk.End  [2], cmin, csz, bmin, bmax);
            const float  rp = std::max(2.5f, lk.Radius * pxPerWorld * 0.7f);
            const bool   bidir = (lk.Dir == 1);
            const ImU32  colEnd1 = bidir ? colBidir : colOneWay;
            const ImU32  colLn   = bidir ? colLineB : colLineO;

            dl->AddLine(sp, ep, colLn, 1.5f);

            // 起点小圆
            dl->AddCircleFilled(sp, rp, bidir ? IM_COL32(200, 80, 255, 50) : IM_COL32(255, 80, 200, 50), 12);
            dl->AddCircle(sp, rp, colBidir, 12, 1.5f);

            // 终点小圆
            dl->AddCircleFilled(ep, rp, bidir ? IM_COL32(200, 80, 255, 50) : IM_COL32(255, 80, 200, 50), 12);
            dl->AddCircle(ep, rp, colEnd1, 12, 1.5f);

            // 单向箭头：在线段中点附近画小三角
            if (!bidir)
            {
                const float dx = ep.x - sp.x;
                const float dy = ep.y - sp.y;
                const float len = std::sqrt(dx * dx + dy * dy);
                if (len > 4.0f)
                {
                    // 箭头放在 60% 处
                    const ImVec2 mid(sp.x + dx * 0.6f, sp.y + dy * 0.6f);
                    const float nx = -dy / len;
                    const float ny =  dx / len;
                    const float aw = 4.5f;
                    const float ah = 7.0f;
                    const ImVec2 tip(mid.x + (dx / len) * ah, mid.y + (dy / len) * ah);
                    const ImVec2 bl (mid.x + nx * aw, mid.y + ny * aw);
                    const ImVec2 br (mid.x - nx * aw, mid.y - ny * aw);
                    dl->AddTriangleFilled(tip, bl, br, colOneWay);
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // 起/终点（胶囊体在俯视图退化为圆）
    // -------------------------------------------------------------------------
    if (p.Start && p.End)
    {
        const ImVec2 sp   = World2D(p.Start[0], p.Start[2], cmin, csz, bmin, bmax);
        const ImVec2 ep   = World2D(p.End[0],   p.End[2],   cmin, csz, bmin, bmax);
        const float  capR = std::max(2.0f, p.AgentRadius * pxPerWorld);
        const ImU32  startCol  = IM_COL32( 80, 160, 255, 255);
        const ImU32  endCol    = IM_COL32(255,  80,  80, 255);
        const ImU32  startFill = IM_COL32( 80, 160, 255,  90);
        const ImU32  endFill   = IM_COL32(255,  80,  80,  90);
        dl->AddCircleFilled(sp, capR, startFill, 24);
        dl->AddCircle(sp, capR, startCol, 24, 2.0f);
        dl->AddCircleFilled(sp, 3.0f, startCol);
        dl->AddCircleFilled(ep, capR, endFill,   24);
        dl->AddCircle(ep, capR, endCol,   24, 2.0f);
        dl->AddCircleFilled(ep, 3.0f, endCol);
    }

    // -------------------------------------------------------------------------
    // 障碍创建草稿（Box 矩形 / Cylinder 圆）
    // -------------------------------------------------------------------------
    if (p.CurrentEditMode == EditMode::CreateBox
        && p.CreateDraft && p.CreateDraft->bActive)
    {
        const ImVec2 a = World2D(p.CreateDraft->StartX, p.CreateDraft->StartZ,
                                 cmin, csz, bmin, bmax);
        const ImVec2 b = World2D(p.CreateDraft->CurX,   p.CreateDraft->CurZ,
                                 cmin, csz, bmin, bmax);
        if (p.DefaultObstacleShape == ObstacleShape::Box)
        {
            const ImVec2 lo(std::min(a.x, b.x), std::min(a.y, b.y));
            const ImVec2 hi(std::max(a.x, b.x), std::max(a.y, b.y));
            dl->AddRectFilled(lo, hi, IM_COL32(255, 230, 100, 80));
            dl->AddRect(lo, hi, IM_COL32(255, 230, 100, 230), 0.0f, 0, 2.0f);
        }
        else
        {
            const float dx = (p.CreateDraft->CurX - p.CreateDraft->StartX);
            const float dz = (p.CreateDraft->CurZ - p.CreateDraft->StartZ);
            const float rw = std::sqrt(dx * dx + dz * dz);
            const float rp = std::max(2.0f, rw * pxPerWorld);
            dl->AddCircleFilled(a, rp, IM_COL32(255, 230, 100, 80), 24);
            dl->AddCircle(a, rp, IM_COL32(255, 230, 100, 230), 24, 2.0f);
        }
    }

    return result;
}

} // namespace Renderer2D
