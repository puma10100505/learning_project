/*
 * UI/Panels.cpp
 * -------------
 * ImGui 左侧各折叠面板绘制实现。
 * 详细说明见 Panels.h。
 */

#include "Panels.h"

#include <cmath>
#include <cstring>  // strncpy
#include <string>
#include <algorithm>

// MAX_PATH 在 Windows 以外不一定有定义
#ifndef MAX_PATH
#  ifdef PATH_MAX
#    define MAX_PATH PATH_MAX
#  else
#    define MAX_PATH 4096
#  endif
#endif

namespace Panels
{

// =============================================================================
// DrawViewPanel
// =============================================================================
void DrawViewPanel(AppState& app, const PanelCallbacks& cb)
{
    if (!ImGui::CollapsingHeader("View", ImGuiTreeNodeFlags_DefaultOpen)) return;

    int vm = (app.CurrentViewMode == ViewMode::TopDown2D) ? 0 : 1;
    ImGui::TextUnformatted("View Mode:");
    ImGui::SameLine();
    if (ImGui::RadioButton("2D Top",   &vm, 0)) app.CurrentViewMode = ViewMode::TopDown2D;
    ImGui::SameLine();
    if (ImGui::RadioButton("3D Orbit", &vm, 1)) app.CurrentViewMode = ViewMode::Orbit3D;

    ImGui::Checkbox("Grid",          &app.View.bShowGrid);         ImGui::SameLine();
    ImGui::Checkbox("Input mesh",    &app.View.bShowInputMesh);    ImGui::SameLine();
    ImGui::Checkbox("Obstacles",     &app.View.bShowObstacles);
    ImGui::Checkbox("Fill polygons", &app.View.bFillPolygons);     ImGui::SameLine();
    ImGui::Checkbox("Region colors", &app.View.bRegionColors);
    ImGui::Checkbox("Poly edges",    &app.View.bShowPolyEdges);    ImGui::SameLine();
    ImGui::Checkbox("Detail mesh",   &app.View.bShowDetailMesh);
    ImGui::Checkbox("Path corridor", &app.View.bShowPathCorridor); ImGui::SameLine();
    ImGui::Checkbox("Auto replan",   &app.View.bAutoReplan);
    ImGui::Checkbox("Auto rebuild",  &app.View.bAutoRebuild);       ImGui::SameLine();
    ImGui::Checkbox("Stats Window",  &app.bShowStatsWindow);
    ImGui::Checkbox("Build Volume",  &app.View.bShowBuildVolume);

    if (ImGui::Button("Reset Camera (Home, 重置视角到包围盒)", ImVec2(-FLT_MIN, 0)))
    {
        if (cb.FrameCameraToBounds) cb.FrameCameraToBounds();
    }

    ImGui::Spacing();
    ImGui::ColorEdit3("##pathcol", &app.PathColor.x,
                      ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel |
                      ImGuiColorEditFlags_PickerHueWheel);
    ImGui::SameLine();
    ImGui::TextUnformatted("Path Color");

    ImGui::ColorEdit4("##corridorcol", &app.CorridorColor.x,
                      ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel |
                      ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_AlphaBar);
    ImGui::SameLine();
    ImGui::TextUnformatted("Corridor Color (RGBA)");

    if (app.CurrentViewMode == ViewMode::Orbit3D)
    {
        ImGui::Separator();
        ImGui::TextUnformatted("Orbit Camera (Right-drag to rotate, wheel to zoom)");
        ImGui::SliderFloat("Yaw",      &app.Cam.Yaw,      -3.14f, 3.14f);
        ImGui::SliderFloat("Pitch",    &app.Cam.Pitch,    -1.50f, 1.50f);
        ImGui::SliderFloat("Distance", &app.Cam.Distance,  2.00f, 800.0f);
    }
}

// =============================================================================
// DrawScenePanel
// =============================================================================
void DrawScenePanel(AppState& app, const PanelCallbacks& cb)
{
    if (!ImGui::CollapsingHeader("Scene / 场景", ImGuiTreeNodeFlags_DefaultOpen)) return;

    if (app.Geom.Source != GeomSource::Procedural)
    {
        ImGui::TextDisabled(" (Scene size only applies in Procedural mode)");
        ImGui::TextDisabled(" Current source: OBJ file");
        return;
    }

    ImGui::TextDisabled(" 场景以原点为中心, 占地 (2 * HalfSize)^2 (单位: 米)");
    ImGui::PushItemWidth(-FLT_MIN);

    bool changed = false;

    float halfSize = app.Scene.HalfSize;
    if (ImGui::SliderFloat("Half Size", &halfSize, 5.0f, 200.0f, "%.1f m"))
    {
        app.Scene.HalfSize = halfSize;
        changed = true;
    }

    int cells = app.Scene.GridCells;
    if (ImGui::SliderInt("Grid Cells", &cells, 4, 400))
    {
        app.Scene.GridCells = cells;
        changed = true;
    }

    ImGui::TextDisabled(" 三角形数: %d  (%dx%d 网格 x 2)",
                        cells * cells * 2, cells, cells);
    ImGui::TextDisabled(" 网格步长: %.2f m  (越小精度越高, 构建越慢)",
                        (app.Scene.HalfSize * 2.0f) / std::max(1, cells));

    ImGui::PopItemWidth();

    // 一键预设
    auto Preset = [&](const char* label, float h, int c)
    {
        if (ImGui::SmallButton(label))
        {
            app.Scene.HalfSize  = h;
            app.Scene.GridCells = c;
            changed = true;
        }
    };
    Preset("Tiny 10",    5.0f,   20);  ImGui::SameLine();
    Preset("Small 30",  15.0f,   30);  ImGui::SameLine();
    Preset("Medium 60", 30.0f,   60);  ImGui::SameLine();
    Preset("Large 120", 60.0f,  120);  ImGui::SameLine();
    Preset("Huge 300",  150.0f, 300);

    if (ImGui::Checkbox("Obstacle solid mesh##solidmesh", &app.Scene.bObstacleSolid))
    {
        if (cb.RebuildProceduralInputGeometry) cb.RebuildProceduralInputGeometry();
        app.bGeomDirty = true;
        if (cb.TryAutoRebuild) cb.TryAutoRebuild();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(顶面可走)");

    if (changed)
    {
        if (cb.ClampAllObstaclesToScene) cb.ClampAllObstaclesToScene(app.Scene.HalfSize);
        const float maxAbs = std::max(0.0f, app.Scene.HalfSize - 1.0f);
        app.Start[0] = std::max(-maxAbs, std::min(maxAbs, app.Start[0]));
        app.Start[2] = std::max(-maxAbs, std::min(maxAbs, app.Start[2]));
        app.End  [0] = std::max(-maxAbs, std::min(maxAbs, app.End  [0]));
        app.End  [2] = std::max(-maxAbs, std::min(maxAbs, app.End  [2]));

        if (cb.RebuildProceduralInputGeometry) cb.RebuildProceduralInputGeometry();
        if (cb.FrameCameraToBounds)            cb.FrameCameraToBounds();
        app.bGeomDirty = true;
        if (cb.TryAutoRebuild) cb.TryAutoRebuild();
    }
}

// =============================================================================
// DrawEditPanel
// =============================================================================
void DrawEditPanel(AppState& app, const PanelCallbacks& cb)
{
    if (!ImGui::CollapsingHeader("Edit Mode", ImGuiTreeNodeFlags_DefaultOpen)) return;

    const char* items[] = {
        "None", "Place Start", "Place End",
        "Create Obstacle", "Select & Move", "Delete Obstacle"
    };
    int cur = static_cast<int>(app.CurrentEditMode);
    if (ImGui::Combo("##edit", &cur, items, IM_ARRAYSIZE(items)))
        app.CurrentEditMode = static_cast<EditMode>(cur);

    // 快捷键提示
    if (ImGui::TreeNode("Hotkeys / 快捷键"))
    {
        ImGui::TextDisabled("--- 场景导航 (3D) ---");
        ImGui::BulletText("W/A/S/D : 沿相机方向 前/左/后/右 平移");
        ImGui::BulletText("Q / E   : 下降 / 上升 (Y 轴)");
        ImGui::BulletText("Shift   : 加速 ×3   |   Ctrl: 慢速 ×0.3");
        ImGui::BulletText("右键拖拽: 旋转视角  |   滚轮: 缩放");
        ImGui::BulletText("Home    : 重置 Camera (拉到包围盒)");
        ImGui::TextDisabled("--- 编辑 / 操作 ---");
        ImGui::BulletText("1 / 2   : Place Start / End");
        ImGui::BulletText("T       : Select & Move (原 S, 让位 WASD)");
        ImGui::BulletText("B / C   : New Box / Cylinder");
        ImGui::BulletText("Del / X : 删除当前选中障碍");
        ImGui::BulletText("R       : (Re)Build NavMesh");
        ImGui::BulletText("F       : Find Path");
        ImGui::BulletText("V       : 切换 2D / 3D");
        ImGui::BulletText("Esc     : 取消选择 / 模式归 None");
        ImGui::TreePop();
    }

    if (app.Geom.Source == GeomSource::ObjFile &&
        (app.CurrentEditMode == EditMode::CreateBox ||
         app.CurrentEditMode == EditMode::MoveBox   ||
         app.CurrentEditMode == EditMode::DeleteBox))
    {
        ImGui::TextColored(ImVec4(1, 0.6f, 0.4f, 1),
                           "Obstacle editing only works in Procedural mode");
    }

    // 当前选择状态显示
    if (app.SelectedObstacle >= 0 &&
        app.SelectedObstacle < static_cast<int>(app.Geom.Obstacles.size()))
    {
        const Obstacle& o = app.Geom.Obstacles[app.SelectedObstacle];
        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.4f, 1.0f),
                           "Selected: #%d [%s]  center=(%.2f, %.2f)  h=%.2f",
                           app.SelectedObstacle,
                           o.Shape == ObstacleShape::Box ? "Box" : "Cylinder",
                           o.CX, o.CZ, o.Height);
    }
    else
    {
        ImGui::TextDisabled("Selected: (none)");
    }

