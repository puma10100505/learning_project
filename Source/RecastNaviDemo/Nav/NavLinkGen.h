#pragma once
/*
 * Nav/NavLinkGen.h
 * ----------------
 * 基于 NavMesh 边界轮廓自动生成 Off-mesh NavLink。
 *
 * 算法：
 *   遍历 dtNavMesh 的所有 tile → poly → edge，
 *   对每条"外边界"（neis[j] == 0）取其中点 A，
 *   与空间近邻 tile 的外边界中点 B 比较 Y 差值 dY = B.y - A.y：
 *     - |dY| <= JumpUpHeight                 → 双向 NavLink (Dir=1)
 *     - JumpUpHeight < -dY <= DropDownMax    → 单向向下 NavLink A→B (Dir=0)
 *   EdgeRadius 用于点对搜索和 NavLink 本身的 Radius 字段。
 *
 * 生成的 NavLink 存储在 std::vector<OffMeshLink>，由调用方放入 AppState。
 * 不修改 NavMesh 本身；渲染时用区别色（紫/品红）区分手动 OffMeshLink。
 */

#include "NavTypes.h"
#include <vector>

// =============================================================================
// 生成接口
// =============================================================================
namespace NavLinkGen
{

/**
 * @brief  根据已构建的 NavMesh 自动生成边界 NavLink。
 *
 * @param runtime  已 bBuilt = true 的 NavRuntime（需要 NavMesh）
 * @param cfg      生成参数
 * @return 生成的 OffMeshLink 列表（若 !cfg.bEnabled 或 NavMesh 无效则为空）
 */
std::vector<OffMeshLink> GenerateEdgeNavLinks(
    const NavRuntime&         runtime,
    const AutoNavLinkConfig&  cfg);

} // namespace NavLinkGen
