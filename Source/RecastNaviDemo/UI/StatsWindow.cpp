/*
 * UI/StatsWindow.cpp
 * ------------------
 * 浮动性能趋势图窗口实现（非模板部分）。
 * 模板函数 DrawTrendChartT 和 DrawStatsTabBody 定义在 StatsWindow.h。
 */

#include "StatsWindow.h"

#include <cstdio>       // std::snprintf
#include <algorithm>    // std::max / std::min

// =============================================================================
// GetBuildSeriesArray — 11 个构建阶段系列描述符
// =============================================================================
const TSeries<BuildStat>* GetBuildSeriesArray()
{
    static const TSeries<BuildStat> arr[] = {
        { "Total",           IM_COL32(255, 245, 120, 255), [](const BuildStat& b){ return b.T.TotalMs;        } },
        { "Rasterize",       IM_COL32( 80, 180, 255, 255), [](const BuildStat& b){ return b.T.RasterizeMs;    } },
        { "Filter",          IM_COL32(255, 130,  80, 255), [](const BuildStat& b){ return b.T.FilterMs;       } },
        { "CompactHF",       IM_COL32(120, 230, 130, 255), [](const BuildStat& b){ return b.T.CompactMs;      } },
        { "Erode",           IM_COL32(220, 110, 200, 255), [](const BuildStat& b){ return b.T.ErodeMs;        } },
        { "DistField",       IM_COL32(  0, 200, 200, 255), [](const BuildStat& b){ return b.T.DistFieldMs;    } },
        { "Regions",         IM_COL32(255,  90, 110, 255), [](const BuildStat& b){ return b.T.RegionsMs;      } },
        { "Contours",        IM_COL32(180, 160, 255, 255), [](const BuildStat& b){ return b.T.ContoursMs;     } },
        { "PolyMesh",        IM_COL32(255, 200,  60, 255), [](const BuildStat& b){ return b.T.PolyMeshMs;     } },
        { "DetailMesh",      IM_COL32(140, 220,  90, 255), [](const BuildStat& b){ return b.T.DetailMeshMs;   } },
        { "dtCreateNavMesh", IM_COL32(200, 200, 200, 255), [](const BuildStat& b){ return b.T.DetourCreateMs; } },
    };
    return arr;
}

// =============================================================================
// GetPathSeriesArray — 5 个寻路阶段系列描述符
// =============================================================================
const TSeries<PathStat>* GetPathSeriesArray()
{
    static const TSeries<PathStat> arr[] = {
        { "Total",            IM_COL32(255, 245, 120, 255), [](const PathStat& p){ return p.T.TotalMs;        } },
        { "FindNearestStart", IM_COL32( 80, 180, 255, 255), [](const PathStat& p){ return p.T.NearestStartMs; } },
        { "FindNearestEnd",   IM_COL32(255, 130,  80, 255), [](const PathStat& p){ return p.T.NearestEndMs;   } },
        { "FindPath",         IM_COL32(120, 230, 130, 255), [](const PathStat& p){ return p.T.FindPathMs;     } },
        { "StraightPath",     IM_COL32(220, 110, 200, 255), [](const PathStat& p){ return p.T.StraightPathMs; } },
    };
    return arr;
}

// =============================================================================
// DrawLegendItemRaw
// =============================================================================
void DrawLegendItemRaw(int id, bool& visible,
                       const char* name, ImU32 color, float lastVal,
                       const char* unit)
{
    const float lh = ImGui::GetTextLineHeightWithSpacing();

    char buf[96];
    std::snprintf(buf, sizeof(buf), "%-17s %7.2f%s", name, lastVal, unit);

    const ImVec2 textSize = ImGui::CalcTextSize(buf);
    const ImVec2 itemSize(textSize.x + 22.0f, lh);

    const ImVec2 cur = ImGui::GetCursorScreenPos();
    ImGui::PushID(id);
    ImGui::InvisibleButton("##leg", itemSize);
    const bool clicked = ImGui::IsItemClicked();
    const bool hovered = ImGui::IsItemHovered();
    ImGui::PopID();

    if (clicked) visible = !visible;

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 boxA(cur.x + 2,  cur.y + (lh - 12) * 0.5f);
    const ImVec2 boxB(cur.x + 14, cur.y + (lh - 12) * 0.5f + 12);
    if (visible)
        draw->AddRectFilled(boxA, boxB, color, 2.0f);
    else
        draw->AddRect(boxA, boxB, (color & 0x00FFFFFF) | 0x80000000, 2.0f, 0, 1.5f);
    const ImU32 tc = visible ? IM_COL32(230, 230, 235, 255)
                              : IM_COL32(140, 140, 145, 255);
    draw->AddText(ImVec2(cur.x + 18, cur.y + 1), tc, buf);
    if (hovered)
        draw->AddRect(ImVec2(cur.x, cur.y),
                      ImVec2(cur.x + itemSize.x, cur.y + itemSize.y),
                      IM_COL32(120, 120, 140, 255), 2.0f);
}

// =============================================================================
// BuildTooltipExtra / PathTooltipExtra
// =============================================================================
void BuildTooltipExtra(const BuildStat& bs)
{
    ImGui::Separator();
    ImGui::Text("Polys=%d  DetailTris=%d  NavData=%.1f KB",
                bs.PolyCount, bs.DetailTris, bs.NavDataBytes / 1024.0f);
}

