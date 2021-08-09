#version 330 core

out vec4 FragColor;
in vec4 vertex_color; 

// uniform vec4 Color;

void main()
{
    FragColor = vertex_color; //vec4(0.8353, 0.2745, 0.0157, 1.0);
}