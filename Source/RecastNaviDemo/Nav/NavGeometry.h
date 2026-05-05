#pragma once
/*
 * Nav/NavGeometry.h
 * -----------------
 * 输入几何管理：
 *   - Procedural 地面网格生成（障碍物挖洞）
 *   - OBJ 文件加载
 *   - 障碍物工厂函数
 *   - 障碍物约束（Clamp 到场景范围）
 *
 * 依赖：Nav/NavTypes.h
 * 不依赖：UI/ 或 Render/
 */

#include "NavTypes.h"
#include <string>

namespace NavGeometry
{

// =============================================================================
// 障碍工厂
// =============================================================================

/// 创建 Box 障碍
Obstacle MakeBox(float cx, float cz, float sx, float sz, float h = 1.6f);

/// 创建 Cylinder 障碍
Obstacle MakeCylinder(float cx, float cz, float r, float h = 1.6f);

// =============================================================================
// 障碍约束
// =============================================================================

/// 把单个障碍的中心 clamp 到 halfSize 范围内（考虑障碍自身尺寸 + 1 单位边距）
void ClampObstacleToScene(Obstacle& o, float halfSize);

/// clamp geom 中所有障碍到 halfSize 范围
void ClampAllObstaclesToScene(InputGeometry& geom, float halfSize);

// =============================================================================
// 三角形辅助
// =============================================================================

/// 判断三角形重心是否位于任一障碍内部（用于 Procedural 几何的 Area 标记）
bool TriCenterInsideAnyObstacle(const float v0[3], const float v1[3], const float v2[3],
                                 const std::vector<Obstacle>& obs);

// =============================================================================
// Procedural 几何
// =============================================================================

/// 用场景配置重新生成程序化地面（保留 geom 中的 Obstacles 列表）
void RebuildProceduralGeometry(InputGeometry& geom, const SceneConfig& scene);

/// 初始化默认几何（若干 box + cylinder 障碍 + 程序化地面）
void InitDefaultGeometry(InputGeometry& geom, const SceneConfig& scene);

// =============================================================================
// 障碍实体网格（顶面 + 侧面，可走）
// =============================================================================

/// 向 geom 追加每个障碍的实体网格（侧面四边形 + 顶面），所有三角形标记为
/// RC_WALKABLE_AREA。此函数应在 RebuildProceduralGeometry 完成后、
/// scene.bObstacleSolid == true 时调用。
void AppendObstacleSolidMesh(InputGeometry& geom);

// =============================================================================
// OBJ 加载
// =============================================================================

/// 加载 OBJ 几何（仅解析 v / f，支持 fan triangulation）
/// 成功返回 true，失败时 errOut 说明原因
bool LoadObjGeometry(const char* path, InputGeometry& geom, std::string& errOut);

} // namespace NavGeometry
