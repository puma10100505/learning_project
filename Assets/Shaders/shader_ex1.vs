#version 330 core

layout (location = 0) in vec3 VertexPosition;

uniform float HorizontalOffset;

void main()
{
    gl_Position = vec4(VertexPosition.x + HorizontalOffset, -VertexPosition.y, VertexPosition.z, 1.0);
}