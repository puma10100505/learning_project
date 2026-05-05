#pragma once
/*
 * Phys/PhysWorld.h
 * -----------------
 * PhysX 静态场景：导航输入几何的三角网碰撞体 + 程序化障碍盒/胶囊近似圆柱。
 * 用于场景射线检测（落点、障碍选择）。
 *
 * 在未链接 PhysX 的平台（如当前 Linux 工程配置）下编译为存根，接口仍可用。
 */

struct InputGeometry;

namespace PhysWorld
{

bool Init();
void Shutdown();
bool IsActive();

/// 根据当前 InputGeometry 重建静态碰撞体（障碍索引写入 actor userData：index+1；地形为 nullptr）。
void RebuildFromInputGeometry(const InputGeometry& geom);

/// 世界空间射线（dir 不必预先归一化）。命中写入 out*；outObstacleIndex 为 -1 表示地形或未命中障碍刚体。
bool RaycastClosest(const float origin[3], const float dir[3], float maxDistance,
                    float& outHitX, float& outHitY, float& outHitZ,
                    int& outObstacleIndex);

/// 顶视图：自顶向下射线，用于 2D 模式下取表面高度与障碍命中。
bool RaycastVerticalTopDown(float wx, float wz, const float boundsMinMax[6],
                            float& outHitY, int& outObstacleIndex);

} // namespace PhysWorld
