/*
 * Nav/NavStepBuilder.cpp
 * ----------------------
 * 分步骤构建实现。语义上与 NavBuilder::BuildNavMesh 完全等价 ——
 * 仅把流水线拆分为 10 个独立可调用的步骤，并把中间数据保留下来供可视化使用。
 */

#include "NavStepBuilder.h"
#include "NavBuilder.h"

#include <chrono>
#include <cmath>
#include <cstring>

// Recast
#include "Recast.h"
// Detour
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "DetourNavMeshQuery.h"
#include "DetourStatus.h"

namespace NavStepBuilder
{

// =============================================================================
// 步骤元信息
// =============================================================================
const char* GetStepName(Step s)
{
    switch (s)
    {
        case Step::None:       return "(idle)";
        case Step::Config:     return "1. Config";
        case Step::Rasterize:  return "2. Rasterize";
        case Step::Filter:     return "3. Filter Walkable";
        case Step::CompactHF:  return "4. Compact HF";
        case Step::Erode:      return "5. Erode + DistField";
        case Step::Regions:    return "6. Build Regions";
        case Step::Contours:   return "7. Build Contours";
        case Step::PolyMesh:   return "8. Build PolyMesh";
        case Step::DetailMesh: return "9. Build DetailMesh";
        case Step::DetourNav:  return "10. dtNavMesh";
    }
    return "?";
}

const char* GetStepDescription(Step s)
{
    switch (s)
    {
        case Step::None:       return "未开始 - 点击 Step Forward 进入第一步";
        case Step::Config:     return "填充 rcConfig (cell size / agent / region 阈值等)";
        case Step::Rasterize:  return "创建体素高度场 + 光栅化三角形 (按 slope 标记可走)";
        case Step::Filter:     return "过滤低悬挂 / 台阶 / 低矮净空 (修改 solid 标记)";
        case Step::CompactHF:  return "紧凑高度场 (释放 solid, 后续步骤基于 chf)";
        case Step::Erode:      return "按 agent 半径腐蚀 + 计算距离场 (chf->dist)";
        case Step::Regions:    return "Watershed 区域分割 (chf->reg, 颜色编号)";
        case Step::Contours:   return "提取区域轮廓 (rcContourSet, 简化后多边形)";
        case Step::PolyMesh:   return "三角化轮廓为凸多边形网格 (rcPolyMesh)";
        case Step::DetailMesh: return "细节网格 (rcPolyMeshDetail, chf 释放)";
        case Step::DetourNav:  return "构建 dtNavMesh + dtNavMeshQuery (流水线完成)";
    }
    return "";
}

// =============================================================================
// 内部辅助
// =============================================================================
namespace
{

void FreeIntermediate(StepBuilder& sb)
{
    if (sb.Solid) { rcFreeHeightField(sb.Solid);          sb.Solid = nullptr; }
    if (sb.Chf)   { rcFreeCompactHeightfield(sb.Chf);     sb.Chf   = nullptr; }
    if (sb.Cset)  { rcFreeContourSet(sb.Cset);            sb.Cset  = nullptr; }
    if (sb.Pmesh) { rcFreePolyMesh(sb.Pmesh);             sb.Pmesh = nullptr; }
    if (sb.Dmesh) { rcFreePolyMeshDetail(sb.Dmesh);       sb.Dmesh = nullptr; }
}

// ---- 各步骤实现：返回 true 成功；失败时把信息写入 sb.FailMsg/Ctx ----

bool DoStep1Config(StepBuilder& sb)
{
    rcConfig& cfg = sb.Cfg;
    std::memset(&cfg, 0, sizeof(cfg));
    const NavBuildConfig& cfg_in = sb.ConfigSnap;

    cfg.cs                     = cfg_in.CellSize;
    cfg.ch                     = cfg_in.CellHeight;
    cfg.walkableSlopeAngle     = cfg_in.AgentMaxSlope;
    cfg.walkableHeight         = static_cast<int>(std::ceil (cfg_in.AgentHeight   / cfg.ch));
    cfg.walkableClimb          = static_cast<int>(std::floor(cfg_in.AgentMaxClimb / cfg.ch));
    cfg.walkableRadius         = static_cast<int>(std::ceil (cfg_in.AgentRadius   / cfg.cs));
    cfg.maxEdgeLen             = static_cast<int>(cfg_in.EdgeMaxLen / cfg.cs);
    cfg.maxSimplificationError = cfg_in.EdgeMaxError;
    cfg.minRegionArea          = static_cast<int>(rcSqr(cfg_in.RegionMinSize));
    cfg.mergeRegionArea        = static_cast<int>(rcSqr(cfg_in.RegionMergeSize));
    cfg.maxVertsPerPoly        = cfg_in.VertsPerPoly;
    cfg.detailSampleDist       = cfg_in.DetailSampleDist < 0.9f
                                   ? 0.0f
                                   : cfg_in.CellSize * cfg_in.DetailSampleDist;
    cfg.detailSampleMaxError   = cfg_in.CellHeight * cfg_in.DetailSampleMaxError;

    if (sb.BVSnap.bActive)
    {
        rcVcopy(cfg.bmin, sb.BVSnap.Min);
        rcVcopy(cfg.bmax, sb.BVSnap.Max);
    }
    else
    {
        rcVcopy(cfg.bmin, &sb.GeomSnap.Bounds[0]);
        rcVcopy(cfg.bmax, &sb.GeomSnap.Bounds[3]);
    }
    rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);
    sb.Ctx.log(RC_LOG_PROGRESS, "Step1: rcConfig OK");
    return true;
}

bool DoStep2Rasterize(StepBuilder& sb)
{
    if (sb.Solid) { rcFreeHeightField(sb.Solid); sb.Solid = nullptr; }

    sb.Solid = rcAllocHeightfield();
    if (!sb.Solid ||
        !rcCreateHeightfield(&sb.Ctx, *sb.Solid,
                             sb.Cfg.width, sb.Cfg.height,
                             sb.Cfg.bmin, sb.Cfg.bmax,
                             sb.Cfg.cs, sb.Cfg.ch))
    {
        sb.FailMsg = "rcCreateHeightfield failed";
        return false;
    }

    const InputGeometry& geom = sb.GeomSnap;
    const int nverts = static_cast<int>(geom.Vertices.size() / 3);
    const int ntris  = static_cast<int>(geom.Triangles.size() / 3);

    std::vector<unsigned char> triareas = geom.AreaTypes;
    triareas.resize(ntris, RC_NULL_AREA);
    rcClearUnwalkableTriangles(&sb.Ctx,
                               sb.Cfg.walkableSlopeAngle,
                               geom.Vertices.data(), nverts,
                               geom.Triangles.data(), ntris,
                               triareas.data());

    {
        ScopedTimer st(&sb.Timings.RasterizeMs);
        if (!rcRasterizeTriangles(&sb.Ctx,
                                  geom.Vertices.data(), nverts,
                                  geom.Triangles.data(), triareas.data(),
                                  ntris, *sb.Solid, sb.Cfg.walkableClimb))
        {
            sb.FailMsg = "rcRasterizeTriangles failed";
            return false;
        }
    }
    return true;
}

bool DoStep3Filter(StepBuilder& sb)
{
    if (!sb.Solid) { sb.FailMsg = "no heightfield (run step 2 first)"; return false; }
    ScopedTimer st(&sb.Timings.FilterMs);
    rcFilterLowHangingWalkableObstacles(&sb.Ctx, sb.Cfg.walkableClimb, *sb.Solid);
    rcFilterLedgeSpans                  (&sb.Ctx, sb.Cfg.walkableHeight, sb.Cfg.walkableClimb, *sb.Solid);
    rcFilterWalkableLowHeightSpans      (&sb.Ctx, sb.Cfg.walkableHeight, *sb.Solid);
    return true;
}

bool DoStep4CompactHF(StepBuilder& sb)
{
    if (!sb.Solid) { sb.FailMsg = "no heightfield (run step 2 first)"; return false; }
    if (sb.Chf) { rcFreeCompactHeightfield(sb.Chf); sb.Chf = nullptr; }

    sb.Chf = rcAllocCompactHeightfield();
    {
        ScopedTimer st(&sb.Timings.CompactMs);
        if (!sb.Chf ||
            !rcBuildCompactHeightfield(&sb.Ctx,
                                       sb.Cfg.walkableHeight, sb.Cfg.walkableClimb,
                                       *sb.Solid, *sb.Chf))
        {
            sb.FailMsg = "rcBuildCompactHeightfield failed";
            return false;
        }
    }
    rcFreeHeightField(sb.Solid); sb.Solid = nullptr;
    return true;
}

bool DoStep5ErodeAndDistField(StepBuilder& sb)
{
    if (!sb.Chf) { sb.FailMsg = "no compact heightfield (run step 4 first)"; return false; }
    {
        ScopedTimer st(&sb.Timings.ErodeMs);
        if (!rcErodeWalkableArea(&sb.Ctx, sb.Cfg.walkableRadius, *sb.Chf))
        {
            sb.FailMsg = "rcErodeWalkableArea failed";
            return false;
        }
    }
    {
        ScopedTimer st(&sb.Timings.DistFieldMs);
        if (!rcBuildDistanceField(&sb.Ctx, *sb.Chf))
        {
            sb.FailMsg = "rcBuildDistanceField failed";
            return false;
        }
    }
    return true;
}

bool DoStep6Regions(StepBuilder& sb)
{
    if (!sb.Chf) { sb.FailMsg = "no compact heightfield"; return false; }
    ScopedTimer st(&sb.Timings.RegionsMs);
    if (!rcBuildRegions(&sb.Ctx, *sb.Chf, 0,
                        sb.Cfg.minRegionArea, sb.Cfg.mergeRegionArea))
    {
        sb.FailMsg = "rcBuildRegions failed";
        return false;
    }
    return true;
}

bool DoStep7Contours(StepBuilder& sb)
{
    if (!sb.Chf) { sb.FailMsg = "no compact heightfield"; return false; }
    if (sb.Cset) { rcFreeContourSet(sb.Cset); sb.Cset = nullptr; }

    sb.Cset = rcAllocContourSet();
    ScopedTimer st(&sb.Timings.ContoursMs);
    if (!sb.Cset ||
        !rcBuildContours(&sb.Ctx, *sb.Chf,
                         sb.Cfg.maxSimplificationError,
                         sb.Cfg.maxEdgeLen, *sb.Cset))
    {
        sb.FailMsg = "rcBuildContours failed";
        return false;
    }
    return true;
}

bool DoStep8PolyMesh(StepBuilder& sb)
{
    if (!sb.Cset) { sb.FailMsg = "no contour set"; return false; }
    if (sb.Pmesh) { rcFreePolyMesh(sb.Pmesh); sb.Pmesh = nullptr; }

    sb.Pmesh = rcAllocPolyMesh();
    ScopedTimer st(&sb.Timings.PolyMeshMs);
    if (!sb.Pmesh ||
        !rcBuildPolyMesh(&sb.Ctx, *sb.Cset, sb.Cfg.maxVertsPerPoly, *sb.Pmesh))
    {
        sb.FailMsg = "rcBuildPolyMesh failed";
        return false;
    }
    // Cset 完成使命，但保留以便用户回退查看 — 这里不释放
    return true;
}

bool DoStep9DetailMesh(StepBuilder& sb)
{
    if (!sb.Pmesh) { sb.FailMsg = "no poly mesh"; return false; }
    if (!sb.Chf)   { sb.FailMsg = "compact heightfield was already freed"; return false; }
    if (sb.Dmesh) { rcFreePolyMeshDetail(sb.Dmesh); sb.Dmesh = nullptr; }

    sb.Dmesh = rcAllocPolyMeshDetail();
    ScopedTimer st(&sb.Timings.DetailMeshMs);
    if (!sb.Dmesh ||
        !rcBuildPolyMeshDetail(&sb.Ctx, *sb.Pmesh, *sb.Chf,
                               sb.Cfg.detailSampleDist,
                               sb.Cfg.detailSampleMaxError,
                               *sb.Dmesh))
    {
        sb.FailMsg = "rcBuildPolyMeshDetail failed";
        return false;
    }
    return true;
}

bool DoStep10DetourNavMesh(StepBuilder&                    sb,
                           NavRuntime&                     runtime,
                           const std::vector<OffMeshLink>* extraLinks)
{
    if (!sb.Pmesh || !sb.Dmesh) { sb.FailMsg = "missing PolyMesh / DetailMesh"; return false; }

    rcPolyMesh*       pm  = sb.Pmesh;
    rcPolyMeshDetail* pmd = sb.Dmesh;

    for (int i = 0; i < pm->npolys; ++i)
        if (pm->areas[i] == RC_WALKABLE_AREA) pm->flags[i] = 0x01;

    const int geomCount  = static_cast<int>(sb.GeomSnap.OffMeshLinks.size());
    const int extraCount = extraLinks ? static_cast<int>(extraLinks->size()) : 0;
    const int linkCount  = geomCount + extraCount;
    std::vector<float>          omVerts(linkCount * 6);
    std::vector<float>          omRad  (linkCount);
    std::vector<unsigned short> omFlags(linkCount);
    std::vector<unsigned char>  omAreas(linkCount);
    std::vector<unsigned char>  omDirs (linkCount);
    for (int i = 0; i < geomCount; ++i)
    {
        const OffMeshLink& lk = sb.GeomSnap.OffMeshLinks[i];
        std::memcpy(&omVerts[i * 6 + 0], lk.Start, sizeof(float) * 3);
        std::memcpy(&omVerts[i * 6 + 3], lk.End,   sizeof(float) * 3);
        omRad[i]   = lk.Radius; omFlags[i] = lk.Flags;
        omAreas[i] = lk.Area;   omDirs [i] = lk.Dir;
    }
    for (int i = 0; i < extraCount; ++i)
    {
        const OffMeshLink& lk = (*extraLinks)[i];
        const int idx = geomCount + i;
        std::memcpy(&omVerts[idx * 6 + 0], lk.Start, sizeof(float) * 3);
        std::memcpy(&omVerts[idx * 6 + 3], lk.End,   sizeof(float) * 3);
        omRad[idx]   = lk.Radius; omFlags[idx] = lk.Flags;
        omAreas[idx] = lk.Area;   omDirs [idx] = lk.Dir;
    }

    dtNavMeshCreateParams params{};
    params.verts            = pm->verts;
    params.vertCount        = pm->nverts;
    params.polys            = pm->polys;
    params.polyAreas        = pm->areas;
    params.polyFlags        = pm->flags;
    params.polyCount        = pm->npolys;
    params.nvp              = pm->nvp;
    params.detailMeshes     = pmd->meshes;
    params.detailVerts      = pmd->verts;
    params.detailVertsCount = pmd->nverts;
    params.detailTris       = pmd->tris;
    params.detailTriCount   = pmd->ntris;
    params.walkableHeight   = sb.ConfigSnap.AgentHeight;
    params.walkableRadius   = sb.ConfigSnap.AgentRadius;
    params.walkableClimb    = sb.ConfigSnap.AgentMaxClimb;
    rcVcopy(params.bmin, pm->bmin);
    rcVcopy(params.bmax, pm->bmax);
    params.cs               = sb.Cfg.cs;
    params.ch               = sb.Cfg.ch;
    params.buildBvTree      = true;
    if (linkCount > 0)
    {
        params.offMeshConVerts = omVerts.data();
        params.offMeshConRad   = omRad.data();
        params.offMeshConFlags = omFlags.data();
        params.offMeshConAreas = omAreas.data();
        params.offMeshConDir   = omDirs.data();
        params.offMeshConCount = linkCount;
    }

    unsigned char* navData     = nullptr;
    int            navDataSize = 0;
    {
        ScopedTimer st(&sb.Timings.DetourCreateMs);
        if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
        {
            sb.FailMsg = "dtCreateNavMeshData failed";
            return false;
        }
    }

    // 把结果转交给 NavRuntime
    NavBuilder::DestroyNavRuntime(runtime);

    runtime.NavMeshData.assign(navData, navData + navDataSize);
    runtime.NavMesh = dtAllocNavMesh();
    if (!runtime.NavMesh)
    {
        dtFree(navData);
        sb.FailMsg = "dtAllocNavMesh failed";
        return false;
    }
    if (dtStatusFailed(runtime.NavMesh->init(navData, navDataSize, DT_TILE_FREE_DATA)))
    {
        sb.FailMsg = "dtNavMesh::init failed";
        return false;
    }
    runtime.NavQuery = dtAllocNavMeshQuery();
    if (!runtime.NavQuery)
    {
        sb.FailMsg = "dtAllocNavMeshQuery failed";
        return false;
    }
    if (dtStatusFailed(runtime.NavQuery->init(runtime.NavMesh, 2048)))
    {
        sb.FailMsg = "dtNavMeshQuery::init failed";
        return false;
    }

    // PolyMesh / DetailMesh 移交所有权（避免双释放）
    runtime.PolyMesh       = sb.Pmesh; sb.Pmesh = nullptr;
    runtime.PolyMeshDetail = sb.Dmesh; sb.Dmesh = nullptr;
    runtime.bBuilt         = true;
    runtime.bTileMode      = false;

    // 把 Step 阶段累计的日志合并进 runtime
    runtime.Ctx.LogLines.insert(runtime.Ctx.LogLines.end(),
                                sb.Ctx.LogLines.begin(),
                                sb.Ctx.LogLines.end());

    return true;
}

bool DispatchStep(Step                            target,
                  StepBuilder&                    sb,
                  NavRuntime&                     runtime,
                  const std::vector<OffMeshLink>* extraLinks)
{
    sb.bFailed = false;
    sb.FailMsg.clear();

    bool ok = false;
    switch (target)
    {
        case Step::Config:     ok = DoStep1Config(sb);                     break;
        case Step::Rasterize:  ok = DoStep2Rasterize(sb);                  break;
        case Step::Filter:     ok = DoStep3Filter(sb);                     break;
        case Step::CompactHF:  ok = DoStep4CompactHF(sb);                  break;
        case Step::Erode:      ok = DoStep5ErodeAndDistField(sb);          break;
        case Step::Regions:    ok = DoStep6Regions(sb);                    break;
        case Step::Contours:   ok = DoStep7Contours(sb);                   break;
        case Step::PolyMesh:   ok = DoStep8PolyMesh(sb);                   break;
        case Step::DetailMesh: ok = DoStep9DetailMesh(sb);                 break;
        case Step::DetourNav:  ok = DoStep10DetourNavMesh(sb, runtime, extraLinks); break;
        default:               sb.FailMsg = "invalid target step";         break;
    }

    if (ok)
    {
        sb.LastCompleted = target;
        char buf[80];
        std::snprintf(buf, sizeof(buf), "Step %d (%s) OK", static_cast<int>(target), GetStepName(target));
        sb.Ctx.log(RC_LOG_PROGRESS, buf);
    }
    else
    {
        sb.bFailed = true;
        if (sb.FailMsg.empty()) sb.FailMsg = "step failed";
        char buf[160];
        std::snprintf(buf, sizeof(buf), "Step %d (%s) FAILED: %s",
                      static_cast<int>(target), GetStepName(target), sb.FailMsg.c_str());
        sb.Ctx.log(RC_LOG_ERROR, buf);
    }
    return ok;
}

} // anonymous namespace

