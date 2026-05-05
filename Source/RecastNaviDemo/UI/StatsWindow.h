#pragma once
/*
 * UI/StatsWindow.h
 * ----------------
 * 浮动性能趋势图窗口（底部停靠式）。
 *
 * 提供：
 *   DrawBuildStatsWindow  — 绘制底部浮动趋势图窗口（含 Build / Path 两 Tab）
 *
 * 内部辅助（也声明于此，供外部测试使用）：
 *   kBuildSeriesCount / kPathSeriesCount — 系列数常量
 *   TSeries<Stat>                        — 系列描述符模板
 *   GetBuildSeriesArray / GetPathSeriesArray — 系列描述符数组
 *   DrawTrendChartT<Stat>                — 趋势图模板（在 header 中定义）
 *   DrawStatsTabBody<Stat>               — tab 内容模板（在 header 中定义）
 *   DrawLegendItemRaw                    — 图例条目绘制（非模板）
 *   BuildTooltipExtra / PathTooltipExtra — tooltip 附加信息
 *
 * 依赖：
 *   - App/AppState.h
 *   - UI/Splitters.h
 *   - imgui.h
 *   - Shared/Profiling.h  （BuildStat / PathStat — 通过 AppState.h 间接包含）
 *   - <algorithm>, <cstdio>, <deque>, <functional>
 *
 * 不依赖：Nav/NavBuilder.h、Nav/NavQuery.h、loguru
 */

#include "../App/AppState.h"
#include "Splitters.h"
#include "imgui.h"

#include <algorithm>    // std::max / std::min
#include <cstdio>       // std::snprintf
#include <deque>
#include <functional>

// =============================================================================
// 系列数常量
// =============================================================================
inline constexpr int kBuildSeriesCount = 11;
inline constexpr int kPathSeriesCount  =  5;

// =============================================================================
// 系列描述符
// =============================================================================
template <typename Stat>
struct TSeries
{
    const char* Name;
    ImU32       Color;
    double (*Get)(const Stat&);
};

// 非模板系列数组（实现在 StatsWindow.cpp）
const TSeries<BuildStat>* GetBuildSeriesArray();
const TSeries<PathStat>*  GetPathSeriesArray();

// =============================================================================
// 非模板辅助函数声明（实现在 StatsWindow.cpp）
// =============================================================================

/// 图例条目（可点击切换 visible）
void DrawLegendItemRaw(int id, bool& visible,
                       const char* name, ImU32 color, float lastVal,
                       const char* unit = " ms");

/// tooltip 附加信息
void BuildTooltipExtra(const BuildStat& bs);
void PathTooltipExtra(const PathStat& ps);

/// 浮动趋势图窗口主入口
void DrawBuildStatsWindow(AppState& app);

