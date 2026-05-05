#pragma once
/*
 * Render/Renderer2D.h
 * -------------------
 * 2D 俯视图软件渲染接口。
 *
 * DrawCanvas2D() 将 NavMesh 场景绘制到一块 ImDrawList 区域中，
 * 并返回本帧的 Map2D 坐标映射缓存（供 Interaction 层做鼠标拣取）。
 *
 * 参数化说明：
 *   原始实现直接读取 30+ 个全局变量，这里将它们按职责分组成
 *   Draw2DParams 结构体，由调用方从 AppState 中填入。
 *
 * 颜色工具（HashColor / ColU32）也在此暴露，供 Renderer3D 共用。
 *
 * 依赖：
 *   - Render/RenderTypes.h  （Map2D）
 *   - Nav/NavTypes.h        （InputGeometry、NavRuntime、ObstacleShape …）
 *   - UI/UITypes.h          （ViewSettings、EditMode、CreateDraftBox …）
 *   - imgui.h               （ImDrawList、ImVec2、ImVec4、ImU32）
 *   - DetourNavMesh.h       （dtPolyRef）
 *
 * 不依赖：App/、Nav/NavBuilder.h、Nav/NavQuery.h
 */

#include "RenderTypes.h"
#include "../Nav/NavTypes.h"
#include "../UI/UITypes.h"

#include "imgui.h"
#include "DetourNavMesh.h"  // dtPolyRef

#include <vector>

namespace Renderer2D
{

// =============================================================================
// 颜色工具（供 Renderer3D 共用）
// =============================================================================

/// 用哈希值生成稳定颜色（NavMesh 区域着色）
ImU32 HashColor(unsigned int id, float alpha = 0.55f);

/// ImVec4 (RGBA float) → ImU32 (ABGR packed)；可选 alpha 覆盖
ImU32 ColU32(const ImVec4& c, float alphaOverride = -1.0f);

// =============================================================================
// 2D 画布绘制参数
// =============================================================================
struct Draw2DParams
{
    // 场景几何
    const InputGeometry*          Geom            = nullptr;

    // NavMesh 运行时（只读）
    const NavRuntime*             Nav             = nullptr;

    // 视图设置
    const ViewSettings*           View            = nullptr;

    // 路径数据
    const std::vector<float>*          StraightPath  = nullptr;
    const std::vector<float>*          SmoothedPath  = nullptr;  ///< 非空且 View->bSmoothPath 时优先绘制
    const std::vector<dtPolyRef>*      PathCorridor  = nullptr;

    // 路径颜色（ImVec4，由 AppState 持有）
    ImVec4                        PathColor       = { 1.00f, 0.86f, 0.24f, 1.00f };
    ImVec4                        CorridorColor   = { 0.30f, 0.85f, 1.00f, 0.35f };

    // 起/终点 & NavBuildConfig（用于胶囊半径/高度）
    const float*                  Start           = nullptr;   ///< float[3]
    const float*                  End             = nullptr;   ///< float[3]
    float                         AgentRadius     = 0.4f;
    float                         AgentHeight     = 2.0f;

    // 障碍选中/移动状态
    int                           SelectedObstacle = -1;
    int                           MoveStateIndex   = -1;

    // 编辑模式
    EditMode                      CurrentEditMode  = EditMode::None;
    const CreateDraftBox*         CreateDraft      = nullptr;
    ObstacleShape                 DefaultObstacleShape  = ObstacleShape::Box;

    // 自定义生成区域（可选，bActive=false 时不渲染）
    const BuildVolume*            BV               = nullptr;

    // 自动生成的 NavLink（可选，View->bShowAutoNavLinks 控制）
    const std::vector<OffMeshLink>* AutoNavLinks   = nullptr;
};

// =============================================================================
// 主绘制函数
// =============================================================================

/// 将 2D 俯视场景绘制到 [panelMin, panelMin+panelSize] 的 ImDrawList 区域。
/// @return  本帧的坐标映射缓存（bValid=true 表示参数有效）；
///          若场景包围盒无效则返回 bValid=false 的空 Map2D。
Map2D DrawCanvas2D(ImDrawList* dl,
                   ImVec2      panelMin,
                   ImVec2      panelSize,
                   const Draw2DParams& p);

} // namespace Renderer2D
