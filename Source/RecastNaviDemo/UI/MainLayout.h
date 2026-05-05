#pragma once
/*
 * UI/MainLayout.h
 * ---------------
 * 顶级 ImGui 布局：左侧面板 + 分隔条 + 右侧画布 + 底部性能趋势图窗口。
 *
 * 提供：
 *   MainLayout::OnGUI           — 每帧主绘制入口（由窗口系统回调调用）
 *   MainLayout::DrawCanvasPanel — 画布区域绘制（内部，也可单独调用）
 *
 * 依赖：
 *   - App/AppState.h
 *   - UI/Interaction.h   （HandleHotkeys、HandleCanvasInteraction、InteractionCallbacks）
 *   - UI/Panels.h        （DrawXxxPanel、PanelCallbacks）
 *   - UI/StatsWindow.h   （DrawBuildStatsWindow）
 *   - UI/Splitters.h     （VSplitter）
 *   - Render/Renderer2D.h（DrawCanvas2D、Draw2DParams）
 *   - Render/Renderer3D.h（DrawCanvas3D、Draw3DParams）
 *   - imgui.h
 *   - <algorithm>
 *
 * 不依赖：Nav/NavBuilder.h、Nav/NavQuery.h、loguru
 */

#include "../App/AppState.h"
#include "Interaction.h"
#include "Panels.h"
#include "StatsWindow.h"
#include "Splitters.h"
#include "../Render/Renderer2D.h"
#include "../Render/Renderer3D.h"
#include "imgui.h"

// =============================================================================
// 主布局回调
// =============================================================================
/// MainLayout 所需的全部回调：Interaction 和 Panel 回调打包在一起。
/// 调用方（main）只需填写一次，OnGUI 内部分发给各子模块。
struct MainLayoutCallbacks
{
    InteractionCallbacks Interaction;  ///< 传给 HandleHotkeys / HandleCanvasInteraction
    PanelCallbacks       Panels;       ///< 传给 DrawXxxPanel
};

// =============================================================================
// 主布局函数
// =============================================================================
namespace MainLayout
{

/// 绘制右侧画布区域（2D 或 3D）并处理鼠标交互。
/// 需在合法的 ImGui Child 窗口内调用，且 InvisibleButton 之前已布局完毕。
void DrawCanvasPanel(AppState& app, const InteractionCallbacks& icb);

/// 每帧顶级 ImGui 绘制。由窗口系统在每帧 ImGui::NewFrame() 之后调用。
void OnGUI(AppState& app, const MainLayoutCallbacks& cb);

} // namespace MainLayout
