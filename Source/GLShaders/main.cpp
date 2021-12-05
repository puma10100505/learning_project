
/*
* 创建窗口的基础代码模板
*
*/
#ifdef __linux__
#include <unistd.h>
#endif

#include <iostream>
#include <string.h>
#include <cstring>
#include <memory>
#include "learning.h"
#include "LearningStatics.h"
#include "CommonDefines.h"
#include "GlfwWindows.h"

extern glm::vec4 BackgroundColor;
extern float DeltaTime;

static void OnTick(float DeltaTime);
static void OnGUI(float DeltaTime);

static void InitGLTriangle();
static void DrawGLTriangle();

const static GLfloat TriangleVertices[] = {
    -0.6f, -0.3f, 0.f,
    0.8f, -0.5f, 0.f, 
    0.f, 0.7f, 0.f
};

static uint32_t TriangleVBO, TriangleVAO;

std::unique_ptr<Shader> TriangleShader = nullptr;

static float HOffset = 0.3f;

int main()
{
    if (GLCreateWindow(FCreateWindowParameters::DefaultWindowParameters()) < 0)
    {
        return EXIT_FAILURE;
    }

    InitGLTriangle();

    GLWindowTick(OnTick, OnGUI);

    GLDestroyWindow();

    return EXIT_SUCCESS;
}

void InitGLTriangle()
{
    TriangleShader = std::make_unique<Shader>("shader_ex1");

    glGenBuffers(1, &TriangleVBO);
    glGenVertexArrays(1, &TriangleVAO);

    glBindVertexArray(TriangleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, TriangleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(TriangleVertices), TriangleVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(0);
}

void DrawGLTriangle()
{
    if (TriangleShader.get())
    {
        TriangleShader->Activate();
        TriangleShader->SetVector4Value("Color", 0.4f, 0.5f, 0.2f, 1.f);
        TriangleShader->SetFloatValue("HorizontalOffset", HOffset);
        glBindVertexArray(TriangleVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
    }
}

void OnTick(float DeltaTime)
{
    DrawGLTriangle();
}

void OnGUI(float DeltaTime)
{
    ImGui::Begin("Shader Parameters");
    ImGui::SliderFloat("HorizontalOffset", &HOffset, 0.1f, 1.f, "%1.2f", 1.f);
    ImGui::End();
}