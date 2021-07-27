#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Texture2D {
    public:
        GLuint ID;
        GLuint width, height;
        GLuint Internal_Format;
        GLuint Image_Format;
        GLuint Wrap_S;
        GLuint Wrap_T;
        GLuint Filter_Min;
        GLuint Filter_Max;

        Texture2D();
        void Bind() const;
        void Generate(GLuint width, GLuint height, unsigned int* data);
};