// =============================================================================
// 公开 API
// =============================================================================
bool IsActive(const StepBuilder& sb)
{
    return sb.LastCompleted != Step::None ||
           sb.Solid || sb.Chf || sb.Cset || sb.Pmesh || sb.Dmesh;
}

Step PeekNextStep(const StepBuilder& sb)
{
    const int next = static_cast<int>(sb.LastCompleted) + 1;
    if (next > kStepCount) return Step::None;
    return static_cast<Step>(next);
}

void Clear(StepBuilder& sb)
{
    FreeIntermediate(sb);
    sb.LastCompleted = Step::None;
    sb.bFailed       = false;
    sb.FailMsg.clear();
    sb.Timings       = PhaseTimings{};
}

void Reset(StepBuilder&            sb,
           NavRuntime&             runtime,
           const InputGeometry&    geom,
           const NavBuildConfig&   cfg,
           const BuildVolume&      bv)
{
    NavBuilder::DestroyNavRuntime(runtime);
    Clear(sb);
    sb.Ctx.LogLines.clear();
    sb.GeomSnap   = geom;
    sb.ConfigSnap = cfg;
    sb.BVSnap     = bv;
    sb.Ctx.log(RC_LOG_PROGRESS, "StepBuilder: session reset");
}

bool Forward(StepBuilder&                    sb,
             NavRuntime&                     runtime,
             const std::vector<OffMeshLink>* extraLinks)
{
    if (sb.ConfigSnap.bUseTileCache)
    {
        sb.bFailed = true;
        sb.FailMsg = "TileCache mode does not support stepwise build (use one-shot Build instead)";
        sb.Ctx.log(RC_LOG_ERROR, sb.FailMsg.c_str());
        return false;
    }
    if (sb.GeomSnap.Vertices.empty() || sb.GeomSnap.Triangles.empty())
    {
        sb.bFailed = true;
        sb.FailMsg = "input geometry is empty";
        return false;
    }
    const Step nxt = PeekNextStep(sb);
    if (nxt == Step::None) return false;  // 已完成或非法
    return DispatchStep(nxt, sb, runtime, extraLinks);
}

