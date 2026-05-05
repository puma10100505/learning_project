#pragma once
/*
 * Render/DepthRaster.h
 * --------------------
 * 软件深度缓冲（Z-Buffer）：透视校正深度 + 每像素深度测试，行为等价于 GPU depth test。
 * 与 ImDrawList 并行：Begin() 之后 Render3D::TriFilled3D / Line3D 写入本缓冲；End() 后取 RGBA 上传纹理。
 */

#include "../Shared/MathUtils.h"
#include "imgui.h"

namespace DepthRaster
{

/// 当前帧是否正在深度光栅（由 Begin/End 管理）
bool IsActive();

/// 开始一帧：分配/清理 rw×rh，清除颜色与深度（深度初值 1 = 远）
void Begin(int rw, int rh, ImU32 clearColor, const Mat4& vp, float fovyRad,
           const Vec3& eyeWorld, float worldBoundsDiagonal);

void End();

int  BufferWidth();
int  BufferHeight();
const uint8_t* PixelsRGBA(); ///< rw*rh*4，行优先，与 ImGui RGBA8 一致

/// 世界空间三角形（供 Primitives 转发）；默认按背面剔除（法线朝向相机为正面）
void DrawTriangleWorld(const Mat4& vp, Vec3 a, Vec3 b, Vec3 c, ImU32 col,
                       bool cullBackface = true);

/// 世界空间线段：展开为细四边形以参与深度测试
void DrawLineWorld(const Mat4& vp, Vec3 a, Vec3 b, ImU32 col, float thicknessPixels);

} // namespace DepthRaster
