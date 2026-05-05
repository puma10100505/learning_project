#version 330 core
// =============================================================================
// recast_demo.vs
//   Render/GpuRenderer.cpp 在 LinkProgram() 时通过 LoadShaderSource() 读入并编译。
//   顶点结构与 Render/GpuRenderer.cpp 中 struct Vertex 一一对应：
//     aPos    : 世界坐标 (vec3)
//     aCol    : 顶点颜色 (vec4, 已 unpack 自 ImU32 RGBA)
//     aNormal : 世界空间面法线 (vec3, CPU 端逐三角计算)
//     aLit    : 光照参与开关 (1.0=三角参与 Lambert，0.0=线段直显颜色)
// =============================================================================

layout(location = 0) in vec3  aPos;
layout(location = 1) in vec4  aCol;
layout(location = 2) in vec3  aNormal;
layout(location = 3) in float aLit;

uniform mat4 uVP;
uniform mat4 uLightVP;   // 光源空间 VP，用于阴影采样（无阴影时填单位阵也无害）

out vec4  vCol;
out vec3  vNormal;
out vec3  vWorld;
out float vLit;
out vec4  vLightSpace;   // 光源空间齐次坐标，FS 内做透视除法 + bias 后采样 shadowMap

void main()
{
    vCol         = aCol;
    vNormal      = aNormal;
    vWorld       = aPos;
    vLit         = aLit;
    vLightSpace  = uLightVP * vec4(aPos, 1.0);
    gl_Position  = uVP * vec4(aPos, 1.0);
}