// =============================================================================
// DrawTrendChartT — 模板定义（必须在 header）
// =============================================================================
template <typename Stat>
void DrawTrendChartT(const ImVec2& chartSize,
                     const std::deque<Stat>& history,
                     const TSeries<Stat>* series,
                     int seriesCount,
                     const bool* visible,
                     const char* emptyMsg,
                     const char* kindLabel,
                     void (*drawTooltipExtra)(const Stat&))
{
    ImDrawList*  draw = ImGui::GetWindowDrawList();
    const ImVec2 p0   = ImGui::GetCursorScreenPos();
    const ImVec2 p1   = ImVec2(p0.x + chartSize.x, p0.y + chartSize.y);
    ImGui::InvisibleButton("##chartArea", chartSize);
    const bool   hov  = ImGui::IsItemHovered();
    const ImVec2 mp   = ImGui::GetIO().MousePos;

    // 背景 + 边框
    draw->AddRectFilled(p0, p1, IM_COL32(20, 20, 24, 255), 4.0f);
    draw->AddRect      (p0, p1, IM_COL32(80, 80, 90, 255), 4.0f);

    const int n = static_cast<int>(history.size());
    if (n == 0)
    {
        const ImVec2 ts = ImGui::CalcTextSize(emptyMsg);
        draw->AddText(ImVec2(p0.x + (chartSize.x - ts.x) * 0.5f,
                             p0.y + (chartSize.y - ts.y) * 0.5f),
                      IM_COL32(160, 160, 160, 255), emptyMsg);
        return;
    }

    // 计算可见系列的全局 y 范围
    double yMax      = 0.0;
    bool   anyVisible = false;
    for (int s = 0; s < seriesCount; ++s)
    {
        if (!visible[s]) continue;
        anyVisible = true;
        for (int i = 0; i < n; ++i)
            yMax = std::max(yMax, series[s].Get(history[i]));
    }
    if (!anyVisible)
    {
        const char* msg = "(all series hidden - click legend to show)";
        const ImVec2 ts = ImGui::CalcTextSize(msg);
        draw->AddText(ImVec2(p0.x + (chartSize.x - ts.x) * 0.5f,
                             p0.y + (chartSize.y - ts.y) * 0.5f),
                      IM_COL32(160, 160, 160, 255), msg);
        return;
    }
    if (yMax <= 0.0) yMax = 1.0;
    yMax *= 1.08; // 顶部 8% padding

    constexpr float kAxisPadL = 44.0f;
    constexpr float kAxisPadB = 18.0f;
    constexpr float kAxisPadT =  6.0f;
    const ImVec2 plotMin(p0.x + kAxisPadL, p0.y + kAxisPadT);
    const ImVec2 plotMax(p1.x - 6.0f,      p1.y - kAxisPadB);
    const float  plotW = plotMax.x - plotMin.x;
    const float  plotH = plotMax.y - plotMin.y;

    // 网格 + y 轴标签（5 条横线）
    for (int g = 0; g <= 4; ++g)
    {
        const float t = g / 4.0f;
        const float y = plotMax.y - t * plotH;
        draw->AddLine(ImVec2(plotMin.x, y), ImVec2(plotMax.x, y), IM_COL32(60, 60, 70, 255));
        char lbl[32];
        std::snprintf(lbl, sizeof(lbl), "%.2f", yMax * t);
        draw->AddText(ImVec2(p0.x + 4, y - 7), IM_COL32(150, 150, 160, 255), lbl);
    }

    // x 轴标签
    {
        char lbl[32];
        std::snprintf(lbl, sizeof(lbl), "#%d", history.front().Index);
        draw->AddText(ImVec2(plotMin.x, plotMax.y + 2), IM_COL32(150, 150, 160, 255), lbl);
        std::snprintf(lbl, sizeof(lbl), "#%d", history.back().Index);
        const ImVec2 ts = ImGui::CalcTextSize(lbl);
        draw->AddText(ImVec2(plotMax.x - ts.x, plotMax.y + 2), IM_COL32(150, 150, 160, 255), lbl);
    }

    auto mapXY = [&](int idx, double val) -> ImVec2
    {
        const float x = (n == 1)
            ? (plotMin.x + plotW * 0.5f)
            : (plotMin.x + plotW * (idx / float(n - 1)));
        const float y = plotMax.y - static_cast<float>(val / yMax) * plotH;
        return ImVec2(x, std::max(plotMin.y, std::min(plotMax.y, y)));
    };

    int hoverIdx = -1;
    if (hov && n > 1)
    {
        const float relX = (mp.x - plotMin.x) / plotW;
        hoverIdx = static_cast<int>(std::round(relX * (n - 1)));
        hoverIdx = std::max(0, std::min(n - 1, hoverIdx));
        const float hx = plotMin.x + plotW * (hoverIdx / float(n - 1));
        draw->AddLine(ImVec2(hx, plotMin.y), ImVec2(hx, plotMax.y),
                      IM_COL32(100, 100, 120, 180));
    }
    else if (hov && n == 1)
    {
        hoverIdx = 0;
    }

    // 折线 + 终点圆点
    for (int s = 0; s < seriesCount; ++s)
    {
        if (!visible[s]) continue;
        for (int i = 1; i < n; ++i)
        {
            const ImVec2 pa = mapXY(i - 1, series[s].Get(history[i - 1]));
            const ImVec2 pb = mapXY(i,     series[s].Get(history[i]));
            draw->AddLine(pa, pb, series[s].Color, 2.0f);
        }
        const ImVec2 pLast = mapXY(n - 1, series[s].Get(history[n - 1]));
        draw->AddCircleFilled(pLast, 3.0f, series[s].Color);
    }

    // hover tooltip
    if (hoverIdx >= 0)
    {
        const Stat& cur = history[hoverIdx];
        ImGui::BeginTooltip();
        ImGui::Text("%s #%d  |  Total %.2f ms", kindLabel, cur.Index, cur.T.TotalMs);
        ImGui::Separator();
        for (int s = 0; s < seriesCount; ++s)
        {
            if (!visible[s]) continue;
            const ImVec4 col = ImGui::ColorConvertU32ToFloat4(series[s].Color);
            ImGui::ColorButton("##sw", col,
                               ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder,
                               ImVec2(10, 10));
            ImGui::SameLine();
            ImGui::Text("%-18s %8.3f ms", series[s].Name, series[s].Get(cur));
        }
        if (drawTooltipExtra) drawTooltipExtra(cur);
        ImGui::EndTooltip();
    }
}

// =============================================================================
// DrawStatsTabBody — 模板定义（必须在 header）
// =============================================================================
template <typename Stat>
void DrawStatsTabBody(const std::deque<Stat>& history,
                      const TSeries<Stat>* series,
                      int seriesCount,
                      bool* visible,
                      const char* emptyMsg,
                      const char* kindLabel,
                      void (*drawTooltipExtra)(const Stat&),
                      const char* sideHeader,
                      std::function<void(const Stat&)> drawSidePanel,
                      const char* splitterId,
                      float& legendWidth)
{
    const ImVec2 avail       = ImGui::GetContentRegionAvail();
    const float  kSplitterW  = 6.0f;
    const float  maxLegendW  = std::max(120.0f, avail.x - 200.0f - kSplitterW);
    legendWidth = std::max(120.0f, std::min(maxLegendW, legendWidth));
    const ImVec2 chartSize(avail.x - legendWidth - kSplitterW, avail.y);

    DrawTrendChartT<Stat>(chartSize, history, series, seriesCount, visible,
                          emptyMsg, kindLabel, drawTooltipExtra);

    ImGui::SameLine(0.0f, 0.0f);
    Splitters::VSplitterRight(splitterId, &legendWidth, 120.0f, maxLegendW, kSplitterW);
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::BeginChild("##legend", ImVec2(legendWidth, avail.y), false);
    ImGui::TextDisabled("Legend (click to toggle)");
    ImGui::Separator();
    for (int s = 0; s < seriesCount; ++s)
    {
        const float lastVal = history.empty()
            ? 0.0f
            : static_cast<float>(series[s].Get(history.back()));
        DrawLegendItemRaw(s, visible[s], series[s].Name, series[s].Color, lastVal);
    }
    if (sideHeader && drawSidePanel)
    {
        ImGui::Spacing();
        ImGui::TextDisabled("%s", sideHeader);
        ImGui::Separator();
        if (!history.empty()) drawSidePanel(history.back());
    }
    ImGui::EndChild();
}
