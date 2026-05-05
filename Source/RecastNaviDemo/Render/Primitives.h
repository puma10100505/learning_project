#pragma once
/*
 * Render/Primitives.h
 * -------------------
 * 低层 3D 软件光栅化绘制原语（基于 ImDrawList）。
 *
 * 所有几何通过合并的视图-投影矩阵 (VP) 进行投影变换，然后
 * 用 ImDrawList 的 2D API 绘制到屏幕上。
 *
 * 对外接口（均在 Render3D 命名空间下）：
 *   Project()                — 世界坐标 → 屏幕坐标
 *   Line3D()                 — 投影线段
 *   TriFilled3D()            — 投影填充三角形
 *   RingXZ()                 — XZ 平面上的圆环
 *   DiscXZFilled()           — XZ 平面上的填充圆盘
 *   DrawCylinderObstacle3D() — 圆柱障碍物
 *   DrawCapsule3D()          — 胶囊体（角色/起终点可视化）
 *
 * 依赖：
 *   - Shared/MathUtils.h（Vec3、Mat4、TransformPoint）
 *   - imgui.h（ImDrawList、ImVec2、ImU32）
 *
 * 不依赖：Nav/、UI/、App/
 */

#include "../Shared/MathUtils.h"
#include "imgui.h"

namespace Render3D
{

// =============================================================================
// 投影结果
// =============================================================================
struct ProjectedPoint
{
    ImVec2 px;   ///< 屏幕坐标
    float  w;    ///< 裁剪空间 w（用于深度排序，如需要）
    bool   ok;   ///< false = 点在近裁剪面后方，不可见
};

// =============================================================================
// 基础投影与绘制原语
// =============================================================================

/// 将世界坐标点 wp 投影到视口 [vMin, vMin+vSize] 内的屏幕像素坐标
ProjectedPoint Project(const Mat4& vp, Vec3 wp, ImVec2 vMin, ImVec2 vSize);

/// 在 3D 空间中绘制线段 a→b（两端均需在近裁剪面前方才绘制）
void Line3D(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
            Vec3 a, Vec3 b, ImU32 col, float thickness = 1.0f);

/// 在 3D 空间中绘制填充三角形（三顶点均需在近裁剪面前方）
void TriFilled3D(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
                 Vec3 a, Vec3 b, Vec3 c, ImU32 col);

// =============================================================================
// 复合几何原语
// =============================================================================

/// 在 XZ 平面 + 给定 Y 高度上绘制 N 段折线圆环
void RingXZ(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
            float cx, float cz, float y, float r, int segs, ImU32 col);

/// 在 XZ 平面 + 给定 Y 高度上绘制填充圆盘（扇形三角剖分）
void DiscXZFilled(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
                  float cx, float cz, float y, float r, int segs, ImU32 col);

/// 绘制圆柱障碍物（从 y=0 到 y=h）：顶面填充圆盘 + 顶底圆环 + 8 条立柱
void DrawCylinderObstacle3D(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
                             float cx, float cz, float r, float h,
                             ImU32 fillTop, ImU32 edgeCol);

/// 绘制胶囊体：圆柱段 + 顶/底半球经线弧
/// @param cx, cz   水平中心
/// @param r        半径
/// @param height   总高（>= 2r）
/// @param groundY  胶囊底部的世界 Y（OBJ 模式下不为 0）
void DrawCapsule3D(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
                   float cx, float cz, float r, float height, ImU32 col,
                   float groundY = 0.0f);

} // namespace Render3D
