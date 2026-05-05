#version 330 core
// =============================================================================
// recast_shadow.vs
//   Directional 方向光阴影的"深度 pass"顶点着色器。
//   绑定与主 pass 相同的 VBO（Vertex 结构：pos / col / normal / lit），
//   只读 aPos 和 aLit；线段类几何（aLit < 0.5）会被推到 clip 空间外，
//   保证只有受光面三角才进入 shadow map。
// =============================================================================

layout(location = 0) in vec3  aPos;
layout(location = 3) in float aLit;

uniform mat4 uLightVP;   // 光源空间 VP（正交投影 × Look-At）

void main()
{
    if (aLit < 0.5)
    {
        // 把整三角顶点推出 NDC 立方体，光栅化阶段会把它丢掉
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
    }
    else
    {
        gl_Position = uLightVP * vec4(aPos, 1.0);
    }
}
