
/*
* 创建窗口的基础代码模板
*
*/
#ifdef __linux__
#include <unistd.h>
#endif

#include <iostream>
#include <string.h>
#include <memory>
#include <cstring>
#include "GlfwWindows.h"
#include "LearningStatics.h"
#include "CommonDefines.h"
#include "LearningCamera.h"
#include "imgui.h"
#include "ImGuizmo.h"

extern glm::vec4 BackgroundColor;
extern float DeltaTime;


static const float identityMatrix[16] =
{ 1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f };

//static std::unique<LearningCamera> MainCamera;

bool bIsPerspective = true;



static void OnKeyboardEvent(GLFWwindow* InWindow, int Key, int ScanCode, int Action, int Mods)
{
    if (glfwGetKey(InWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(InWindow, GLFW_TRUE);
    }
}

static void OnTick(float DeltaTime)
{
    
}

static void OnGUI(float DeltaTime)
{
    ImGuiIO& io = ImGui::GetIO();
    
    glm::mat4 CameraProjection = glm::mat4(1.f);
    if (bIsPerspective)
    {
        CameraProjection = glm::perspective(60.f, io.DisplaySize.x / io.DisplaySize.y, 0.1f, 100.f);
    }
    else
    {
        float ViewHeight = 10.f * io.DisplaySize.y / io.DisplaySize.x + io.DisplaySize.y;
        CameraProjection = glm::ortho(-10.f, 10.f, -ViewHeight, ViewHeight,1000.f, -1000.f);
    }

    ImGuizmo::SetOrthographic(!bIsPerspective);
    ImGuizmo::BeginFrame();

    ImGui::SetNextWindowPos(ImVec2(1024, 100));
    ImGui::SetNextWindowSize(ImVec2(900, 600));
    ImGui::Begin("Test");

    ImGuizmo::SetID(0);
    
    glm::mat4 CameraView = glm::lookAt(glm::vec3{0.f, 0.f, 10.f}, glm::vec3{0.f, 0.f, 0.f}, 
        glm::vec3{0.f, 1.f, 0.f});
    const float* CameraViewPtr = (const float*)glm::value_ptr(CameraView);
    const float* CameraProjectionPtr = (const float*)glm::value_ptr(CameraProjection);
    ImGuizmo::DrawGrid(CameraViewPtr, CameraProjectionPtr, (const float*)glm::value_ptr(glm::mat4(1.f)), 100.f);

    ImGui::End();
}

int main()
{
    //MainCamera = std::make_unique<LearningCamera>({0.f, 0.f, 10.f}, {1.f, 0.f, 0.f});
    if (GLCreateWindow(FCreateWindowParameters::DefaultWindowParameters()) < 0)
    {
        return EXIT_FAILURE;
    }

    GLWindowTick(OnTick, OnGUI);

    GLDestroyWindow();

    return EXIT_SUCCESS;
}