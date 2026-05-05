#pragma once
/*
 * Render/GpuRenderer.h
 * --------------------
 * GPU 着色器 3D 渲染（与 DepthRaster 接口对偶）：
 *   - 通过 OpenGL 3.3 Core + GLSL 330 + FBO 把场景画到一张离屏纹理；
 *   - 真硬件深度测试 + alpha blend；
 *   - End() 后通过 TextureId() 取 ImTextureID，由 ImGui::Image 显示在画布。
 *
 * 平台支持：
 *   - macOS / Linux：基于 GLFW + glad 已建立的 GL 3.3 Core context，可用。
 *   - Windows：当前 demo 走 D3D12 后端，无 GL 上下文 → Available() 返回 false，
 *     调用方应回退到 None / CpuZBuffer / PainterSort。
 *
 * 使用模式（与 DepthRaster 完全一致）：
 *   GpuRenderer::Begin(rw, rh, clearColor, vp, fovy, eye, worldDiag);
 *   ... TriFilled3D / Line3D 走 Primitives.cpp 内部转发 ...
 *   GpuRenderer::End();
 *   ImTextureID tid = GpuRenderer::TextureId();
 */

#include "../Shared/MathUtils.h"
#include "imgui.h"

namespace GpuRenderer
{

/// 灯光参数（与 UI 层的 LightSettings 解耦；调用者负责把 yaw/pitch、direction × distance
/// 这类用户可调字段折成下面这套，shader 只信这套数）。
struct LightParams
{
    /// 0 = Directional（vec 当作光的方向，shader 内 normalize），
    /// 1 = Point      （vec 当作光的世界坐标位置；衰减按 d²/(d² + |vec|²)）。
    int   type      = 0;
    /// Directional：光的方向（任意模长，shader 内 normalize，约定指向"光来自的那一侧"，
    ///             即 dot(N, normalize(vec)) 大代表更亮）。
    /// Point      ：光的世界位置；同时 |vec| 还兼作衰减半径（=该距离时 atten=0.5）。
    Vec3  vec       = { 0.45f, 0.85f, 0.30f };
    /// 颜色（线性 RGB 0-1），与 diffuse 项相乘。
    Vec3  color     = { 1.0f, 1.0f, 1.0f };
    /// 漫反射强度倍数；0 = 完全无方向光（仅 ambient），1 = 经典 Lambert，>1 过曝。
    float intensity = 1.0f;
    /// 环境光强度（0-1）；min(1, ambient + (1-ambient)*diffK*color) 作为最终颜色 multiplier。
    float ambient   = 0.38f;

    // ---- Shadow Mapping（仅 Directional 生效；type==1 时被忽略） ----
    bool  shadowsOn      = false;     ///< 是否启用 shadow pass
    int   shadowMapSize  = 1024;      ///< 正方形阴影贴图边长
    float shadowStrength = 0.7f;      ///< 0 = 无阴影，1 = 阴影区漫反射全部抹除
    float shadowBias     = 0.0015f;   ///< 深度比较 bias，防 acne
    int   shadowPcf      = 1;         ///< 0 单点采样（硬阴影），1 3x3 PCF（软）
    /// 用于构造正交投影的场景中心；通常取 geom 包围盒中心
    Vec3  sceneCenter    = { 0, 0, 0 };
    /// 用于构造正交投影的"半径"：ortho 框 = ±sceneRadius 平面内 + ±2·sceneRadius 沿光方向
    float sceneRadius    = 50.0f;
};

/// 当前平台/构建是否可用（Windows D3D12 构建下为 false）
bool Available();

/// 是否处于 Begin/End 之间（Primitives 转发用）
bool IsActive();

/// 开始一帧：分配/重建 FBO 至 rw×rh，清色 + 清深度
/// samples: MSAA 采样数（1 关闭，2/4/8 启用；超过 GL_MAX_SAMPLES 自动夹紧）
/// light  : 灯光参数（shader fragment 阶段使用），见 LightParams 注释
void Begin(int rw, int rh, ImU32 clearColor, const Mat4& vp, float fovyRad,
           const Vec3& eyeWorld, float worldBoundsDiagonal, int samples = 4,
           const LightParams& light = LightParams{});

/// 提交一个世界空间三角到本帧批次（背面剔除可选；CPU 端做剔除可省 GPU 三角）
void DrawTriangleWorld(const Mat4& vp, Vec3 a, Vec3 b, Vec3 c, ImU32 col,
                       bool cullBackface = true);

/// 提交一条世界空间线段（按相机距离展开成 view-space 细四边形，跨平台保持线宽一致）
void DrawLineWorld(const Mat4& vp, Vec3 a, Vec3 b, ImU32 col, float thicknessPixels);

/// 提交所有批次到 GPU，结束本帧；之后可调用 TextureId() / Buffer{Width,Height}()
void End();

int          BufferWidth();
int          BufferHeight();
ImTextureID  TextureId();

} // namespace GpuRenderer
