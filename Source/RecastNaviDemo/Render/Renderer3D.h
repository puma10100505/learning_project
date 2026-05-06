#pragma once
/*
 * Render/Renderer3D.h
 * -------------------
 * 3D 轨道视角软件渲染接口（投影到 ImDrawList）。
 *
 * DrawCanvas3D() 将 NavMesh 场景以轨道相机投影到指定 ImDrawList 区域，
 * 并返回本帧的 Map3D 矩阵/视口缓存（供 Interaction 层做射线拣取）。
 *
 * 参数化说明：
 *   原始实现通过全局变量访问相机、视图设置、障碍状态等；
 *   这里将它们统一收录到 Draw3DParams 结构体，由调用方从 AppState 中填入。
 *
 * 依赖：
 *   - Render/RenderTypes.h   （Map3D、OrbitCamera）
 *   - Render/Renderer2D.h    （HashColor、ColU32 共用）
 *   - Nav/NavTypes.h         （InputGeometry、NavRuntime、ObstacleShape …）
 *   - UI/UITypes.h           （ViewSettings、EditMode、CreateDraftBox …）
 *   - imgui.h                （ImDrawList、ImVec2、ImVec4、ImU32）
 *   - DetourNavMesh.h        （dtPolyRef）
 *
 * 不依赖：App/、Nav/NavBuilder.h、Nav/NavQuery.h
 */

#include "RenderTypes.h"
#include "Renderer2D.h"        // HashColor / ColU32
#include "../Nav/NavTypes.h"
#include "../UI/UITypes.h"

#include "imgui.h"
#include "DetourNavMesh.h"  // dtPolyRef

#include <vector>

namespace Renderer3D
{

// =============================================================================
// 3D 画布绘制参数
// =============================================================================
struct Draw3DParams
{
    // 场景几何
    const InputGeometry*          Geom                 = nullptr;

    // NavMesh 运行时（只读）
    const NavRuntime*             Nav                  = nullptr;

    // 视图设置
    const ViewSettings*           View                 = nullptr;

    // 路径数据
    const std::vector<float>*          StraightPath     = nullptr;
    const std::vector<float>*          SmoothedPath     = nullptr;  ///< 非空且 View->bSmoothPath 时优先绘制
    const std::vector<dtPolyRef>*      PathCorridor     = nullptr;

    // 路径颜色
    ImVec4                        PathColor            = { 1.00f, 0.86f, 0.24f, 1.00f };
    ImVec4                        CorridorColor        = { 0.30f, 0.85f, 1.00f, 0.35f };

    // 起/终点 & 代理尺寸
    const float*                  Start                = nullptr;   ///< float[3]
    const float*                  End                  = nullptr;   ///< float[3]
    float                         AgentRadius          = 0.4f;
    float                         AgentHeight          = 2.0f;

    // 障碍选中/移动状态
    int                           SelectedObstacle     = -1;
    int                           MoveStateIndex       = -1;

    // 编辑模式（用于草稿预览）
    EditMode                      CurrentEditMode      = EditMode::None;
    const CreateDraftBox*         CreateDraft          = nullptr;
    ObstacleShape                 DefaultObstacleShape = ObstacleShape::Box;
    float                         DefaultObstacleHeight = 1.6f;

    // 自定义生成区域（可选，bActive=false 时不渲染）
    const BuildVolume*            BV                   = nullptr;

    // 自动生成的 NavLink（可选，View->bShowAutoNavLinks 控制）
    const std::vector<OffMeshLink>* AutoNavLinks        = nullptr;
};

// =============================================================================
// 主绘制函数
// =============================================================================

/// 将 3D 场景绘制到 [panelMin, panelMin+panelSize] 的 ImDrawList 区域。
/// @param cam  已同步 Target 的相机（Eye 枢轴旋转；参见 OrbitCamSyncLookAtFromEye）
/// @return     本帧的视口/矩阵缓存（bValid=true 表示已成功写入）
Map3D DrawCanvas3D(ImDrawList*          dl,
                   ImVec2               panelMin,
                   ImVec2               panelSize,
                   const OrbitCamera&   cam,
                   const Draw3DParams&  p);

} // namespace Renderer3D
