#pragma once
/*
 * UI/UITypes.h
 * ------------
 * UI 层所需的枚举和状态结构体：
 *   - ViewMode      ：2D 俯视 / 3D 轨道
 *   - EditMode      ：当前编辑操作（无 / 放置起点 / 放置终点 / 创建障碍 / 移动 / 删除）
 *   - ViewSettings  ：渲染可见性开关 + 自动重新规划/重构标志
 *   - CreateDraftBox：鼠标拖拽创建 Box 障碍的临时状态
 *   - MoveBoxState  ：鼠标拖拽移动 Box 障碍的临时状态
 *
 * 依赖：无（不依赖 Nav/、Render/、ImGui）
 */

// =============================================================================
// 视图模式
// =============================================================================
enum class ViewMode
{
    TopDown2D,  ///< 2D 俯视图（ImDrawList 软件绘制）
    Orbit3D,    ///< 3D 轨道视角（软件投影到 ImDrawList）
};

// =============================================================================
// 编辑模式
// =============================================================================
enum class EditMode
{
    None,              ///< 仅浏览，不编辑
    PlaceStart,        ///< 点击画布放置寻路起点
    PlaceEnd,          ///< 点击画布放置寻路终点
    CreateBox,         ///< 鼠标拖拽创建 Box 障碍
    MoveBox,           ///< 鼠标拖拽移动已有障碍
    DeleteBox,         ///< 点击画布删除障碍
    PlaceOffMeshStart, ///< 点击画布设定 OffMeshLink 起点
    PlaceOffMeshEnd,   ///< 点击画布设定 OffMeshLink 终点
};

// =============================================================================
// 视图渲染可见性设置
// =============================================================================
struct ViewSettings
{
    bool bShowGrid         = true;   ///< 显示网格辅助线
    bool bShowObstacles    = true;   ///< 显示障碍物轮廓
    bool bShowInputMesh    = true;   ///< 显示输入几何线框（地面/OBJ）
    bool bFillPolygons     = true;   ///< 填充 NavMesh 多边形
    bool bRegionColors     = true;   ///< 使用区域哈希色区分多边形
    bool bShowDetailMesh   = false;  ///< 显示细节网格
    bool bShowPolyEdges    = true;   ///< 显示多边形边线
    bool bShowPathCorridor = true;   ///< 显示寻路走廊高亮
    bool bAutoReplan       = true;   ///< 每次操作后自动重新寻路
    bool bAutoRebuild      = false;  ///< 几何变化后自动重新构建 NavMesh
    bool bShowBuildVolume  = true;   ///< 显示自定义生成区域线框盒
    bool bShowAutoNavLinks = true;   ///< 显示自动生成的 NavLink
    bool bSmoothPath       = false;  ///< 显示平滑路径（代替原始折线）
    int  SmoothMethod      = 0;      ///< 0=StringPull, 1=Bezier
    int  SmoothSubdivisions = 3;     ///< Bezier 细分轮数（1-6）
};

// =============================================================================
// 创建 Box 障碍的草稿状态（鼠标按下→拖拽→释放）
// =============================================================================
struct CreateDraftBox
{
    bool  bActive = false;  ///< 是否正在拖拽创建中
    float StartX  = 0.0f;  ///< 鼠标按下时的世界 X
    float StartZ  = 0.0f;  ///< 鼠标按下时的世界 Z
    float CurX    = 0.0f;  ///< 鼠标当前位置的世界 X
    float CurZ    = 0.0f;  ///< 鼠标当前位置的世界 Z
};

// =============================================================================
// 移动障碍的临时状态（鼠标拖拽）
// =============================================================================
struct MoveBoxState
{
    int   Index   = -1;    ///< 被拖拽的障碍索引（-1 = 无）
    float OffsetX = 0.0f;  ///< 鼠标按下时相对障碍中心 CX 的 X 偏移
    float OffsetZ = 0.0f;  ///< 鼠标按下时相对障碍中心 CZ 的 Z 偏移
};