    // 新建障碍属性
    const char* shapeItems[] = { "Box", "Cylinder" };
    int shapeIdx = static_cast<int>(app.DefaultObstacleShape);
    if (ImGui::Combo("New Shape", &shapeIdx, shapeItems, IM_ARRAYSIZE(shapeItems)))
        app.DefaultObstacleShape = static_cast<ObstacleShape>(shapeIdx);
    ImGui::SliderFloat("New Height", &app.DefaultObstacleHeight, 0.30f, 8.00f);
    ImGui::TextDisabled("Drag on canvas: Box=corner-to-corner, Cylinder=center-to-edge");

    ImGui::TextDisabled("Obstacles: %d", static_cast<int>(app.Geom.Obstacles.size()));
    if (ImGui::Button("Clear All Obstacles", ImVec2(-FLT_MIN, 0)))
    {
        if (app.Geom.Source == GeomSource::Procedural)
        {
            app.Geom.Obstacles.clear();
            if (cb.RebuildProceduralInputGeometry) cb.RebuildProceduralInputGeometry();
            app.bGeomDirty = true;
            if (cb.TryAutoRebuild) cb.TryAutoRebuild();
        }
    }
    if (ImGui::Button("Reset Geometry", ImVec2(-FLT_MIN, 0)))
    {
        if (cb.DestroyNavRuntime)  cb.DestroyNavRuntime();
        if (cb.InitDefaultGeometry) cb.InitDefaultGeometry();
        if (cb.TryAutoRebuild)     cb.TryAutoRebuild();
    }

