/*
 * Nav/NavQuery.cpp
 * ----------------
 * Detour 路径查询实现。
 *
 * 寻路流水线：
 *   1. findNearestPoly (start)  ← 把世界坐标映射到最近的 NavMesh 多边形
 *   2. findNearestPoly (end)    ← 同上
 *   3. findPath                 ← 返回多边形走廊（Corridor）
 *   4. findStraightPath         ← 把走廊简化为直线折线（Waypoints）
 *
 * Y 方向搜索容差（halfExtents[1]）自适应：
 *   max(8.0f, (geom.Bounds[4] - geom.Bounds[1]) * 0.5f + 4.0f)
 *   确保 OBJ 网格整体在 y ≠ 0 时，点击位置仍能命中 NavMesh。
 */

#include "NavQuery.h"

#include <algorithm>
#include <chrono>
#include <string>

// Detour
#include "DetourNavMeshQuery.h"
#include "DetourStatus.h"

namespace NavQuery
{

bool FindPath(const NavRuntime&       nav,
              const float             start[3],
              const float             end[3],
              const InputGeometry&    geom,
              std::vector<float>&     outStraightPath,
              std::vector<dtPolyRef>& outCorridor,
              int&                    outPolyCount,
              std::string&            outStatus,
              PathTimings&            outTimings)
{
    // 清空输出
    outStraightPath.clear();
    outCorridor.clear();
    outPolyCount = 0;
    outTimings   = PathTimings{};

    if (!nav.bBuilt || !nav.NavQuery)
    {
        outStatus = "Build NavMesh first";
        return false;
    }

    // 总耗时起始点（各阶段 ScopedTimer 累加，TotalMs 在函数末尾整体计算）
    const auto totalT0 = std::chrono::high_resolution_clock::now();

    // 查询过滤器：接受全部 flag
    dtQueryFilter filter;
    filter.setIncludeFlags(0xffff);
    filter.setExcludeFlags(0);

    // Y 方向容差自适应：整个几何高度的一半 + 余量，防止 OBJ 整体偏移后点击偏离
    const float yExtent     = std::max(8.0f, (geom.Bounds[4] - geom.Bounds[1]) * 0.5f + 4.0f);
    const float halfExtents[3] = { 2.0f, yExtent, 2.0f };

    // -----------------------------------------------------------------------
    // Step 1 & 2：将起终点映射到 NavMesh 上最近的多边形
    // -----------------------------------------------------------------------
    dtPolyRef startRef = 0, endRef = 0;
    float     nearestStart[3], nearestEnd[3];

    {
        ScopedTimer st(&outTimings.NearestStartMs);
        nav.NavQuery->findNearestPoly(start, halfExtents, &filter, &startRef, nearestStart);
    }
    {
        ScopedTimer st(&outTimings.NearestEndMs);
        nav.NavQuery->findNearestPoly(end, halfExtents, &filter, &endRef, nearestEnd);
    }

    if (!startRef || !endRef)
    {
        outStatus = "Start or end not on navmesh";
        outTimings.TotalMs = std::chrono::duration<double, std::milli>(
            std::chrono::high_resolution_clock::now() - totalT0).count();
        return false;
    }

    // -----------------------------------------------------------------------
    // Step 3：findPath — 返回多边形走廊
    // -----------------------------------------------------------------------
    constexpr int MAX_POLYS = 256;
    dtPolyRef polys[MAX_POLYS];
    int       npolys = 0;
    dtStatus  s      = 0;

    {
        ScopedTimer st(&outTimings.FindPathMs);
        s = nav.NavQuery->findPath(startRef, endRef,
                                   nearestStart, nearestEnd,
                                   &filter, polys, &npolys, MAX_POLYS);
    }

    if (dtStatusFailed(s) || npolys == 0)
    {
        outStatus = "findPath failed";
        outTimings.TotalMs = std::chrono::duration<double, std::milli>(
            std::chrono::high_resolution_clock::now() - totalT0).count();
        return false;
    }

    outPolyCount = npolys;
    outCorridor.assign(polys, polys + npolys);

    // -----------------------------------------------------------------------
    // Step 4：findStraightPath — 把走廊简化为直线折线
    // -----------------------------------------------------------------------
    constexpr int MAX_STRAIGHT = 256;
    float         straightPath[MAX_STRAIGHT * 3];
    unsigned char straightFlags[MAX_STRAIGHT];
    dtPolyRef     straightRefs[MAX_STRAIGHT];
    int           nstraight = 0;

    {
        ScopedTimer st(&outTimings.StraightPathMs);
        s = nav.NavQuery->findStraightPath(nearestStart, nearestEnd,
                                           polys, npolys,
                                           straightPath, straightFlags, straightRefs,
                                           &nstraight, MAX_STRAIGHT);
    }

    if (dtStatusFailed(s))
    {
        outStatus = "findStraightPath failed";
        outTimings.TotalMs = std::chrono::duration<double, std::milli>(
            std::chrono::high_resolution_clock::now() - totalT0).count();
        return false;
    }

    outStraightPath.assign(straightPath, straightPath + nstraight * 3);

    outStatus = "OK (" + std::to_string(nstraight) + " corners, "
              + std::to_string(npolys)    + " polys)";

    outTimings.TotalMs = std::chrono::duration<double, std::milli>(
        std::chrono::high_resolution_clock::now() - totalT0).count();

    return true;
}

} // namespace NavQuery
