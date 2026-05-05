#pragma once
/*
 * Render/RenderTypes.h
 * --------------------
 * 渲染层所需的数据结构：
 *   - OrbitCamera：轨道摄像机参数（Yaw/Pitch/Distance/Target/Fovy）
 *   - Map2D：2D 画布的屏幕↔世界坐标映射缓存
 *   - Map3D：3D 视图的视口/矩阵缓存（用于射线拣取）
 *
 * 依赖：Shared/MathUtils.h（Vec3、Mat4）
 * 不依赖：Nav/、UI/、ImGui
 *
 * 注意：Map2D / Map3D 中的 ImVec2 字段使用 imgui 头文件，
 *       因此使用方需先 include imgui.h。
 */

#include "../Shared/MathUtils.h"
#include "imgui.h"

// =============================================================================
// OrbitCamera — 绕目标点的轨道摄像机
// =============================================================================
struct OrbitCamera
{
    Vec3  Target   = { 0.0f, 0.0f, 0.0f };
    float Yaw      = 0.7f;           ///< 水平旋转角（弧度）
    float Pitch    = 0.9f;           ///< 俯仰角（弧度）
    float Distance = 36.0f;          ///< 距离目标的半径
    float Fovy     = 60.0f * 3.1415926f / 180.0f;  ///< 垂直视角（弧度）
};

// =============================================================================
// Map2D — 2D 俯视图的坐标映射缓存
//         每帧由 DrawCanvas2D 更新，供 Interaction 中的鼠标拣取使用
// =============================================================================
struct Map2D
{
    ImVec2 CanvasMin{};    ///< 画布左上角屏幕坐标
    ImVec2 CanvasSize{};   ///< 画布宽高（像素）
    float  Bmin[3]{};      ///< 世界包围盒最小值（来自 InputGeometry::Bounds[0..2]）
    float  Bmax[3]{};      ///< 世界包围盒最大值（来自 InputGeometry::Bounds[3..5]）
    bool   bValid = false; ///< 本帧是否已由 DrawCanvas2D 写入有效数据
};

// =============================================================================
// Map3D — 3D 轨道视图的视口/矩阵缓存
//         每帧由 DrawCanvas3D 更新，供 Interaction 中的射线拣取使用
// =============================================================================
struct Map3D
{
    ImVec2 ViewportMin{};   ///< 画布左上角屏幕坐标
    ImVec2 ViewportSize{};  ///< 画布宽高（像素）
    Vec3   EyeWorld{};      ///< 摄像机眼睛位置（世界空间）
    Mat4   InvVP{};         ///< 视图投影矩阵的逆（用于射线反向投影）
    Mat4   View{};          ///< 视图矩阵
    Mat4   Proj{};          ///< 投影矩阵
    bool   bValid = false;  ///< 本帧是否已由 DrawCanvas3D 写入有效数据
};