    // 障碍列表（可调高度 / 删除）
    if (app.Geom.Source == GeomSource::Procedural && !app.Geom.Obstacles.empty())
    {
        ImGui::Spacing();
        ImGui::TextUnformatted("Obstacle list:");
        if (ImGui::BeginChild("##oblist", ImVec2(0, 160), true,
                              ImGuiWindowFlags_HorizontalScrollbar))
        {
            int  toDelete = -1;
            bool listChanged = false;
            for (size_t i = 0; i < app.Geom.Obstacles.size(); ++i)
            {
                Obstacle& o = app.Geom.Obstacles[i];
                ImGui::PushID(static_cast<int>(i));
                const char* tag = (o.Shape == ObstacleShape::Box) ? "B" : "C";
                if (o.Shape == ObstacleShape::Box)
                    ImGui::Text("%02d [%s] (%.1f,%.1f) sx=%.1f sz=%.1f",
                                static_cast<int>(i), tag, o.CX, o.CZ, o.SX, o.SZ);
                else
                    ImGui::Text("%02d [%s] (%.1f,%.1f) r=%.2f",
                                static_cast<int>(i), tag, o.CX, o.CZ, o.Radius);
                ImGui::SetNextItemWidth(140);
                if (ImGui::SliderFloat("h", &o.Height, 0.20f, 8.00f, "%.2f"))
                    listChanged = true;
                ImGui::SameLine();
                if (ImGui::SmallButton("del")) toDelete = static_cast<int>(i);
                ImGui::PopID();
            }
            if (toDelete >= 0)
            {
                app.Geom.Obstacles.erase(app.Geom.Obstacles.begin() + toDelete);
                listChanged = true;
            }
            if (listChanged)
            {
                if (cb.RebuildProceduralInputGeometry) cb.RebuildProceduralInputGeometry();
                app.bGeomDirty = true;
                if (cb.TryAutoRebuild) cb.TryAutoRebuild();
            }
        }
        ImGui::EndChild();
    }

