#pragma once
/*
 * Render/PainterSort.h
 * ----------------------
 * 画家算法（按深度排序）：先收集 TriFilled3D / Line3D，再按三角重心距相机由远到近绘制，
 * 最后绘制线段，用于无 Z-Buffer 时的近似遮挡顺序。
 */

#include "../Shared/MathUtils.h"
#include "imgui.h"

namespace PainterSort
{

bool IsCollecting();

/// 开始收集（与 DepthRaster 互斥；仅 Renderer3D 在 PainterSort 模式下调用）
void Begin(const Vec3& eyeWorld, const Mat4& vp, ImVec2 vMin, ImVec2 vSize);

void RecordTri(const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
               Vec3 a, Vec3 b, Vec3 c, ImU32 col);

void RecordLine(const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
                Vec3 a, Vec3 b, ImU32 col, float thickness);

/// 排序并绘制全部三角，再按提交顺序绘制线段，然后结束收集状态
void Flush(ImDrawList* dl);

} // namespace PainterSort
