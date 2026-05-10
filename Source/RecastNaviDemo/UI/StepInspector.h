#pragma once
/*
 * UI/StepInspector.h
 * ------------------
 * NavMesh 分步构建的"详情检视"窗口：可独立浮动（拖动 / 调大小），也可停靠在主窗口右侧。
 *
 * 显示内容（按 10 个步骤分别展开）：
 *   - 状态徽章   ：Pending / Current / Done / Failed
 *   - 算法说明   ：该步骤具体调用的 Recast/Detour API 与算法细节
 *   - 输出数据   ：步骤完成后产生的中间数据快照（spans 数、cell 数、轮廓数等）
 *   - 耗时       ：本会话中该步骤的最近一次执行时间（ms）
 *
 * 设计原则：
 *   - 与 StatsWindow 风格一致：通过 `app.bShowStepInspector` 总开关控制
 *   - 内容是只读视图；不直接调用 NavStepBuilder API（避免与左侧 Step Build 面板的状态不一致）
 *   - 提供 "Auto-scroll to current step" 选项：每次推进步骤时自动展开当前 / 上一步
 *
 * 依赖：
 *   - App/AppState.h    （AppState、NavStepBuilder::StepBuilder）
 *   - imgui.h
 */

#include "../App/AppState.h"
#include "imgui.h"

namespace StepInspector
{

/// 主入口：浮动模式时每帧绘制（`app.bStepInspectorDocked` 为 false 时）。MainLayout::OnGUI 末尾调用。
void Draw(AppState& app);

/// 停靠模式：在 MainLayout 右侧子窗口内绘制（带「浮动」切换按钮 + 正文）。
void DrawDocked(AppState& app);

} // namespace StepInspector