    // OffMeshLink 列表
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextUnformatted("OffMeshLinks:");
    ImGui::SameLine();
    ImGui::TextDisabled("(%d)", static_cast<int>(app.Geom.OffMeshLinks.size()));

    if (ImGui::Button("+ Add Link##oml", ImVec2(-FLT_MIN, 0)))
    {
        app.CurrentEditMode = EditMode::PlaceOffMeshStart;
    }

    if (!app.Geom.OffMeshLinks.empty())
    {
        if (ImGui::BeginChild("##omllist", ImVec2(0, 120), true,
                              ImGuiWindowFlags_HorizontalScrollbar))
        {
            int toLinkDelete = -1;
            for (size_t i = 0; i < app.Geom.OffMeshLinks.size(); ++i)
            {
                OffMeshLink& lk = app.Geom.OffMeshLinks[i];
                ImGui::PushID(static_cast<int>(i + 1000));
                ImGui::Text("#%02d S(%.1f,%.1f,%.1f) E(%.1f,%.1f,%.1f) r=%.2f",
                            static_cast<int>(i),
                            lk.Start[0], lk.Start[1], lk.Start[2],
                            lk.End  [0], lk.End  [1], lk.End  [2],
                            lk.Radius);
                ImGui::SetNextItemWidth(90);
                ImGui::SameLine();
                ImGui::DragFloat("##r", &lk.Radius, 0.05f, 0.1f, 5.0f, "r=%.2f");
                ImGui::SameLine();
                if (ImGui::SmallButton("del##oml")) toLinkDelete = static_cast<int>(i);
                ImGui::PopID();
            }
            if (toLinkDelete >= 0)
            {
                app.Geom.OffMeshLinks.erase(app.Geom.OffMeshLinks.begin() + toLinkDelete);
                app.bGeomDirty = true;
            }
        }
        ImGui::EndChild();
        if (ImGui::Button("Clear All Links##oml", ImVec2(-FLT_MIN, 0)))
        {
            app.Geom.OffMeshLinks.clear();
            app.bGeomDirty = true;
        }
    }
}

