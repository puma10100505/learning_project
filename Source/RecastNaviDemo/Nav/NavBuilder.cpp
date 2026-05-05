/*
 * Nav/NavBuilder.cpp
 * ------------------
 * NavMesh 构建实现（Recast/Detour 10 步流水线）。
 *
 * Step1~Step6、Step10 为私有静态辅助函数。
 * Step7~Step9 因为 CHF 生命周期跨越多步，直接内联在 BuildNavMesh() 中，
 * 以保证 rcBuildPolyMeshDetail（Step9）在 CHF 被释放之前执行。
 */

#include "NavBuilder.h"
#include "TileCacheSupport.h"

#include <chrono>
#include <cmath>
#include <cstring>
#include <vector>

// Recast
#include "Recast.h"
// Detour
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "DetourNavMeshQuery.h"
#include "DetourStatus.h"
// TileCache
#include "DetourCommon.h"
#include "DetourTileCache.h"
#include "DetourTileCacheBuilder.h"

namespace NavBuilder
{

// Forward declaration for TileCache build path (defined later in this file)
static bool BuildNavMeshTileCachePath(
    const InputGeometry&  geom,
    const NavBuildConfig& config,
    const BuildVolume&    bv,
    NavRuntime&           runtime,
    PhaseTimings&         timings);

// =============================================================================
// Step 1：根据 NavBuildConfig 和几何包围盒填充 rcConfig
// =============================================================================

static bool Step1_ConfigureRcConfig(const InputGeometry&  geom,
                                    const NavBuildConfig& cfg_in,
                                    const BuildVolume&    bv,
                                    rcConfig&             cfg)
{
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

    // 若 BuildVolume 激活则用自定义 AABB，否则使用几何包围盒
    if (bv.bActive)
    {
        rcVcopy(cfg.bmin, bv.Min);
        rcVcopy(cfg.bmax, bv.Max);
    }
    else
    {
        rcVcopy(cfg.bmin, &geom.Bounds[0]);
        rcVcopy(cfg.bmax, &geom.Bounds[3]);
    }
    rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

    return true;
}

// =============================================================================
// Step 2：创建高度场并光栅化三角形
// =============================================================================

static bool Step2_RasterizeTriangles(const InputGeometry& geom,
                                     const rcConfig&      cfg,
                                     NavRuntime&          runtime,
                                     PhaseTimings&        timings,
                                     rcHeightfield*&      solid)
{
    solid = rcAllocHeightfield();
    if (!solid ||
        !rcCreateHeightfield(&runtime.Ctx, *solid,
                             cfg.width, cfg.height,
                             cfg.bmin, cfg.bmax,
                             cfg.cs, cfg.ch))
    {
        runtime.Ctx.log(RC_LOG_ERROR, "Step2: rcCreateHeightfield failed");
        return false;
    }

    const int nverts = static_cast<int>(geom.Vertices.size() / 3);
    const int ntris  = static_cast<int>(geom.Triangles.size() / 3);

    // 以 geom.AreaTypes 为基础（Procedural 障碍下方已标 RC_NULL_AREA）
    // rcClearUnwalkableTriangles 把陡坡清为 RC_NULL_AREA，不会把 NULL 改回 WALKABLE
    std::vector<unsigned char> triareas = geom.AreaTypes;
    triareas.resize(ntris, RC_NULL_AREA);
    rcClearUnwalkableTriangles(&runtime.Ctx,
                               cfg.walkableSlopeAngle,
                               geom.Vertices.data(), nverts,
                               geom.Triangles.data(), ntris,
                               triareas.data());

    {
        ScopedTimer st(&timings.RasterizeMs);
        if (!rcRasterizeTriangles(&runtime.Ctx,
                                  geom.Vertices.data(), nverts,
                                  geom.Triangles.data(), triareas.data(),
                                  ntris, *solid, cfg.walkableClimb))
        {
            runtime.Ctx.log(RC_LOG_ERROR, "Step2: rcRasterizeTriangles failed");
            return false;
        }
    }

    return true;
}

// =============================================================================
// Step 3：过滤可行走区域（低悬挂、台阶、低矮空间）
// =============================================================================

static bool Step3_FilterWalkable(const rcConfig& cfg,
                                  NavRuntime&     runtime,
                                  PhaseTimings&   timings,
                                  rcHeightfield&  solid)
{
    ScopedTimer st(&timings.FilterMs);
    rcFilterLowHangingWalkableObstacles(&runtime.Ctx, cfg.walkableClimb, solid);
    rcFilterLedgeSpans             (&runtime.Ctx, cfg.walkableHeight, cfg.walkableClimb, solid);
    rcFilterWalkableLowHeightSpans (&runtime.Ctx, cfg.walkableHeight, solid);
    return true;
}

// =============================================================================
// Step 4：构建紧凑高度场（并释放不再需要的 Heightfield）
// =============================================================================

static bool Step4_BuildCompactHeightfield(const rcConfig&        cfg,
                                           NavRuntime&            runtime,
                                           PhaseTimings&          timings,
                                           rcHeightfield*&        solid,
                                           rcCompactHeightfield*& chf)
{
    chf = rcAllocCompactHeightfield();
    {
        ScopedTimer st(&timings.CompactMs);
        if (!chf ||
            !rcBuildCompactHeightfield(&runtime.Ctx,
                                       cfg.walkableHeight, cfg.walkableClimb,
                                       *solid, *chf))
        {
            runtime.Ctx.log(RC_LOG_ERROR, "Step4: rcBuildCompactHeightfield failed");
            return false;
        }
    }

    rcFreeHeightField(solid);
    solid = nullptr;

    return true;
}

// =============================================================================
// Step 5：腐蚀可行走区域 + 构建距离场
// =============================================================================

static bool Step5_ErodeAndDistanceField(const rcConfig&       cfg,
                                         NavRuntime&           runtime,
                                         PhaseTimings&         timings,
                                         rcCompactHeightfield& chf)
{
    {
        ScopedTimer st(&timings.ErodeMs);
        if (!rcErodeWalkableArea(&runtime.Ctx, cfg.walkableRadius, chf))
        {
            runtime.Ctx.log(RC_LOG_ERROR, "Step5: rcErodeWalkableArea failed");
            return false;
        }
    }
    {
        ScopedTimer st(&timings.DistFieldMs);
        if (!rcBuildDistanceField(&runtime.Ctx, chf))
        {
            runtime.Ctx.log(RC_LOG_ERROR, "Step5: rcBuildDistanceField failed");
            return false;
        }
    }
    return true;
}

// =============================================================================
// Step 6：构建区域
// =============================================================================

static bool Step6_BuildRegions(const rcConfig&       cfg,
                                NavRuntime&           runtime,
                                PhaseTimings&         timings,
                                rcCompactHeightfield& chf)
{
    ScopedTimer st(&timings.RegionsMs);
    if (!rcBuildRegions(&runtime.Ctx, chf,
                        0,
                        cfg.minRegionArea,
                        cfg.mergeRegionArea))
    {
        runtime.Ctx.log(RC_LOG_ERROR, "Step6: rcBuildRegions failed");
        return false;
    }
    return true;
}

// =============================================================================
// Step 10：设置 poly flags、填 dtNavMeshCreateParams、创建 Detour NavMesh
// =============================================================================

static bool Step10_CreateDetourNavMesh(const InputGeometry&             geom,
                                        const NavBuildConfig&            config,
                                        const rcConfig&                  cfg,
                                        NavRuntime&                      runtime,
                                        PhaseTimings&                    timings,
                                        const std::vector<OffMeshLink>*  extraLinks)
{
    rcPolyMesh*       pm  = runtime.PolyMesh;
    rcPolyMeshDetail* pmd = runtime.PolyMeshDetail;

    for (int i = 0; i < pm->npolys; ++i)
    {
        if (pm->areas[i] == RC_WALKABLE_AREA)
            pm->flags[i] = 0x01;
    }

    // OffMeshLink 数据展开（geom 中的手动链接 + extraLinks 中自动生成的链接合并）
    const int geomCount  = static_cast<int>(geom.OffMeshLinks.size());
    const int extraCount = extraLinks ? static_cast<int>(extraLinks->size()) : 0;
    const int linkCount  = geomCount + extraCount;
    std::vector<float>          omVerts(linkCount * 6);
    std::vector<float>          omRad  (linkCount);
    std::vector<unsigned short> omFlags(linkCount);
    std::vector<unsigned char>  omAreas(linkCount);
    std::vector<unsigned char>  omDirs (linkCount);

    // 填充 geom.OffMeshLinks
    for (int i = 0; i < geomCount; ++i)
    {
        const OffMeshLink& lk = geom.OffMeshLinks[i];
        omVerts[i*6+0] = lk.Start[0];  omVerts[i*6+1] = lk.Start[1];  omVerts[i*6+2] = lk.Start[2];
        omVerts[i*6+3] = lk.End  [0];  omVerts[i*6+4] = lk.End  [1];  omVerts[i*6+5] = lk.End  [2];
        omRad  [i]     = lk.Radius;
        omFlags[i]     = lk.Flags;
        omAreas[i]     = lk.Area;
        omDirs [i]     = lk.Dir;
    }
    // 追加 extraLinks（自动生成的 NavLink）
    for (int i = 0; i < extraCount; ++i)
    {
        const OffMeshLink& lk = (*extraLinks)[i];
        const int idx = geomCount + i;
        omVerts[idx*6+0] = lk.Start[0];  omVerts[idx*6+1] = lk.Start[1];  omVerts[idx*6+2] = lk.Start[2];
        omVerts[idx*6+3] = lk.End  [0];  omVerts[idx*6+4] = lk.End  [1];  omVerts[idx*6+5] = lk.End  [2];
        omRad  [idx]     = lk.Radius;
        omFlags[idx]     = lk.Flags;
        omAreas[idx]     = lk.Area;
        omDirs [idx]     = lk.Dir;
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
    params.walkableHeight   = config.AgentHeight;
    params.walkableRadius   = config.AgentRadius;
    params.walkableClimb    = config.AgentMaxClimb;
    rcVcopy(params.bmin, pm->bmin);
    rcVcopy(params.bmax, pm->bmax);
    params.cs               = cfg.cs;
    params.ch               = cfg.ch;
    params.buildBvTree      = true;

    // Off-mesh connections
    if (linkCount > 0)
    {
        params.offMeshConVerts  = omVerts.data();
        params.offMeshConRad    = omRad.data();
        params.offMeshConFlags  = omFlags.data();
        params.offMeshConAreas  = omAreas.data();
        params.offMeshConDir    = omDirs.data();
        params.offMeshConCount  = linkCount;
    }

    unsigned char* navData     = nullptr;
    int            navDataSize = 0;
    {
        ScopedTimer st(&timings.DetourCreateMs);
        if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
        {
            runtime.Ctx.log(RC_LOG_ERROR, "Step10: dtCreateNavMeshData failed");
            return false;
        }
    }

    // 保留一份副本供 Save NavMesh 使用
    runtime.NavMeshData.assign(navData, navData + navDataSize);

    // 创建 dtNavMesh（DT_TILE_FREE_DATA 表示 NavMesh 接管 navData 内存）
    runtime.NavMesh = dtAllocNavMesh();
    if (!runtime.NavMesh)
    {
        dtFree(navData);
        runtime.Ctx.log(RC_LOG_ERROR, "Step10: dtAllocNavMesh failed");
        return false;
    }

    dtStatus s = runtime.NavMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
    if (dtStatusFailed(s))
    {
        runtime.Ctx.log(RC_LOG_ERROR, "Step10: dtNavMesh::init failed");
        return false;
    }

    runtime.NavQuery = dtAllocNavMeshQuery();
    if (!runtime.NavQuery)
    {
        runtime.Ctx.log(RC_LOG_ERROR, "Step10: dtAllocNavMeshQuery failed");
        return false;
    }

    s = runtime.NavQuery->init(runtime.NavMesh, 2048);
    if (dtStatusFailed(s))
    {
        runtime.Ctx.log(RC_LOG_ERROR, "Step10: dtNavMeshQuery::init failed");
        return false;
    }

    runtime.bBuilt = true;
    return true;
}

// =============================================================================
// 公开 API 实现
// =============================================================================

void DestroyNavRuntime(NavRuntime& runtime)
{
    if (runtime.NavQuery)       { dtFreeNavMeshQuery(runtime.NavQuery);         runtime.NavQuery       = nullptr; }
    if (runtime.NavMesh)        { dtFreeNavMesh(runtime.NavMesh);               runtime.NavMesh        = nullptr; }
    if (runtime.TileCache)      { dtFreeTileCache(runtime.TileCache);           runtime.TileCache      = nullptr; }
    if (runtime.PolyMeshDetail) { rcFreePolyMeshDetail(runtime.PolyMeshDetail); runtime.PolyMeshDetail = nullptr; }
    if (runtime.PolyMesh)       { rcFreePolyMesh(runtime.PolyMesh);             runtime.PolyMesh       = nullptr; }
    runtime.bBuilt    = false;
    runtime.bTileMode = false;
    runtime.NavMeshData.clear();
}

bool InitNavMeshFromData(NavRuntime& runtime)
{
    if (runtime.NavMeshData.empty()) return false;

    // 为 dtNavMesh 分配独立缓冲（DT_TILE_FREE_DATA 会在内部 dtFree 掉它）
    // 这样 runtime.NavMeshData 副本保持不变，可继续用于 Save NavMesh
    const int n   = static_cast<int>(runtime.NavMeshData.size());
    unsigned char* buf = static_cast<unsigned char*>(dtAlloc(n, DT_ALLOC_PERM));
    if (!buf) return false;
    std::memcpy(buf, runtime.NavMeshData.data(), n);

    runtime.NavMesh = dtAllocNavMesh();
    if (!runtime.NavMesh) { dtFree(buf); return false; }

    dtStatus s = runtime.NavMesh->init(buf, n, DT_TILE_FREE_DATA);
    if (dtStatusFailed(s))
    {
        dtFreeNavMesh(runtime.NavMesh);
        runtime.NavMesh = nullptr;
        return false;
    }

    runtime.NavQuery = dtAllocNavMeshQuery();
    if (!runtime.NavQuery) return false;

    s = runtime.NavQuery->init(runtime.NavMesh, 2048);
    if (dtStatusFailed(s))
    {
        dtFreeNavMeshQuery(runtime.NavQuery);
        runtime.NavQuery = nullptr;
        return false;
    }

    runtime.bBuilt = true;
    return true;
}

bool BuildNavMesh(const InputGeometry&             geom,
                  const NavBuildConfig&            config,
                  const BuildVolume&               bv,
                  NavRuntime&                      runtime,
                  PhaseTimings&                    timings,
                  const std::vector<OffMeshLink>*  extraLinks)
{
    DestroyNavRuntime(runtime);
    timings = PhaseTimings{};
    runtime.Ctx.LogLines.clear();
    runtime.Ctx.log(RC_LOG_PROGRESS, "BuildNavMesh start");

    if (geom.Vertices.empty() || geom.Triangles.empty())
    {
        runtime.Ctx.log(RC_LOG_ERROR, "BuildNavMesh: input geometry is empty");
        return false;
    }

    // TileCache 模式走专用路径（不使用 Step2-Step10 的单 tile 流程）
    if (config.bUseTileCache)
    {
        const auto t0tc = std::chrono::high_resolution_clock::now();
        bool ok = BuildNavMeshTileCachePath(geom, config, bv, runtime, timings);
        const auto t1tc = std::chrono::high_resolution_clock::now();
        timings.TotalMs = std::chrono::duration<double, std::milli>(t1tc - t0tc).count();
        runtime.Ctx.log(ok ? RC_LOG_PROGRESS : RC_LOG_ERROR,
                        ok ? "BuildNavMesh (TileCache) OK" : "BuildNavMesh (TileCache) FAILED");
        return ok;
    }

    const auto t0 = std::chrono::high_resolution_clock::now();

    // 中间资源——任意步骤失败时在 cleanup lambda 中统一释放
    rcConfig              cfg{};
    rcHeightfield*        solid = nullptr;
    rcCompactHeightfield* chf   = nullptr;

    auto cleanup = [&]()
    {
        if (solid) { rcFreeHeightField(solid);       solid = nullptr; }
        if (chf)   { rcFreeCompactHeightfield(chf);  chf   = nullptr; }
    };

    // Step 1
    if (!Step1_ConfigureRcConfig(geom, config, bv, cfg))
        { cleanup(); return false; }

    // Step 2
    if (!Step2_RasterizeTriangles(geom, cfg, runtime, timings, solid))
        { cleanup(); return false; }

    // Step 3
    if (!Step3_FilterWalkable(cfg, runtime, timings, *solid))
        { cleanup(); return false; }

    // Step 4（内部释放 solid）
    if (!Step4_BuildCompactHeightfield(cfg, runtime, timings, solid, chf))
        { cleanup(); return false; }

    // Step 5
    if (!Step5_ErodeAndDistanceField(cfg, runtime, timings, *chf))
        { cleanup(); return false; }

    // Step 6
    if (!Step6_BuildRegions(cfg, runtime, timings, *chf))
        { cleanup(); return false; }

    // --- Step 7: 构建轮廓集（CHF 保持有效，供 Step9 使用）---
    rcContourSet* cset = rcAllocContourSet();
    {
        ScopedTimer st(&timings.ContoursMs);
        if (!cset ||
            !rcBuildContours(&runtime.Ctx, *chf,
                             cfg.maxSimplificationError,
                             cfg.maxEdgeLen,
                             *cset))
        {
            runtime.Ctx.log(RC_LOG_ERROR, "Step7: rcBuildContours failed");
            rcFreeContourSet(cset);
            cleanup();
            return false;
        }
    }

    // --- Step 8: 构建多边形网格 ---
    runtime.PolyMesh = rcAllocPolyMesh();
    {
        ScopedTimer st(&timings.PolyMeshMs);
        if (!runtime.PolyMesh ||
            !rcBuildPolyMesh(&runtime.Ctx, *cset,
                             cfg.maxVertsPerPoly,
                             *runtime.PolyMesh))
        {
            runtime.Ctx.log(RC_LOG_ERROR, "Step8: rcBuildPolyMesh failed");
            rcFreeContourSet(cset);
            cleanup();
            return false;
        }
    }
    rcFreeContourSet(cset); cset = nullptr;

    // --- Step 9: 构建细节网格（此时 CHF 仍然有效）---
    runtime.PolyMeshDetail = rcAllocPolyMeshDetail();
    {
        ScopedTimer st(&timings.DetailMeshMs);
        if (!runtime.PolyMeshDetail ||
            !rcBuildPolyMeshDetail(&runtime.Ctx,
                                   *runtime.PolyMesh,
                                   *chf,
                                   cfg.detailSampleDist,
                                   cfg.detailSampleMaxError,
                                   *runtime.PolyMeshDetail))
        {
            runtime.Ctx.log(RC_LOG_ERROR, "Step9: rcBuildPolyMeshDetail failed");
            cleanup();
            return false;
        }
    }

    // CHF 使命完成
    rcFreeCompactHeightfield(chf); chf = nullptr;

    // --- Step 10: 创建 Detour NavMesh ---
    if (!Step10_CreateDetourNavMesh(geom, config, cfg, runtime, timings, extraLinks))
    {
        cleanup();
        return false;
    }

    const auto t1   = std::chrono::high_resolution_clock::now();
    timings.TotalMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    runtime.Ctx.log(RC_LOG_PROGRESS, "BuildNavMesh OK");
    return true;
}

// =============================================================================
// TileCache 构建辅助
// =============================================================================

/// 每个 tile 的尺寸（格子数），固定为 32×32
static constexpr int kTileSize = 32;

/// TileCache 线程-local 分配器（每次 BuildNavMesh 分配一个，供整个构建生命周期使用）
/// 使用 static 局部变量以保持简洁（单线程构建场景）
static LinearAllocator  s_talloc(32 * 1024);
static FastLZCompressor s_tcomp;
static NavMeshProcess   s_tmproc;

/**
 * @brief 光栅化一个 tile 并将所有 layer 压缩加入 TileCache。
 *        对应 Sample_TempObstacles 中的 rasterizeTileLayers()。
 */
static bool RasterizeTileToCache(
    int tx, int ty,
    const InputGeometry&  geom,
    const rcConfig&       baseCfg,
    NavRuntime&           runtime,
    dtTileCache*          tileCache)
{
    // 计算 tile 包围盒（加一格 border）
    const float tcs = baseCfg.cs * kTileSize;
    const int   tw  = baseCfg.width;
    const int   th  = baseCfg.height;
    (void)tw; (void)th;

    float tbmin[3], tbmax[3];
    tbmin[0] = baseCfg.bmin[0] + tx * tcs;
    tbmin[1] = baseCfg.bmin[1];
    tbmin[2] = baseCfg.bmin[2] + ty * tcs;
    tbmax[0] = tbmin[0] + tcs;
    tbmax[1] = baseCfg.bmax[1];
    tbmax[2] = tbmin[2] + tcs;

    // 扩展 border 以避免 tile 边缘剪裁
    const float border = baseCfg.cs * (float)baseCfg.walkableRadius * 3;
    float ebmin[3] = { tbmin[0] - border, tbmin[1], tbmin[2] - border };
    float ebmax[3] = { tbmax[0] + border, tbmax[1], tbmax[2] + border };

    // 裁剪到实际场景范围
    for (int k = 0; k < 3; k += 2)  // only X and Z
    {
        ebmin[k] = rcMax(baseCfg.bmin[k], ebmin[k]);
        ebmax[k] = rcMin(baseCfg.bmax[k], ebmax[k]);
    }

    // tile-local rcConfig
    rcConfig tcfg = baseCfg;
    rcVcopy(tcfg.bmin, ebmin);
    rcVcopy(tcfg.bmax, ebmax);
    tcfg.width  = kTileSize + baseCfg.borderSize * 2;
    tcfg.height = kTileSize + baseCfg.borderSize * 2;
    rcCalcGridSize(tcfg.bmin, tcfg.bmax, tcfg.cs, &tcfg.width, &tcfg.height);

    // 光栅化
    rcHeightfield* solid = rcAllocHeightfield();
    if (!solid || !rcCreateHeightfield(&runtime.Ctx, *solid,
                                        tcfg.width, tcfg.height,
                                        tcfg.bmin, tcfg.bmax,
                                        tcfg.cs, tcfg.ch))
    {
        rcFreeHeightField(solid);
        return false;
    }

    const int ntris = static_cast<int>(geom.Triangles.size() / 3);
    const int nverts= static_cast<int>(geom.Vertices.size()  / 3);
    std::vector<unsigned char> triareas = geom.AreaTypes;
    triareas.resize(ntris, RC_NULL_AREA);
    rcClearUnwalkableTriangles(&runtime.Ctx, tcfg.walkableSlopeAngle,
                               geom.Vertices.data(), nverts,
                               geom.Triangles.data(), ntris,
                               triareas.data());
    rcRasterizeTriangles(&runtime.Ctx,
                         geom.Vertices.data(), nverts,
                         geom.Triangles.data(), triareas.data(), ntris,
                         *solid, tcfg.walkableClimb);

    // 过滤
    rcFilterLowHangingWalkableObstacles(&runtime.Ctx, tcfg.walkableClimb, *solid);
    rcFilterLedgeSpans(&runtime.Ctx, tcfg.walkableHeight, tcfg.walkableClimb, *solid);
    rcFilterWalkableLowHeightSpans(&runtime.Ctx, tcfg.walkableHeight, *solid);

    rcCompactHeightfield* chf = rcAllocCompactHeightfield();
    if (!chf || !rcBuildCompactHeightfield(&runtime.Ctx,
                                            tcfg.walkableHeight, tcfg.walkableClimb,
                                            *solid, *chf))
    {
        rcFreeHeightField(solid);
        rcFreeCompactHeightfield(chf);
        return false;
    }
    rcFreeHeightField(solid); solid = nullptr;

    rcErodeWalkableArea(&runtime.Ctx, tcfg.walkableRadius, *chf);

    // 构建 HeightfieldLayers（TileCache 核心格式）
    rcHeightfieldLayerSet* lset = rcAllocHeightfieldLayerSet();
    if (!lset || !rcBuildHeightfieldLayers(&runtime.Ctx, *chf,
                                            baseCfg.borderSize,
                                            tcfg.walkableHeight,
                                            *lset))
    {
        rcFreeCompactHeightfield(chf);
        rcFreeHeightfieldLayerSet(lset);
        return false;
    }
    rcFreeCompactHeightfield(chf); chf = nullptr;

    // 对每个 layer 构建压缩 tile 并添加到 TileCache
    for (int i = 0; i < lset->nlayers; ++i)
    {
        const rcHeightfieldLayer& layer = lset->layers[i];

        dtTileCacheLayerHeader header{};
        header.magic   = DT_TILECACHE_MAGIC;
        header.version = DT_TILECACHE_VERSION;
        header.tx = tx;
        header.ty = ty;
        header.tlayer = i;
        dtVcopy(header.bmin, layer.bmin);
        dtVcopy(header.bmax, layer.bmax);
        header.width  = static_cast<unsigned char>(layer.width);
        header.height = static_cast<unsigned char>(layer.height);
        header.minx   = static_cast<unsigned char>(layer.minx);
        header.maxx   = static_cast<unsigned char>(layer.maxx);
        header.miny   = static_cast<unsigned char>(layer.miny);
        header.maxy   = static_cast<unsigned char>(layer.maxy);
        header.hmin   = static_cast<unsigned short>(layer.hmin);
        header.hmax   = static_cast<unsigned short>(layer.hmax);

        unsigned char* tileData     = nullptr;
        int            tileDataSize = 0;
        if (dtStatusFailed(dtBuildTileCacheLayer(&s_tcomp, &header,
                                                  layer.heights, layer.areas, layer.cons,
                                                  &tileData, &tileDataSize)))
            continue;

        tileCache->addTile(tileData, tileDataSize, DT_COMPRESSEDTILE_FREE_DATA, nullptr);
    }

    rcFreeHeightfieldLayerSet(lset);
    return true;
}

/**
 * @brief TileCache 模式的完整构建路径（替代标准 Step10）。
 *        - 初始化 Tiled dtNavMesh
 *        - 初始化 dtTileCache
 *        - 对每个 tile 调 RasterizeTileToCache()
 *        - 调 tileCache->buildNavMeshTilesAt() 生成初始 tile 数据
 */
static bool BuildNavMeshTileCachePath(
    const InputGeometry&  geom,
    const NavBuildConfig& config,
    const BuildVolume&    bv,
    NavRuntime&           runtime,
    PhaseTimings&         timings)
{
    // 使用 BV 或几何自身 AABB
    float bmin[3], bmax[3];
    if (bv.bActive)
    {
        rcVcopy(bmin, bv.Min);
        rcVcopy(bmax, bv.Max);
    }
    else
    {
        rcVcopy(bmin, &geom.Bounds[0]);
        rcVcopy(bmax, &geom.Bounds[3]);
    }

    const float cs = config.CellSize;
    const float ch = config.CellHeight;

    // 计算整体 tile 网格大小
    int gw = 0, gh = 0;
    rcCalcGridSize(bmin, bmax, cs, &gw, &gh);
    const int tx = (gw + kTileSize - 1) / kTileSize;
    const int ty = (gh + kTileSize - 1) / kTileSize;
    const int maxTiles = tx * ty;

    // 构建 tile 层数上限（每列最多 4 层）
    const int maxObstacles = 128;

    // --- 初始化 Tiled dtNavMesh ---
    dtNavMeshParams nmParams{};
    rcVcopy(nmParams.orig, bmin);
    nmParams.tileWidth  = kTileSize * cs;
    nmParams.tileHeight = kTileSize * cs;
    nmParams.maxTiles   = maxTiles * 4;   // 留多一些空间给 layer
    nmParams.maxPolys   = 1 << 12;

    runtime.NavMesh = dtAllocNavMesh();
    if (!runtime.NavMesh)
    {
        runtime.Ctx.log(RC_LOG_ERROR, "TileCache: dtAllocNavMesh failed");
        return false;
    }
    if (dtStatusFailed(runtime.NavMesh->init(&nmParams)))
    {
        runtime.Ctx.log(RC_LOG_ERROR, "TileCache: dtNavMesh::init (tiled) failed");
        return false;
    }

    // --- 初始化 dtTileCache ---
    dtTileCacheParams tcParams{};
    rcVcopy(tcParams.orig, bmin);
    tcParams.cs                    = cs;
    tcParams.ch                    = ch;
    tcParams.width                 = kTileSize;
    tcParams.height                = kTileSize;
    tcParams.walkableHeight        = config.AgentHeight;
    tcParams.walkableRadius        = config.AgentRadius;
    tcParams.walkableClimb         = config.AgentMaxClimb;
    tcParams.maxSimplificationError= config.EdgeMaxError;
    tcParams.maxTiles              = maxTiles * 4;
    tcParams.maxObstacles          = maxObstacles;

    s_talloc.reset();
    runtime.TileCache = dtAllocTileCache();
    if (!runtime.TileCache)
    {
        runtime.Ctx.log(RC_LOG_ERROR, "TileCache: dtAllocTileCache failed");
        return false;
    }
    if (dtStatusFailed(runtime.TileCache->init(&tcParams, &s_talloc, &s_tcomp, &s_tmproc)))
    {
        runtime.Ctx.log(RC_LOG_ERROR, "TileCache: dtTileCache::init failed");
        return false;
    }

    // --- 构建 rcConfig（用于每 tile 的光栅化） ---
    rcConfig baseCfg{};
    baseCfg.cs                     = cs;
    baseCfg.ch                     = ch;
    baseCfg.walkableSlopeAngle     = config.AgentMaxSlope;
    baseCfg.walkableHeight         = static_cast<int>(std::ceil (config.AgentHeight   / ch));
    baseCfg.walkableClimb          = static_cast<int>(std::floor(config.AgentMaxClimb / ch));
    baseCfg.walkableRadius         = static_cast<int>(std::ceil (config.AgentRadius   / cs));
    baseCfg.maxEdgeLen             = static_cast<int>(config.EdgeMaxLen / cs);
    baseCfg.maxSimplificationError = config.EdgeMaxError;
    baseCfg.minRegionArea          = static_cast<int>(rcSqr(config.RegionMinSize));
    baseCfg.mergeRegionArea        = static_cast<int>(rcSqr(config.RegionMergeSize));
    baseCfg.maxVertsPerPoly        = config.VertsPerPoly;
    baseCfg.borderSize             = baseCfg.walkableRadius + 3;
    baseCfg.tileSize               = kTileSize;
    rcVcopy(baseCfg.bmin, bmin);
    rcVcopy(baseCfg.bmax, bmax);
    rcCalcGridSize(baseCfg.bmin, baseCfg.bmax, cs, &baseCfg.width, &baseCfg.height);

    // --- 逐 tile 光栅化 ---
    {
        ScopedTimer st(&timings.RasterizeMs);
        for (int y = 0; y < ty; ++y)
        {
            for (int x = 0; x < tx; ++x)
            {
                RasterizeTileToCache(x, y, geom, baseCfg, runtime, runtime.TileCache);
            }
        }
    }

    // --- 生成初始 NavMesh tile 数据 ---
    {
        ScopedTimer st(&timings.DetourCreateMs);
        for (int y = 0; y < ty; ++y)
        {
            for (int x = 0; x < tx; ++x)
            {
                if (dtStatusFailed(runtime.TileCache->buildNavMeshTilesAt(x, y, runtime.NavMesh)))
                    runtime.Ctx.log(RC_LOG_WARNING, "TileCache: buildNavMeshTilesAt failed for some tile");
            }
        }
    }

    // --- 初始化 NavMeshQuery ---
    runtime.NavQuery = dtAllocNavMeshQuery();
    if (!runtime.NavQuery)
    {
        runtime.Ctx.log(RC_LOG_ERROR, "TileCache: dtAllocNavMeshQuery failed");
        return false;
    }
    if (dtStatusFailed(runtime.NavQuery->init(runtime.NavMesh, 2048)))
    {
        runtime.Ctx.log(RC_LOG_ERROR, "TileCache: dtNavMeshQuery::init failed");
        return false;
    }

    runtime.bBuilt    = true;
    runtime.bTileMode = true;
    return true;
}

// =============================================================================
// TileCache 运行时 API
// =============================================================================

void UpdateTileCache(NavRuntime& runtime)
{
    if (!runtime.TileCache || !runtime.NavMesh) return;
    bool upToDate = false;
    // 可能需要多次 update 才能处理完所有待处理请求
    for (int i = 0; i < 4 && !upToDate; ++i)
        runtime.TileCache->update(0, runtime.NavMesh, &upToDate);
}

dtObstacleRef AddObstacleToTileCache(NavRuntime& runtime, const Obstacle& o)
{
    if (!runtime.TileCache) return 0;
    dtObstacleRef ref = 0;
    if (o.Shape == ObstacleShape::Cylinder)
    {
        const float pos[3] = { o.CX, 0.0f, o.CZ };
        runtime.TileCache->addObstacle(pos, o.Radius, o.Height, &ref);
    }
    else
    {
        const float bmin[3] = { o.CX - o.SX, 0.0f,     o.CZ - o.SZ };
        const float bmax[3] = { o.CX + o.SX, o.Height, o.CZ + o.SZ };
        runtime.TileCache->addBoxObstacle(bmin, bmax, &ref);
    }
    return ref;
}

void RemoveObstacleFromTileCache(NavRuntime& runtime, dtObstacleRef ref)
{
    if (!runtime.TileCache || ref == 0) return;
    runtime.TileCache->removeObstacle(ref);
}

void SyncObstaclesToTileCache(const InputGeometry&        geom,
                               NavRuntime&                 runtime,
                               std::vector<dtObstacleRef>& obsRefs)
{
    obsRefs.clear();
    obsRefs.reserve(geom.Obstacles.size());
    for (const Obstacle& o : geom.Obstacles)
    {
        dtObstacleRef ref = AddObstacleToTileCache(runtime, o);
        obsRefs.push_back(ref);
    }
    // 处理完所有添加请求
    UpdateTileCache(runtime);
}

} // namespace NavBuilder
