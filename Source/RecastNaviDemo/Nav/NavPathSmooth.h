#pragma once
/*
 * Nav/NavPathSmooth.h
 * -------------------
 * 路径平滑后处理算法。
 *
 * 提供两种平滑方法：
 *   - StringPull  : 字符串拉直（利用 Detour raycast 从走廊中剔除冗余转角）
 *   - BezierSubdivide : 纯几何三次均匀 B 样条细分（不依赖 NavMesh 查询）
 *
 * 两个函数均为纯函数，无副作用，输入不变。
 *
 * 依赖：Nav/NavTypes.h、DetourNavMesh.h
 * 不依赖：UI/ 或 Render/
 */

#include "NavTypes.h"

#include <vector>

// Detour
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"

namespace NavPathSmooth
{

/**
 * @brief  字符串拉直平滑。
 *
 * 沿走廊多边形序列迭代，用 dtNavMeshQuery::raycast 检测能否从当前点直线到达
 * 更远的折角，若能则跳过中间冗余转角，输出更少折角的折线。
 *
 * @param straight   原始 findStraightPath 输出（xyz 交替，length = corners × 3）
 * @param corridor   原始 findPath 输出的 dtPolyRef 数组
 * @param query      Detour NavMeshQuery（非 null）
 * @param filter     Detour 查询过滤器（null 时使用默认权重）
 * @return 平滑后折线点序列（xyz 交替），失败时返回 straight 原样
 */
std::vector<float> StringPull(
    const std::vector<float>&     straight,
    const std::vector<dtPolyRef>& corridor,
    const dtNavMeshQuery*         query,
    const dtQueryFilter*          filter);

/**
 * @brief  均匀 B 样条细分平滑（Chaikin 变体）。
 *
 * 对折线做 subdivisions 轮 Chaikin 切角，每轮将每段切出 1/4 与 3/4 两点，
 * 结果保留首尾原始端点。纯几何，不依赖 NavMesh。
 *
 * @param pts          输入折线点序列（xyz 交替），至少 6 个分量（2 个点）
 * @param subdivisions 细分轮数（1~6，超出范围被 clamp）
 * @return 细分后折线点序列（xyz 交替）
 */
std::vector<float> BezierSubdivide(
    const std::vector<float>& pts,
    int subdivisions = 4);

} // namespace NavPathSmooth
