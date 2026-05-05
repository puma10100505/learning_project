/*
 * UI/MainLayout.cpp
 * -----------------
 * 顶级 ImGui 布局实现。详细说明见 MainLayout.h。
 */

#include "MainLayout.h"

#include <algorithm>    // std::max / std::min

namespace MainLayout
{

// =============================================================================
// DrawCanvasPanel
// =============================================================================
void DrawCanvasPanel(AppState& app, const InteractionCallbacks& icb)
{
    const ImVec2 panelMin  = ImGui::GetCursorScreenPos();
    const ImVec2 panelSize = ImGui::GetContentRegionAvail();
    if (panelSize.x < 50.0f || panelSize.y < 50.0f) return;

    // 占位并注册 hit-test 区域
    ImGui::InvisibleButton("##NavCanvas", panelSize,
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    if (app.CurrentViewMode == ViewMode::TopDown2D)
    {
        // 构造 2D 绘制参数
        Renderer2D::Draw2DParams p2;
        p2.Geom                 = &app.Geom;
        p2.Nav                  = &app.Nav;
        p2.View                 = &app.View;
        p2.StraightPath         = &app.StraightPath;
        p2.SmoothedPath         = &app.SmoothedPath;
        p2.PathCorridor         = &app.PathCorridor;
        p2.PathColor            = app.PathColor;
        p2.CorridorColor        = app.CorridorColor;
        p2.Start                = app.Start;
        p2.End                  = app.End;
        p2.AgentRadius          = app.Config.AgentRadius;
        p2.AgentHeight          = app.Config.AgentHeight;
        p2.SelectedObstacle     = app.SelectedObstacle;
        p2.MoveStateIndex       = app.MoveState.Index;
        p2.CurrentEditMode      = app.CurrentEditMode;
        p2.CreateDraft          = &app.CreateDraft;
        p2.DefaultObstacleShape = app.DefaultObstacleShape;
        p2.BV                   = &app.BV;
        p2.AutoNavLinks         = &app.AutoNavLinks;

        app.LastMap2D = Renderer2D::DrawCanvas2D(dl, panelMin, panelSize, p2);
    }
    else
    {
        // 构造 3D 绘制参数
        Renderer3D::Draw3DParams p3;
        p3.Geom                   = &app.Geom;
        p3.Nav                    = &app.Nav;
        p3.View                   = &app.View;
        p3.StraightPath           = &app.StraightPath;
        p3.SmoothedPath           = &app.SmoothedPath;
        p3.PathCorridor           = &app.PathCorridor;
        p3.PathColor              = app.PathColor;
        p3.CorridorColor          = app.CorridorColor;
        p3.Start                  = app.Start;
        p3.End                    = app.End;
        p3.AgentRadius            = app.Config.AgentRadius;
        p3.AgentHeight            = app.Config.AgentHeight;
        p3.SelectedObstacle       = app.SelectedObstacle;
        p3.MoveStateIndex         = app.MoveState.Index;
        p3.CurrentEditMode        = app.CurrentEditMode;
        p3.CreateDraft            = &app.CreateDraft;
        p3.DefaultObstacleShape   = app.DefaultObstacleShape;
        p3.DefaultObstacleHeight  = app.DefaultObstacleHeight;
        p3.BV                     = &app.BV;
        p3.AutoNavLinks           = &app.AutoNavLinks;

        app.LastMap3D = Renderer3D::DrawCanvas3D(dl, panelMin, panelSize, app.Cam, p3);
    }

    // 处理鼠标交互
    Interaction::HandleCanvasInteraction(app, icb, panelMin, panelSize);
}

// =============================================================================
// OnGUI
// =============================================================================
void OnGUI(AppState& app, const MainLayoutCallbacks& cb)
{
    // 全局快捷键（键盘）
    Interaction::HandleHotkeys(app, cb.Interaction);

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImVec2 mainPos  = vp->WorkPos;
    ImVec2 mainSize = vp->WorkSize;
    if (app.bShowStatsWindow)
        mainSize.y = std::max(120.0f, mainSize.y - app.StatsDockHeight);
    ImGui::SetNextWindowPos (mainPos);
    ImGui::SetNextWindowSize(mainSize);

    constexpr ImGuiWindowFlags kMainFlags =
        ImGuiWindowFlags_NoTitleBar            |
        ImGuiWindowFlags_NoResize              |
        ImGuiWindowFlags_NoMove                |
        ImGuiWindowFlags_NoCollapse            |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus            |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    ImVec2(6, 6));

    if (ImGui::Begin("##RecastNaviDemoMain", nullptr, kMainFlags))
    {
        // 左侧宽度 clamp
        const float kMainAvailW = ImGui::GetContentRegionAvail().x;
        const float kSplitterW  = 6.0f;
        const float kMaxLeft    = std::max(120.0f, kMainAvailW - 200.0f - kSplitterW);
        app.LeftPaneWidth = std::max(180.0f, std::min(kMaxLeft, app.LeftPaneWidth));

        // ---- 左侧面板 ----
        if (ImGui::BeginChild("##LeftPane",
                              ImVec2(app.LeftPaneWidth, 0.0f),
                              true,
                              ImGuiWindowFlags_NoSavedSettings))
        {
            ImGui::TextUnformatted("RecastNavigation Demo");
            ImGui::SameLine();
            ImGui::TextDisabled(app.Geom.Source == GeomSource::Procedural
                                 ? " [procedural]" : " [obj]");
            ImGui::Separator();

            Panels::DrawViewPanel  (app, cb.Panels);
            ImGui::Separator();
            Panels::DrawScenePanel (app, cb.Panels);
            ImGui::Separator();
            Panels::DrawEditPanel  (app, cb.Panels);
            ImGui::Separator();
            Panels::DrawBuildPanel (app, cb.Panels);
            ImGui::Separator();
            Panels::DrawPathPanel  (app, cb.Panels);
            ImGui::Separator();
            Panels::DrawIOPanel    (app, cb.Panels);
            ImGui::Separator();
            Panels::DrawStatsPanel (app);
        }
        ImGui::EndChild();

        // ---- 分隔条 ----
        ImGui::SameLine(0.0f, 0.0f);
        Splitters::VSplitter("##split_main", &app.LeftPaneWidth, 180.0f, kMaxLeft, kSplitterW);
        ImGui::SameLine(0.0f, 0.0f);

        // ---- 右侧画布 ----
        if (ImGui::BeginChild("##RightPane",
                              ImVec2(0.0f, 0.0f),
                              true,
                              ImGuiWindowFlags_NoSavedSettings))
        {
            // 顶部状态栏
            static const char* kEditName[] = {
                "None", "PlaceStart", "PlaceEnd", "CreateObstacle", "Select&Move", "Delete",
                "OML-Start", "OML-End"
            };
            const char* viewName = (app.CurrentViewMode == ViewMode::TopDown2D)
                                    ? "2D Top" : "3D Orbit";
            ImGui::Text("View: %s   Edit: %s",
                        viewName,
                        kEditName[static_cast<int>(app.CurrentEditMode)]);
            ImGui::SameLine(); ImGui::TextDisabled("  |  ");
            ImGui::SameLine(); ImGui::TextColored(ImVec4(0.71f, 0.31f, 0.31f, 1), "Obstacle");
            ImGui::SameLine(); ImGui::TextColored(ImVec4(0.31f, 0.78f, 0.39f, 1), "NavMesh");
            ImGui::SameLine(); ImGui::TextColored(ImVec4(0.31f, 0.63f, 1.00f, 1), "Start");
            ImGui::SameLine(); ImGui::TextColored(ImVec4(1.00f, 0.31f, 0.31f, 1), "End");
            ImGui::SameLine(); ImGui::TextColored(ImVec4(1.00f, 0.86f, 0.24f, 1), "Path");
            ImGui::Separator();

            DrawCanvasPanel(app, cb.Interaction);
        }
        ImGui::EndChild();
    }
    ImGui::End();

    ImGui::PopStyleVar(3);

    // 浮动趋势图窗口（放在主窗口 End 之后，让它独立可拖拽）
    DrawBuildStatsWindow(app);
}

} // namespace MainLayout
