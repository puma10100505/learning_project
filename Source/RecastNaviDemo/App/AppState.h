#pragma once
/*
 * App/AppState.h
 * --------------
 * 全应用状态聚合体。
 *
 * 把原 RecastNaviDemo.cpp 中散落的 30+ 全局变量按职责分组，
 * 统一收录到 AppState 结构体中。各模块通过引用接收 AppState，
 * 不再依赖隐式全局变量，便于测试和多场景扩展。
 *
 * 依赖：
 *   - Nav/NavTypes.h      （InputGeometry、NavRuntime 等）
 *   - Shared/Profiling.h  （PhaseTimings、BuildStat、PathStat）
 *   - Render/RenderTypes.h（OrbitCamera、Map2D、Map3D）
 *   - UI/UITypes.h         （ViewMode、EditMode、ViewSettings 等）
 *   - imgui.h             （ImVec4）
 *   - DetourNavMesh.h     （dtPolyRef）
 *
 * 使用方式：
 *   AppState& app = GetAppState();   // 单例，由 main 拥有
 *   NavBuilder::BuildNavMesh(app.Geom, app.Config, app.Nav, app.Timings);
 */

#include "../Nav/NavTypes.h"
#include "../Shared/Profiling.h"
#include "../Render/RenderTypes.h"
#include "../UI/UITypes.h"

#include "imgui.h"
#include "DetourNavMesh.h"   // dtPolyRef

#include <deque>
#include <string>
#include <vector>

// =============================================================================
// 历史记录长度上限
// =============================================================================
static constexpr int kBuildHistoryMax = 128;
static constexpr int kPathHistoryMax  = 128;

// =============================================================================
// AppState — 全局状态聚合
// =============================================================================
struct AppState
{
    // -------------------------------------------------------------------------
    // 导航数据
    // -------------------------------------------------------------------------
    InputGeometry  Geom;    ///< 输入几何（顶点、三角形、障碍列表）
    SceneConfig    Scene;   ///< Procedural 地面尺寸 / 网格密度
    NavBuildConfig Config;  ///< Recast 构建参数
    BuildVolume    BV;      ///< 自定义 NavMesh 生成区域（可选 AABB）
    NavRuntime     Nav;     ///< 构建结果（PolyMesh、dtNavMesh、dtNavMeshQuery …）
    PhaseTimings   Timings; ///< 最近一次构建各阶段耗时

    // -------------------------------------------------------------------------
    // 寻路结果
    // -------------------------------------------------------------------------
    float                  Start[3]      = { -10.0f, 0.0f, -10.0f };  ///< 寻路起点
    float                  End[3]        = {  12.0f, 0.0f,  12.0f };  ///< 寻路终点
    std::vector<float>     StraightPath;  ///< 直线路径点（x,y,z 交替）
    std::vector<float>     SmoothedPath;  ///< 平滑路径点（ViewSettings.bSmoothPath 开启时有效）
    std::vector<dtPolyRef> PathCorridor; ///< 多边形走廊（findPath 结果）
    int                    PathPolyCount = 0;
    std::string            PathStatus    = "(none)";
    std::string            BuildStatus   = "(not built)";

    // -------------------------------------------------------------------------
    // 性能历史（用于趋势图）
    // -------------------------------------------------------------------------
    std::deque<BuildStat>  BuildHistory;
    int                    BuildCounter = 0;
    std::deque<PathStat>   PathHistory;
    int                    PathCounter  = 0;

    // -------------------------------------------------------------------------
    // 场景编辑状态
    // -------------------------------------------------------------------------
    bool           bGeomDirty            = false;              ///< 几何已变化，需重构 NavMesh
    ObstacleShape  DefaultObstacleShape  = ObstacleShape::Box;
    float          DefaultObstacleHeight = 1.6f;
    int            SelectedObstacle      = -1;                 ///< -1 表示无选中
    CreateDraftBox CreateDraft;  ///< 拖拽创建 Box 草稿
    MoveBoxState   MoveState;    ///< 拖拽移动障碍状态
    float          PendingOffMeshStart[3] = {};  ///< OffMeshLink 第一个端点（等待用户点第二下）
    std::vector<dtObstacleRef> TileCacheObsRefs;  ///< TileCache 模式下障碍引用（与 Obstacles 一一对应）

    // -------------------------------------------------------------------------
    // 自动 NavLink（基于边界轮廓生成，BuildNavMesh 后由用户触发）
    // -------------------------------------------------------------------------
    AutoNavLinkConfig        AutoNavLinkCfg;   ///< 自动 NavLink 生成配置
    std::vector<OffMeshLink> AutoNavLinks;     ///< 自动生成的 NavLink 列表（纯可视化，不参与寻路构建）

    // -------------------------------------------------------------------------
    // 视图 / 相机
    // -------------------------------------------------------------------------
    ViewSettings View;
    OrbitCamera  Cam;
    ViewMode     CurrentViewMode = ViewMode::TopDown2D;
    EditMode     CurrentEditMode = EditMode::None;

    // -------------------------------------------------------------------------
    // 坐标映射缓存（每帧由 DrawCanvas2D / DrawCanvas3D 刷新）
    // -------------------------------------------------------------------------
    Map2D LastMap2D;
    Map3D LastMap3D;

    // -------------------------------------------------------------------------
    // UI 可视状态
    // -------------------------------------------------------------------------
    bool   bShowStatsWindow   = true;
    float  LeftPaneWidth      = 400.0f;  ///< 左侧配置区宽度（像素，可拖动）
    float  StatsDockHeight    = 280.0f;  ///< 底部性能窗口高度（像素，可拖动）
    float  StatsLegendWidth   = 200.0f;  ///< 性能图右侧图例宽度（像素，可拖动）
    ImVec4 PathColor          = { 1.00f, 0.86f, 0.24f, 1.00f };  ///< 路径折线颜色
    ImVec4 CorridorColor      = { 0.30f, 0.85f, 1.00f, 0.35f };  ///< 走廊高亮颜色

    // -------------------------------------------------------------------------
    // 文件路径缓冲（ImGui InputText 专用）
    // -------------------------------------------------------------------------
    char ObjPathInput[256]     = "";
    char NavSavePathInput[256] = "navmesh.bin";
    char NavLoadPathInput[256] = "navmesh.bin";
};

// =============================================================================
// 单例访问
//
// AppState 由 main() 拥有（static AppState g_State），
// 通过引用传递给各模块，不再有隐式全局依赖。
// GetAppState() 仅作为从顶层回调（OnGUI、OnTick 等）访问全局实例的桥梁。
// =============================================================================
AppState& GetAppState();
