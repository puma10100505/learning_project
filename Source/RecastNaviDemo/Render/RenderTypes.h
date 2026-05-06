#pragma once
/*
 * Render/RenderTypes.h
 * --------------------
 * 渲染层所需的数据结构：
 *   - OrbitCamera：Eye 为摄像头世界位置（旋转绕自身），Target 为视线前方 Distance 处的观察点
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
// OrbitCamera — Eye 枢轴旋转，Target = Eye − dir(Yaw,Pitch)×Distance（视线落脚点）
// =============================================================================

/// 单位方向：从历史「绕 Target」公式来的 Target→Eye（与旧 Eye = Target + dir×D 一致）
inline Vec3 OrbitCamDirTargetToEye(float yawRad, float pitchRad)
{
    const float cx = std::cos(yawRad),   sx = std::sin(yawRad);
    const float cy = std::cos(pitchRad), sy = std::sin(pitchRad);
    return V3(cx * cy, sy, sx * cy);
}

struct OrbitCamera
{
    Vec3  Eye{}; ///< 摄像机世界坐标（右键旋转绕此点；平移/WASD 主要改此字段）
    Vec3  Target   = { 0.0f, 0.0f, 0.0f }; ///< Look-at（由 Eye/Yaw/Pitch/Distance 同步）
    float Yaw      = 0.7f;           ///< 水平旋转角（弧度）
    float Pitch    = 0.9f;           ///< 俯仰角（弧度）
    float Distance = 36.0f;          ///< 沿视线方向 Eye 到 Target 的距离
    float Fovy     = 60.0f * 3.1415926f / 180.0f;  ///< 垂直视角（弧度）
    /// 对齐 UE Level Editor：按住右键时滚轮调节 WASD 飞行速度倍率（非缩放）。
    float NavSpeedScale = 1.0f;
};

/// FrameCamera：已设 Target / Yaw / Pitch / Distance 时写入 Eye（与旧构图等价）
inline void OrbitCamSyncEyeFromPivot(OrbitCamera& c)
{
    c.Eye = c.Target + OrbitCamDirTargetToEye(c.Yaw, c.Pitch) * c.Distance;
}

/// Eye 固定时刷新 Target（旋转/缩放/键盘平移后以 Eye 为准）
inline void OrbitCamSyncLookAtFromEye(OrbitCamera& c)
{
    const Vec3 d = OrbitCamDirTargetToEye(c.Yaw, c.Pitch);
    c.Target     = c.Eye - d * c.Distance;
}

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
