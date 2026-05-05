#pragma once
/*
 * UI/Panels.h
 * -----------
 * ImGui 左侧面板各折叠区绘制函数。
 *
 * 提供：
 *   DrawViewPanel   — 视图模式 / 渲染开关 / 相机参数
 *   DrawScenePanel  — 程序化场景尺寸 / 网格密度
 *   DrawEditPanel   — 编辑模式选择 / 快捷键提示 / 障碍列表
 *   DrawBuildPanel  — Recast 构建参数 / (Re)Build 触发
 *   DrawPathPanel   — 起终点 DragFloat / Find Path 触发
 *   DrawIOPanel     — OBJ 导入 / NavMesh 保存加载
 *   DrawStatsPanel  — 构建阶段耗时表格 / rcContext 日志
 *
 * 所有触发副作用的操作通过 PanelCallbacks 注入，
 * 避免对 Nav/NavBuilder.h、Nav/NavQuery.h、loguru 产生直接依赖。
 *
 * 依赖：
 *   - App/AppState.h
 *   - UI/FileDialog.h  （DrawIOPanel 使用）
 *   - imgui.h
 *   - <functional>, <string>
 *
 * 不依赖：Nav/NavBuilder.h、Nav/NavQuery.h
 */

#include "../App/AppState.h"
#include "FileDialog.h"
#include "imgui.h"

#include <functional>
#include <string>

// =============================================================================
// 面板回调（由 AppState 的拥有者注入）
// =============================================================================
struct PanelCallbacks
{
    // ---- 无参数 void 回调 ----
    std::function<void()> BuildNavMesh;                   ///< (Re)Build NavMesh（调用方负责日志）
    std::function<void()> FindPath;                       ///< Find path（调用方负责日志）
    std::function<void()> TryAutoRebuild;                 ///< 几何变化后按 bAutoRebuild 决定是否构建
    std::function<void()> TryAutoReplan;                  ///< 路径变化后按 bAutoReplan 决定是否寻路
    std::function<void()> RebuildProceduralInputGeometry; ///< 重建程序化场景几何
    std::function<void()> FrameCameraToBounds;            ///< 将相机对准当前包围盒
    std::function<void()> DestroyNavRuntime;              ///< 销毁 NavMesh 运行时数据
    std::function<void()> InitDefaultGeometry;            ///< 重置为默认程序化场景

    // ---- 带参数回调 ----
    /// 将障碍坐标 clamp 进以 ±halfSize 为边界的场景范围
    std::function<void(float halfSize)> ClampAllObstaclesToScene;

    /// 从文件加载 OBJ 几何；成功返回 true，失败写 errOut
    std::function<bool(const char* path, std::string& errOut)> LoadObjGeometry;

    /// 保存 NavMesh 到文件；成功返回 true，失败写 errOut
    std::function<bool(const char* path, std::string& errOut)> SaveNavMesh;

    /// 从文件加载 NavMesh；成功返回 true，失败写 errOut
    std::function<bool(const char* path, std::string& errOut)> LoadNavMesh;

    /// 根据当前 NavMesh 自动生成边界 NavLink（需先构建 NavMesh）
    std::function<void()> GenerateNavLinks;
};

// =============================================================================
// 面板绘制函数
// =============================================================================
namespace Panels
{

/// 视图模式 / 渲染开关 / 颜色 / 相机参数（折叠头 "View"）
void DrawViewPanel(AppState& app, const PanelCallbacks& cb);

/// 程序化场景尺寸 / 网格密度 / 预设按钮（折叠头 "Scene / 场景"）
void DrawScenePanel(AppState& app, const PanelCallbacks& cb);

/// 编辑模式 combo / 快捷键树 / 障碍列表（折叠头 "Edit Mode"）
void DrawEditPanel(AppState& app, const PanelCallbacks& cb);

/// Recast 11 参数 + (Re)Build 按钮 + 构建状态（折叠头 "Build Config"）
void DrawBuildPanel(AppState& app, const PanelCallbacks& cb);

/// 起终点 DragFloat + Find Path 按钮 + 路径统计（折叠头 "Path Query"）
void DrawPathPanel(AppState& app, const PanelCallbacks& cb);

/// OBJ 导入 / NavMesh 保存加载（折叠头 "Import / Export"）
void DrawIOPanel(AppState& app, const PanelCallbacks& cb);

/// 构建阶段耗时表格 + rcContext 日志（折叠头 "Stats & Log"）
/// 此面板无副作用回调，仅读取 app 数据
void DrawStatsPanel(AppState& app);

} // namespace Panels
