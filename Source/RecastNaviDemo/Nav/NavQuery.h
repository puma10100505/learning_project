#pragma once
/*
 * Nav/NavQuery.h
 * --------------
 * Detour 路径查询接口：
 *   - FindPath：从 start 到 end 的完整寻路流水线
 *     （findNearestPoly × 2 → findPath → findStraightPath）
 *
 * 设计原则：
 *   - 纯函数，不持有全局状态
 *   - NavRuntime 只读（const&），结果通过 out 参数返回
 *   - 各阶段耗时写入 PathTimings（Shared/Profiling.h）
 *
 * 依赖：Nav/NavTypes.h、Shared/Profiling.h
 * 不依赖：UI/ 或 Render/
 */

#include "NavTypes.h"
#include "../Shared/Profiling.h"

#include <string>
#include <vector>

// Detour
#include "DetourNavMesh.h"

namespace NavQuery
{

/**
 * @brief  从 start 到 end 执行完整寻路，结果写入 out 参数。
 *
 * @param nav              已构建好的 NavRuntime（bBuilt == true，NavQuery != nullptr）
 * @param start            起点世界坐标 [x, y, z]
 * @param end              终点世界坐标 [x, y, z]
 * @param geom             输入几何（仅使用 Bounds[1]/[4] 计算 Y 方向搜索容差）
 * @param outStraightPath  输出直线路径点序列（x,y,z 交替，长度 = corner 数 × 3）
 * @param outCorridor      输出多边形走廊（findPath 返回的 dtPolyRef 数组）
 * @param outPolyCount     输出走廊多边形数量
 * @param outStatus        输出可读状态字符串（"OK (...)" 或错误描述）
 * @param outTimings       输出各阶段耗时（毫秒）
 * @return true 表示成功找到路径；失败时 outStatus 说明原因
 */
bool FindPath(const NavRuntime&       nav,
              const float             start[3],
              const float             end[3],
              const InputGeometry&    geom,
              std::vector<float>&     outStraightPath,
              std::vector<dtPolyRef>& outCorridor,
              int&                    outPolyCount,
              std::string&            outStatus,
              PathTimings&            outTimings);

} // namespace NavQuery