// =============================================================================
// DrawBuildPanel
// =============================================================================
void DrawBuildPanel(AppState& app, const PanelCallbacks& cb)
{
    if (!ImGui::CollapsingHeader("Build Config", ImGuiTreeNodeFlags_DefaultOpen)) return;

    auto Desc = [](const char* zh) { ImGui::TextDisabled(" %s", zh); };

    ImGui::PushItemWidth(-FLT_MIN);

    Desc("体素水平尺寸 (XZ, 单位: 世界米) - 越小精度越高");
    ImGui::SliderFloat("Cell Size",         &app.Config.CellSize,         0.10f,  1.00f);

    Desc("体素垂直尺寸 (Y, 单位: 世界米)");
    ImGui::SliderFloat("Cell Height",       &app.Config.CellHeight,       0.10f,  1.00f);

    Desc("角色高度 (米) - 用于头顶净空判定");
    ImGui::SliderFloat("Agent Height",      &app.Config.AgentHeight,      0.50f,  5.00f);

    Desc("角色半径 (米) - 用于侵蚀, 路径绕开障碍的距离");
    ImGui::SliderFloat("Agent Radius",      &app.Config.AgentRadius,      0.00f,  3.00f);

    Desc("最大攀爬高度 (米) - 单步可上的台阶高度");
    ImGui::SliderFloat("Agent Max Climb",   &app.Config.AgentMaxClimb,    0.10f,  5.00f);

    Desc("最大可行走坡度 (度) - 大于此角度判定为不可走");
    ImGui::SliderFloat("Agent Max Slope",   &app.Config.AgentMaxSlope,    0.00f, 90.00f);

    Desc("区域最小尺寸 (cell^2) - 小于此值的孤立区域被丢弃");
    ImGui::SliderInt  ("Region Min Size",   &app.Config.RegionMinSize,    0,    150);

    Desc("区域合并阈值 (cell^2) - 小于此值的相邻区域会被合并");
    ImGui::SliderInt  ("Region Merge Size", &app.Config.RegionMergeSize,  0,    150);

    Desc("轮廓最大边长 (世界米) - 长边会被切分");
    ImGui::SliderFloat("Edge Max Len",      &app.Config.EdgeMaxLen,       0.0f,  50.0f);

    Desc("轮廓简化最大偏差 - 越大轮廓越粗糙");
    ImGui::SliderFloat("Edge Max Error",    &app.Config.EdgeMaxError,     0.10f,  3.00f);

    Desc("多边形最大顶点数 - 一般 6, 受 Detour 限制最大 12");
    ImGui::SliderInt  ("Verts Per Poly",    &app.Config.VertsPerPoly,     3,     12);

    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Checkbox("Custom Build Volume##bv", &app.BV.bActive);
    if (app.BV.bActive)
    {
        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::DragFloat3("BV Min", app.BV.Min, 0.5f, -500.f, 500.f, "%.1f");
        ImGui::DragFloat3("BV Max", app.BV.Max, 0.5f, -500.f, 500.f, "%.1f");
        ImGui::PopItemWidth();
    }

    ImGui::Spacing();
    ImGui::Checkbox("Use TileCache (dynamic obstacles)##tilecache", &app.Config.bUseTileCache);
    if (app.Config.bUseTileCache)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("(runtime add/remove, no full rebuild)");
        if (app.Nav.bTileMode)
            ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), "TileCache active");
        else
            ImGui::TextDisabled("TileCache inactive (build to activate)");
    }

    // ---- 自动 NavLink 生成 ----
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Checkbox("Auto NavLinks##autonl", &app.AutoNavLinkCfg.bEnabled);
    ImGui::SameLine();
    ImGui::TextDisabled("(基于边界轮廓自动生成)");
    if (app.AutoNavLinkCfg.bEnabled)
    {
        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::DragFloat("Jump Up Height##juh",    &app.AutoNavLinkCfg.JumpUpHeight,      0.05f, 0.0f, 10.0f,  "%.2f m");
        ImGui::TextDisabled("  高度差 <= 此值 → 双向 NavLink (紫色)");
        ImGui::DragFloat("Drop Down Max##ddm",     &app.AutoNavLinkCfg.DropDownMaxHeight, 0.10f, 0.0f, 20.0f,  "%.2f m");
        ImGui::TextDisabled("  高度差 <= 此值 → 单向下跳 NavLink (品红)");
        ImGui::DragFloat("Search Radius##sr",      &app.AutoNavLinkCfg.EdgeSearchRadius,  0.10f, 0.5f, 10.0f,  "%.2f m");
        ImGui::TextDisabled("  XZ 方向配对搜索范围（推荐 2-4m）");
        ImGui::DragFloat("Link Radius##lr",        &app.AutoNavLinkCfg.LinkRadius,        0.05f, 0.1f,  5.0f,  "%.2f m");
        ImGui::DragFloat("Min Height Diff##mhd",   &app.AutoNavLinkCfg.MinHeightDiff,     0.01f, 0.0f,  2.0f,  "%.2f m");
        ImGui::TextDisabled("  小于此高度差的边对不生成（过滤平地噪声）");
        ImGui::DragFloat("Merge Radius##mr",       &app.AutoNavLinkCfg.EdgeMergeRadius,   0.05f, 0.1f,  5.0f,  "%.2f m");
        ImGui::PopItemWidth();

        if (ImGui::Button("Generate NavLinks##gen", ImVec2(-FLT_MIN, 0)))
        {
            if (cb.GenerateNavLinks) cb.GenerateNavLinks();
        }

        ImGui::Checkbox("Show Auto NavLinks##shownl", &app.View.bShowAutoNavLinks);
        if (!app.AutoNavLinks.empty())
        {
            int bidir = 0, oneway = 0;
            for (const auto& lk : app.AutoNavLinks)
                (lk.Dir ? bidir : oneway)++;
            ImGui::TextColored(ImVec4(0.78f, 0.31f, 1.00f, 1.0f),
                               "双向 %d  单向下跳 %d  共 %d",
                               bidir, oneway, (int)app.AutoNavLinks.size());
        }
        else
        {
            ImGui::TextDisabled("(未生成 — 请先 Build NavMesh 再点 Generate)");
        }
    }

    ImGui::Spacing();
    if (ImGui::Button("(Re)Build NavMesh", ImVec2(-FLT_MIN, 0)))
    {
        if (cb.BuildNavMesh) cb.BuildNavMesh();
        if (cb.TryAutoReplan) cb.TryAutoReplan();
    }
    ImGui::Text("Built: %s | %s",
                app.Nav.bBuilt ? "Yes" : "No",
                app.BuildStatus.c_str());
    if (app.Nav.bBuilt && app.Nav.PolyMesh)
    {
        ImGui::Text("PolyMesh:   %d polys, %d verts",
                    app.Nav.PolyMesh->npolys, app.Nav.PolyMesh->nverts);
        if (app.Nav.PolyMeshDetail)
            ImGui::Text("DetailMesh: %d tris, %d verts",
                        app.Nav.PolyMeshDetail->ntris, app.Nav.PolyMeshDetail->nverts);
    }
    if (app.bGeomDirty)
        ImGui::TextColored(ImVec4(1, 0.7f, 0.2f, 1), "Geometry dirty - rebuild needed");
}