bool Back(StepBuilder&                       sb,
          NavRuntime&                        runtime,
          const std::vector<OffMeshLink>*    extraLinks)
{
    if (sb.LastCompleted == Step::None) return false;

    const int target = static_cast<int>(sb.LastCompleted) - 1;

    // 重放策略：销毁所有中间数据并重新执行 [Step1 .. target]。
    // Step10 会销毁 NavRuntime；其它步骤不影响。这里统一先销毁 NavRuntime
    // 以保证回到 < 10 的步骤时 NavRuntime 已是干净状态。
    NavBuilder::DestroyNavRuntime(runtime);
    FreeIntermediate(sb);
    sb.LastCompleted = Step::None;
    sb.bFailed       = false;
    sb.FailMsg.clear();
    sb.Timings       = PhaseTimings{};
    // 保留日志: Reset 不再调用，让用户能看到历史

    for (int i = 1; i <= target; ++i)
    {
        if (!DispatchStep(static_cast<Step>(i), sb, runtime, extraLinks))
            return false;
    }
    return true;
}

bool RunAll(StepBuilder&                    sb,
            NavRuntime&                     runtime,
            const std::vector<OffMeshLink>* extraLinks)
{
    while (PeekNextStep(sb) != Step::None)
    {
        if (!Forward(sb, runtime, extraLinks)) return false;
    }
    return true;
}

} // namespace NavStepBuilder
