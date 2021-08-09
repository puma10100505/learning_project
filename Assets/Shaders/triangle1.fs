#version 330 core

out vec4 frag_color;
in vec4 vertex_color; 

void main()
{
    frag_color = vertex_color; //vec4(0.6353, 0.0353, 0.6902, 1.0);
}