// =============================================================================
// DrawPathPanel
// =============================================================================
void DrawPathPanel(AppState& app, const PanelCallbacks& cb)
{
    if (!ImGui::CollapsingHeader("Path Query", ImGuiTreeNodeFlags_DefaultOpen)) return;

    ImGui::PushItemWidth(-FLT_MIN);
    ImGui::DragFloat3("##Start", app.Start, 0.1f); ImGui::SameLine(); ImGui::TextUnformatted("Start");
    ImGui::DragFloat3("##End",   app.End,   0.1f); ImGui::SameLine(); ImGui::TextUnformatted("End");
    ImGui::PopItemWidth();

    if (ImGui::Button("Find Path", ImVec2(-FLT_MIN, 0)))
    {
        if (cb.FindPath) cb.FindPath();
    }
    ImGui::TextWrapped("Status: %s", app.PathStatus.c_str());
    if (!app.StraightPath.empty())
    {
        ImGui::Text("Straight corners: %d", static_cast<int>(app.StraightPath.size() / 3));
        ImGui::Text("Corridor polys:   %d", app.PathPolyCount);
        if (!app.SmoothedPath.empty())
            ImGui::Text("Smoothed corners: %d", static_cast<int>(app.SmoothedPath.size() / 3));
    }

    // ---- Path Smoothing ----
    ImGui::Spacing();
    ImGui::Separator();
    bool smoothChanged = ImGui::Checkbox("Smooth Path##smoothpath", &app.View.bSmoothPath);
    if (app.View.bSmoothPath)
    {
        ImGui::SameLine();
        ImGui::PushItemWidth(100.0f);
        static const char* kMethods[] = { "StringPull", "Bezier" };
        smoothChanged |= ImGui::Combo("##smoothmethod", &app.View.SmoothMethod, kMethods, 2);
        ImGui::PopItemWidth();
        if (app.View.SmoothMethod == 1)
        {
            ImGui::SameLine();
            ImGui::PushItemWidth(60.0f);
            smoothChanged |= ImGui::SliderInt("##subdiv", &app.View.SmoothSubdivisions, 1, 6, "%d");
            ImGui::PopItemWidth();
            ImGui::SameLine(); ImGui::TextDisabled("subdiv");
        }
    }
    if (smoothChanged && !app.StraightPath.empty())
    {
        if (cb.FindPath) cb.FindPath();   // 重新计算以应用新平滑设置
    }
}

