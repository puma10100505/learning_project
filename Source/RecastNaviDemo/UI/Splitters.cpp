/*
 * UI/Splitters.cpp
 * ----------------
 * 可拖动分隔条实现。详细说明见 Splitters.h。
 */

#include "Splitters.h"

#include <algorithm>   // std::max / std::min

namespace Splitters
{

bool VSplitter(const char* id, float* width, float minW, float maxW, float thickness)
{
    ImGui::InvisibleButton(id, ImVec2(thickness, ImGui::GetContentRegionAvail().y));
    const bool active  = ImGui::IsItemActive();
    const bool hovered = ImGui::IsItemHovered();
    if (hovered || active) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    if (active)
    {
        *width += ImGui::GetIO().MouseDelta.x;
        *width = std::max(minW, std::min(maxW, *width));
    }
    ImDrawList*  draw = ImGui::GetWindowDrawList();
    const ImVec2 a    = ImGui::GetItemRectMin();
    const ImVec2 b    = ImGui::GetItemRectMax();
    const ImU32  col  = active  ? IM_COL32(120, 160, 220, 255)
                      : hovered ? IM_COL32(100, 130, 180, 200)
                                : IM_COL32( 70,  70,  80, 200);
    const float midX = (a.x + b.x) * 0.5f;
    draw->AddLine(ImVec2(midX, a.y + 4), ImVec2(midX, b.y - 4), col, 2.0f);
    return active;
}

bool VSplitterRight(const char* id, float* rightWidth, float minW, float maxW, float thickness)
{
    ImGui::InvisibleButton(id, ImVec2(thickness, ImGui::GetContentRegionAvail().y));
    const bool active  = ImGui::IsItemActive();
    const bool hovered = ImGui::IsItemHovered();
    if (hovered || active) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    if (active)
    {
        *rightWidth -= ImGui::GetIO().MouseDelta.x;
        *rightWidth = std::max(minW, std::min(maxW, *rightWidth));
    }
    ImDrawList*  draw = ImGui::GetWindowDrawList();
    const ImVec2 a    = ImGui::GetItemRectMin();
    const ImVec2 b    = ImGui::GetItemRectMax();
    const ImU32  col  = active  ? IM_COL32(120, 160, 220, 255)
                      : hovered ? IM_COL32(100, 130, 180, 200)
                                : IM_COL32( 70,  70,  80, 200);
    const float midX = (a.x + b.x) * 0.5f;
    draw->AddLine(ImVec2(midX, a.y + 4), ImVec2(midX, b.y - 4), col, 2.0f);
    return active;
}

bool HSplitterBottom(const char* id, float* height, float minH, float maxH, float thickness)
{
    ImGui::InvisibleButton(id, ImVec2(ImGui::GetContentRegionAvail().x, thickness));
    const bool active  = ImGui::IsItemActive();
    const bool hovered = ImGui::IsItemHovered();
    if (hovered || active) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    if (active)
    {
        *height -= ImGui::GetIO().MouseDelta.y;
        *height = std::max(minH, std::min(maxH, *height));
    }
    ImDrawList*  draw = ImGui::GetWindowDrawList();
    const ImVec2 a    = ImGui::GetItemRectMin();
    const ImVec2 b    = ImGui::GetItemRectMax();
    const ImU32  col  = active  ? IM_COL32(120, 160, 220, 255)
                      : hovered ? IM_COL32(100, 130, 180, 200)
                                : IM_COL32( 70,  70,  80, 200);
    const float midY = (a.y + b.y) * 0.5f;
    draw->AddLine(ImVec2(a.x + 4, midY), ImVec2(b.x - 4, midY), col, 2.0f);
    return active;
}

} // namespace Splitters
