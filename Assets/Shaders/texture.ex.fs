#version 330 core

out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D texture1;
uniform sampler2D texture2;

void main()
{
    // 混合两个纹理(Lerp)
    FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.5);
}