#pragma once
/*
 * UI/Gizmo.h
 * ----------
 * Unreal Editor 风格的 3 轴 Widget：选中障碍后在其中心叠加 ImGuizmo
 * 提供 Translate / Rotate-Y / Scale 操作。
 *
 * 工作流：
 *   1) 上层布局 (MainLayout::DrawCanvasPanel) 在 3D 渲染完成、且选中合法障碍时调用 Draw()；
 *   2) Draw() 把世界 ViewMatrix / ProjMatrix 转为 ImGuizmo 期望的列优先 float[16];
 *   3) 由 Obstacle 的 (CX, BaseY+H/2, CZ) + YawDeg + (SX/Height/SZ) 合成模型矩阵；
 *   4) 调用 ImGuizmo::Manipulate; 若用户正在拖拽，再分解 model 矩阵回写 obstacle 字段。
 *
 * 设计取舍：
 *   - 旋转仅暴露 Y 轴（最常用、可干净映射到 YawDeg）；ImGuizmo 操作枚举使用 ROTATE_Y。
 *   - Cylinder 形状：YawDeg 不影响其几何（关于 Y 对称），但缩放仍然作用于 Radius / Height。
 *   - LOCAL / WORLD 模式：Translate / Scale 在 LOCAL 下沿 Yaw 方向；Rotate 不区分。
 *
 * 输入冲突避免：
 *   - 当 ImGuizmo::IsUsing() 时，主画布的 Pan / Rotate / Pick 应被忽略，
 *     由 Interaction.cpp 通过 ImGuizmo::IsOver() 判断（外部 short-circuit）。
 *
 * 依赖：
 *   - App/AppState.h
 *   - Thirdparty/ImGuizmo
 */

#include "../App/AppState.h"
#include "imgui.h"

namespace Gizmo
{

/// 当前是否有 gizmo 处于鼠标 hover 上（用于让画布交互让位）。
bool IsOverGizmo();

/// 当前是否正在被用户拖拽。
bool IsUsingGizmo();

/// 在每帧 ImGui::NewFrame 之后必须调用一次（同 ImGuizmo 文档建议）。
void OnFrameBegin();

/// 在 3D 视口绘制完成后调用：根据 app 状态在选中障碍中心绘制 widget。
/// canvasMin / canvasSize 与 DrawCanvasPanel 保持一致（屏幕空间）。
/// 若画布回写 obstacle 字段，将设置 app.bGeomDirty = true 以触发自动重建。
void Draw(AppState& app, const ImVec2& canvasMin, const ImVec2& canvasSize);

/// 处理热键：W=平移 / E=旋转 / R=缩放 / Q=隐藏 (类似 Unreal Level Editor)
/// 仅在画布悬浮 / 焦点时触发，避免影响输入框。
void HandleHotkeys(AppState& app);

} // namespace Gizmo
