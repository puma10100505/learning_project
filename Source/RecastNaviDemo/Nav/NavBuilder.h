#pragma once
/*
 * Nav/NavBuilder.h
 * ----------------
 * NavMesh 构建接口：
 *   - BuildNavMesh：完整的 10 步 Recast/Detour 构建流水线
 *   - DestroyNavRuntime：释放所有运行时资源
 *   - InitNavMeshFromData：从已有的 navData 字节流创建 dtNavMesh + dtNavMeshQuery
 *
 * 依赖：Nav/NavTypes.h、Shared/Profiling.h
 * 不依赖：UI/ 或 Render/
 */

#include "NavTypes.h"
#include "../Shared/Profiling.h"

namespace NavBuilder
{

// =============================================================================
// NavMesh 构建
// =============================================================================

/**
 * @brief  用给定几何与配置构建 NavMesh，结果写入 runtime。
 *
 * @param geom       输入几何（顶点、三角形、AreaTypes、包围盒）
 * @param config     构建参数（CellSize、AgentHeight 等）
 * @param bv         自定义生成区域（bv.bActive=false 时使用几何自身包围盒）
 * @param runtime    输出：PolyMesh*、PolyMeshDetail*、NavMesh*、NavQuery*，以及 NavMeshData
 * @param timings    输出：各阶段耗时（毫秒）
 * @param extraLinks 附加 OffMeshLink（如自动生成的 NavLink），会与 geom.OffMeshLinks 合并
 *                   传 nullptr 表示不附加额外连接
 * @return true 表示成功；失败时 runtime.Ctx.LogLines 含错误信息
 */
bool BuildNavMesh(const InputGeometry&              geom,
                  const NavBuildConfig&              config,
                  const BuildVolume&                 bv,
                  NavRuntime&                        runtime,
                  PhaseTimings&                      timings,
                  const std::vector<OffMeshLink>*    extraLinks = nullptr);

// =============================================================================
// 运行时生命周期
// =============================================================================

/**
 * @brief 释放 runtime 中所有 Recast/Detour 资源并重置状态。
 *        调用后 runtime.bBuilt == false，所有指针为 nullptr。
 */
void DestroyNavRuntime(NavRuntime& runtime);

/**
 * @brief 用 runtime.NavMeshData 中的字节流创建 dtNavMesh + dtNavMeshQuery。
 *        成功后 runtime.bBuilt == true。
 *
 * 注意：此函数会把 NavMeshData 的所有权转交给 dtNavMesh（DT_TILE_FREE_DATA），
 *       因此调用后 runtime.NavMeshData 不再保留副本。若仍需要副本请在调用前自行拷贝。
 */
bool InitNavMeshFromData(NavRuntime& runtime);

// =============================================================================
// TileCache 动态障碍 API（仅在 runtime.bTileMode == true 时有效）
// =============================================================================

/**
 * @brief 每帧调用，将待处理的障碍变更刷新到 NavMesh 中。
 *        应在 OnGUI / OnTick 中调用（调用频率与帧率相同）。
 * @param runtime  含有效 TileCache + NavMesh 的运行时
 */
void UpdateTileCache(NavRuntime& runtime);

/**
 * @brief 向 TileCache 添加一个障碍（Box 或 Cylinder）。
 * @param runtime  含有效 TileCache 的运行时
 * @param o        障碍描述
 * @return Detour 障碍引用（失败时返回 0）
 */
dtObstacleRef AddObstacleToTileCache(NavRuntime& runtime, const Obstacle& o);

/**
 * @brief 从 TileCache 移除一个障碍。
 * @param runtime  含有效 TileCache 的运行时
 * @param ref      由 AddObstacleToTileCache 返回的引用
 */
void RemoveObstacleFromTileCache(NavRuntime& runtime, dtObstacleRef ref);

/**
 * @brief 将 geom.Obstacles 全部同步到 TileCache，并填充 obsRefs 映射。
 *        通常在 BuildNavMesh（TileCache 模式）成功后立刻调用一次。
 */
void SyncObstaclesToTileCache(const InputGeometry&       geom,
                               NavRuntime&                runtime,
                               std::vector<dtObstacleRef>& obsRefs);

} // namespace NavBuilder