// =============================================================================
// DrawIOPanel
// =============================================================================
void DrawIOPanel(AppState& app, const PanelCallbacks& cb)
{
    if (!ImGui::CollapsingHeader("Import / Export")) return;

    // ---- OBJ ----
    ImGui::TextUnformatted("OBJ geometry");
    if (ImGui::Button("Open OBJ...", ImVec2(120, 0)))
    {
        char buf[MAX_PATH] = {};
        if (app.ObjPathInput[0])
        {
            std::strncpy(buf, app.ObjPathInput, sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';
        }
        if (FileDialog::ShowOpenFileDialog(
                "Open OBJ Geometry",
                "OBJ Files (*.obj)\0*.obj\0All Files (*.*)\0*.*\0",
                buf, sizeof(buf)))
        {
            std::strncpy(app.ObjPathInput, buf, sizeof(app.ObjPathInput) - 1);
            app.ObjPathInput[sizeof(app.ObjPathInput) - 1] = '\0';

            std::string err;
            if (cb.LoadObjGeometry && cb.LoadObjGeometry(app.ObjPathInput, err))
            {
                if (cb.DestroyNavRuntime) cb.DestroyNavRuntime();
                app.Geom.Obstacles.clear();
                app.SelectedObstacle = -1;
                app.MoveState.Index  = -1;
                if (cb.FrameCameraToBounds) cb.FrameCameraToBounds();

                // Start/End 按包围盒重定位
                const float* bmin = app.Geom.Bounds + 0;
                const float* bmax = app.Geom.Bounds + 3;
                const float cx = (bmin[0] + bmax[0]) * 0.5f;
                const float cy = (bmin[1] + bmax[1]) * 0.5f;
                const float cz = (bmin[2] + bmax[2]) * 0.5f;
                const float dx = (bmax[0] - bmin[0]) * 0.25f;
                const float dz = (bmax[2] - bmin[2]) * 0.25f;
                app.Start[0] = cx - dx; app.Start[1] = cy; app.Start[2] = cz - dz;
                app.End  [0] = cx + dx; app.End  [1] = cy; app.End  [2] = cz + dz;

                app.BuildStatus = "OBJ loaded: "
                    + std::to_string(app.Geom.Vertices.size()  / 3) + " verts, "
                    + std::to_string(app.Geom.Triangles.size() / 3) + " tris";
                if (cb.TryAutoRebuild) cb.TryAutoRebuild();
            }
            else if (cb.LoadObjGeometry)
            {
                app.BuildStatus = "OBJ load failed: " + err;
            }
        }
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%s", FileDialog::EllipsizePath(app.ObjPathInput).c_str());

    if (ImGui::Button("Use Procedural Geometry", ImVec2(-FLT_MIN, 0)))
    {
        if (cb.DestroyNavRuntime)   cb.DestroyNavRuntime();
        if (cb.InitDefaultGeometry) cb.InitDefaultGeometry();
        if (cb.FrameCameraToBounds) cb.FrameCameraToBounds();
        if (cb.TryAutoRebuild)      cb.TryAutoRebuild();
    }

    ImGui::Separator();

    // ---- NavMesh binary ----
    ImGui::TextUnformatted("NavMesh binary (HNV1 format)");
    if (ImGui::Button("Save As...", ImVec2(120, 0)))
    {
        char buf[MAX_PATH] = {};
        std::strncpy(buf,
                     app.NavSavePathInput[0] ? app.NavSavePathInput : "navmesh.bin",
                     sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';

        if (FileDialog::ShowSaveFileDialog(
                "Save NavMesh",
                "NavMesh (*.bin)\0*.bin\0All Files (*.*)\0*.*\0",
                "bin", buf, sizeof(buf)))
        {
            std::strncpy(app.NavSavePathInput, buf, sizeof(app.NavSavePathInput) - 1);
            app.NavSavePathInput[sizeof(app.NavSavePathInput) - 1] = '\0';

            std::string err;
            if (cb.SaveNavMesh && !cb.SaveNavMesh(app.NavSavePathInput, err))
                app.BuildStatus = "Save failed: " + err;
        }
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%s", FileDialog::EllipsizePath(app.NavSavePathInput).c_str());

    if (ImGui::Button("Open NavMesh...", ImVec2(120, 0)))
    {
        char buf[MAX_PATH] = {};
        if (app.NavLoadPathInput[0])
        {
            std::strncpy(buf, app.NavLoadPathInput, sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';
        }
        if (FileDialog::ShowOpenFileDialog(
                "Open NavMesh Binary",
                "NavMesh (*.bin)\0*.bin\0All Files (*.*)\0*.*\0",
                buf, sizeof(buf)))
        {
            std::strncpy(app.NavLoadPathInput, buf, sizeof(app.NavLoadPathInput) - 1);
            app.NavLoadPathInput[sizeof(app.NavLoadPathInput) - 1] = '\0';

            std::string err;
            if (cb.LoadNavMesh)
            {
                if (cb.LoadNavMesh(app.NavLoadPathInput, err))
                {
                    if (cb.TryAutoReplan) cb.TryAutoReplan();
                }
                else
                {
                    app.BuildStatus = "Load failed: " + err;
                }
            }
        }
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%s", FileDialog::EllipsizePath(app.NavLoadPathInput).c_str());
}

// =============================================================================
// DrawStatsPanel
// =============================================================================
void DrawStatsPanel(AppState& app)
{
    if (!ImGui::CollapsingHeader("Stats & Log")) return;

    ImGui::Text("Build phases (ms):");
    ImGui::BeginTable("##timings", 2,
                      ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH);
    auto Row = [](const char* k, double v)
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn(); ImGui::TextUnformatted(k);
        ImGui::TableNextColumn(); ImGui::Text("%.3f", v);
    };
    Row("Rasterize",       app.Timings.RasterizeMs);
    Row("Filter",          app.Timings.FilterMs);
    Row("CompactHF",       app.Timings.CompactMs);
    Row("Erode",           app.Timings.ErodeMs);
    Row("DistanceField",   app.Timings.DistFieldMs);
    Row("Regions",         app.Timings.RegionsMs);
    Row("Contours",        app.Timings.ContoursMs);
    Row("PolyMesh",        app.Timings.PolyMeshMs);
    Row("DetailMesh",      app.Timings.DetailMeshMs);
    Row("dtCreateNavMesh", app.Timings.DetourCreateMs);
    Row("Total",           app.Timings.TotalMs);
    ImGui::EndTable();

    ImGui::Spacing();
    ImGui::Text("rcContext log (%d lines):", static_cast<int>(app.Nav.Ctx.LogLines.size()));
    if (ImGui::Button("Clear", ImVec2(80, 0))) app.Nav.Ctx.LogLines.clear();
    ImGui::BeginChild("##rclog", ImVec2(0, 160), true,
                      ImGuiWindowFlags_HorizontalScrollbar);
    for (const std::string& line : app.Nav.Ctx.LogLines)
        ImGui::TextUnformatted(line.c_str());
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 20.0f)
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
}

} // namespace Panels
