#pragma once
/*
 * UI/Interaction.h
 * ----------------
 * 画布交互层：鼠标 / 键盘输入处理。
 *
 * 提供：
 *   - 射线拣取辅助函数（RayTriangle、RaycastInputMesh）
 *   - 屏幕坐标 → 世界坐标转换（ScreenTo2DWorld、ScreenTo3DGroundWorld、PickGroundWorld）
 *   - 障碍命中检测（FindObstacleAt）
 *   - 画布交互主循环（HandleCanvasInteraction）
 *   - 全局快捷键处理（HandleHotkeys）
 *
 * 调用约定：
 *   所有函数接受 AppState& + InteractionCallbacks const& 两个参数。
 *   回调结构体封装了所有会触发副作用的操作（构建 NavMesh、寻路、
 *   重建几何、帧相机等），避免此模块直接依赖 Nav/NavBuilder.h 等构建层。
 *
 * 依赖：
 *   - App/AppState.h  （AppState 完整定义）
 *   - Shared/MathUtils.h （Vec3、Cross、Dot、Normalize 等）
 *   - imgui.h
 *
 * 不依赖：Nav/NavBuilder.h、Nav/NavQuery.h、Render/、UI/Panels.h
 */

#include "../App/AppState.h"
#include "../Shared/MathUtils.h"
#include "imgui.h"

#include <functional>

// =============================================================================
// 交互回调（由 AppState 的拥有者注入）
// =============================================================================
struct InteractionCallbacks
{
    std::function<void()> BuildNavMesh;                  ///< (Re)Build NavMesh
    std::function<void()> FindPath;                      ///< Find path
    std::function<void()> TryAutoRebuild;                ///< 几何变化后按 bAutoRebuild 决定是否构建
    std::function<void()> TryAutoReplan;                 ///< 路径相关变化后按 bAutoReplan 决定是否寻路
    std::function<void()> RebuildProceduralInputGeometry;///< 重建程序化场景几何
    std::function<void()> FrameCameraToBounds;           ///< 将相机对准当前包围盒
};

namespace Interaction
{

// =============================================================================
// 低层射线拣取
// =============================================================================

/// Möller-Trumbore: 射线与三角形相交。命中时写入 outT（>0）并返回 true。
bool RayTriangle(const Vec3& orig, const Vec3& dir,
                 const Vec3& v0,   const Vec3& v1, const Vec3& v2,
                 float& outT);

/// 射线与 InputGeometry 全部三角形求交，返回最近命中的世界坐标。
bool RaycastInputMesh(const AppState& app,
                      const Vec3& orig, const Vec3& dir,
                      float& outX, float& outY, float& outZ);

// =============================================================================
// 屏幕 → 世界坐标
// =============================================================================

/// 2D 顶视图：将屏幕像素映射到世界 XZ 平面（Y 取包围盒中心）。
bool ScreenTo2DWorld(const AppState& app, const ImVec2& mp,
                     float& wx, float& wy, float& wz);

/// 3D 轨道视角：构造视图射线；优先与 OBJ 三角形求交，退回与 y=bmid 平面求交。
bool ScreenTo3DGroundWorld(const AppState& app, const ImVec2& mp,
                           float& wx, float& wy, float& wz);

/// 根据当前 ViewMode 选择 ScreenTo2DWorld 或 ScreenTo3DGroundWorld。
bool PickGroundWorld(const AppState& app, const ImVec2& mp,
                     float& wx, float& wy, float& wz);

// =============================================================================
// 障碍命中
// =============================================================================

/// 从后往前遍历 Obstacles，返回第一个包含 (wx, wz) 的障碍索引，找不到返回 -1。
int FindObstacleAt(const AppState& app, float wx, float wz);

/// PhysX 场景就绪时，用场景射线（3D 透视 / 2D 顶视竖直射线）解析障碍刚体；否则退回 FindObstacleAt。
int PickObstacleIndex(const AppState& app, const ImVec2& mousePos);

// =============================================================================
// 主交互入口
// =============================================================================

/// 处理画布区域的鼠标交互（需在 ImGui::InvisibleButton 之后调用）。
void HandleCanvasInteraction(AppState& app,
                             const InteractionCallbacks& cb,
                             const ImVec2& canvasItemMin,
                             const ImVec2& canvasItemSize);

/// 处理全局快捷键（需在每帧 ImGui 帧开始后调用）。
void HandleHotkeys(AppState& app, const InteractionCallbacks& cb);

} // namespace Interaction