void PathTooltipExtra(const PathStat& ps)
{
    ImGui::Separator();
    ImGui::Text("Status: %s", ps.Status.c_str());
    ImGui::Text("Corridor=%d polys   Straight=%d corners",
                ps.CorridorPolys, ps.StraightCorners);
}

// =============================================================================
// DrawBuildStatsWindow
// =============================================================================
void DrawBuildStatsWindow(AppState& app)
{
    if (!app.bShowStatsWindow) return;

    // 各 tab 系列可见状态（跨帧持久化）
    static bool sBuildVisible[kBuildSeriesCount] = {
        true, true, true, true, true, true, true, true, true, true, true
    };
    static bool sPathVisible[kPathSeriesCount] = { true, true, true, true, true };

    const ImGuiViewport* vp = ImGui::GetMainViewport();

    // 永久贴主视口底部：全宽 + 固定高（高度由顶部水平 splitter 拖动）
    const float h = app.StatsDockHeight;
    ImGui::SetNextWindowPos (ImVec2(vp->WorkPos.x, vp->WorkPos.y + vp->WorkSize.y - h));
    ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x, h));
    constexpr ImGuiWindowFlags wflags =
          ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (!ImGui::Begin("Performance Stats / 性能趋势", &app.bShowStatsWindow, wflags))
    {
        ImGui::End();
        return;
    }

    // 顶部水平 splitter（拖动调整窗口高度）
    {
        const float maxDockH = std::max(200.0f, vp->WorkSize.y - 200.0f);
        Splitters::HSplitterBottom("##split_dock", &app.StatsDockHeight, 160.0f, maxDockH, 6.0f);
    }

    if (ImGui::BeginTabBar("##stats_tabs"))
    {
        // ===== Tab 1: NavMesh Build =====
        if (ImGui::BeginTabItem("NavMesh Build / 构建"))
        {
            const int n = static_cast<int>(app.BuildHistory.size());
            if (n > 0)
            {
                const BuildStat& last = app.BuildHistory.back();
                ImGui::Text("Builds: %d   Last #%d   Total: %.2f ms   Polys: %d   NavData: %.1f KB",
                            n, last.Index, last.T.TotalMs, last.PolyCount,
                            last.NavDataBytes / 1024.0f);
            }
            else
            {
                ImGui::TextUnformatted("(no builds yet - press R or click Build)");
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("All##b"))  { for (bool& v : sBuildVisible) v = true; }
            ImGui::SameLine();
            if (ImGui::SmallButton("None##b")) { for (bool& v : sBuildVisible) v = false; }
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60);
            if (ImGui::SmallButton("Clear##b"))
            {
                app.BuildHistory.clear();
                app.BuildCounter = 0;
            }
            ImGui::Separator();

            auto buildSidePanel = [](const BuildStat& last)
            {
                ImGui::Text("Polys     : %d", last.PolyCount);
                ImGui::Text("PolyVerts : %d", last.PolyVerts);
                ImGui::Text("DetailTris: %d", last.DetailTris);
                ImGui::Text("NavData   : %.1f KB", last.NavDataBytes / 1024.0f);
                ImGui::Text("InputTris : %d", last.InputTris);
            };
            DrawStatsTabBody<BuildStat>(app.BuildHistory,
                                        GetBuildSeriesArray(), kBuildSeriesCount,
                                        sBuildVisible,
                                        "(no builds yet - press R or click Build)",
                                        "Build",
                                        &BuildTooltipExtra,
                                        "Output sizes (last)",
                                        buildSidePanel,
                                        "##split_legend_b",
                                        app.StatsLegendWidth);
            ImGui::EndTabItem();
        }

        // ===== Tab 2: Path Query =====
        if (ImGui::BeginTabItem("Path Query / 寻路"))
        {
            const int n = static_cast<int>(app.PathHistory.size());
            if (n > 0)
            {
                const PathStat& last = app.PathHistory.back();
                ImGui::Text("Queries: %d   Last #%d   Total: %.3f ms   %s",
                            n, last.Index, last.T.TotalMs,
                            last.bOk ? "OK" : "FAIL");
            }
            else
            {
                ImGui::TextUnformatted("(no path queries yet - press F or click Find Path)");
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("All##p"))  { for (bool& v : sPathVisible) v = true; }
            ImGui::SameLine();
            if (ImGui::SmallButton("None##p")) { for (bool& v : sPathVisible) v = false; }
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60);
            if (ImGui::SmallButton("Clear##p"))
            {
                app.PathHistory.clear();
                app.PathCounter = 0;
            }
            ImGui::Separator();

            auto pathSidePanel = [](const PathStat& last)
            {
                ImGui::Text("Status     : %s", last.Status.c_str());
                ImGui::Text("Corridor   : %d polys", last.CorridorPolys);
                ImGui::Text("Straight   : %d corners", last.StraightCorners);
                ImGui::Text("Result     : %s", last.bOk ? "OK" : "FAIL");
            };
            DrawStatsTabBody<PathStat>(app.PathHistory,
                                       GetPathSeriesArray(), kPathSeriesCount,
                                       sPathVisible,
                                       "(no path queries yet - press F or click Find Path)",
                                       "Path",
                                       &PathTooltipExtra,
                                       "Last query",
                                       pathSidePanel,
                                       "##split_legend_p",
                                       app.StatsLegendWidth);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}
