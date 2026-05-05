#pragma once
/*
 * UI/Splitters.h
 * --------------
 * 可拖动分隔条小部件。
 *
 * 提供：
 *   VSplitter       — 垂直分隔条，调整 *左侧* 宽度
 *   VSplitterRight  — 垂直分隔条，调整 *右侧* 宽度
 *   HSplitterBottom — 水平分隔条，调整 *底部* 高度
 *
 * 用法：在 ImGui::InvisibleButton 所在行前后调用，绘制可拖动线条。
 * 返回值：当前帧是否处于拖动激活状态。
 *
 * 依赖：
 *   - imgui.h
 *
 * 不依赖：AppState.h、NavTypes.h
 */

#include "imgui.h"

namespace Splitters
{

/// 垂直分隔条 — 横向拖动改变 *左侧* 宽度（*width 为左宽）。
bool VSplitter(const char* id, float* width, float minW, float maxW,
               float thickness = 6.0f);

/// 垂直分隔条 — 横向拖动改变 *右侧* 宽度（向右拖则右侧变窄）。
bool VSplitterRight(const char* id, float* rightWidth, float minW, float maxW,
                    float thickness = 6.0f);

/// 水平分隔条 — 纵向拖动改变 *底部* 高度（向上拖则底部变高）。
bool HSplitterBottom(const char* id, float* height, float minH, float maxH,
                     float thickness = 6.0f);

} // namespace Splitters
