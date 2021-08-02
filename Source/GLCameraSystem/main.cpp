#ifdef __linux__
    #include <unistd.h>
#endif 
#include <cstdio>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

#include "CommonDefines.h"
#include "learning.h"
#include "LearningStatics.h"

extern glm::vec4 BackgroundColor;
extern float DeltaTime;

static float LastMouseX = ScreenWidth / 2.f;
static float LastMouseY = ScreenHeight / 2.f;
static bool bIsFirstMove = true;

static std::unique_ptr<GLScene> MainScene = std::make_unique<GLScene>(__back * 6.f);
static std::unique_ptr<Shader> MainShader = nullptr;
static std::unique_ptr<GLCubic> Cube = nullptr;

static void WindowKeyCallback(GLFWwindow* InWindow, int Key, int ScanCode, int Action, int Mods)
{
    if (glfwGetKey(InWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(InWindow, GLFW_TRUE);
    }

    if (glfwGetKey(InWindow, GLFW_KEY_W) == GLFW_PRESS)
    {
        printf("w for forward key is pressing\n");
        MainScene->MainCamera->ProcessKeyboard(FORWARD, DeltaTime);
    }

    if (glfwGetKey(InWindow, GLFW_KEY_S) == GLFW_PRESS)
    {
        MainScene->MainCamera->ProcessKeyboard(BACKWARD, DeltaTime);
    }

    if (glfwGetKey(InWindow, GLFW_KEY_A) == GLFW_PRESS)
    {
        MainScene->MainCamera->ProcessKeyboard(LEFT, DeltaTime);
    }

    if (glfwGetKey(InWindow, GLFW_KEY_D) == GLFW_PRESS)
    {
        MainScene->MainCamera->ProcessKeyboard(RIGHT, DeltaTime);
    }

    if (glfwGetKey(InWindow, GLFW_KEY_Q) == GLFW_PRESS)
    {
        MainScene->MainCamera->ProcessKeyboard(UP, DeltaTime);
    }

    if (glfwGetKey(InWindow, GLFW_KEY_E) == GLFW_PRESS)
    {
        MainScene->MainCamera->ProcessKeyboard(DOWN, DeltaTime);
    }
}

static void OnGUI(float DeltaTime)
{
    ImGui::Begin("Test");                          // Create a window called "Hello, world!" and append into it.

    ImGui::Text("Background Color:");
    ImGui::ColorEdit3("Background Color", (float*)&BackgroundColor); // Edit 3 floats representing a color

    ImGui::Text("Camera:");
    ImGui::SliderFloat("FOV", &(MainScene->MainCamera->Zoom), 0.0f, 170.0f);
    ImGui::DragFloat3("camera.position", (float*)&(MainScene->MainCamera->Position), .1f);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 
        1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
}

static void OnTick(float DeltaTime)
{
    if (MainShader.get() == nullptr)
    {
        return;
    }

    if (MainScene.get() == nullptr)
    {
        return;
    }

    MainShader->use();

    LearningStatics::GLDrawWorldGrid(10.f, 100, 2.f, {0.3f, 0.5f, 0.6f, .8f});

    glm::mat4 Projection = glm::perspective(glm::radians(MainScene->MainCamera->Zoom), MainScene->WindowRatio, .1f, 5000.f);
    glm::mat4 View = MainScene->MainCamera->GetViewMatrix();

    MainShader->setMat4("projection", Projection);
    MainShader->setMat4("view", View);

    MainScene->Render();
}

static void OnCursorPosChanged(GLFWwindow* window, double xpos, double ypos)
{
    if (bIsFirstMove)
    {
        LastMouseX = xpos;
        LastMouseY = ypos;
        bIsFirstMove = false;
    }

    float MouseXOffset = xpos - LastMouseX;
    float MouseYOffset = LastMouseY - ypos;

    LastMouseX = xpos;
    LastMouseY = ypos;

    MainScene->MainCamera->ProcessMoveMovement(MouseXOffset, MouseYOffset);
}

static void OnMouseScrollChanged(GLFWwindow* window, double xoffset, double yoffset)
{
    printf("Entry of OnMouseScrollChanged, xoffset: %f, yoffset: %f\n", xoffset, yoffset);
    MainScene->MainCamera->Zoom = 
        glm::clamp<float>(MainScene->MainCamera->Zoom - yoffset, 1.f, 45.f);
}

int main()
{
    FCreateWindowParameters Params 
    {
        ScreenWidth,
        ScreenHeight,
        "Camera System",
        false,
        true,
        30
    };

    Params.KeyEventCallback = WindowKeyCallback;
    Params.MouseCursorPosChanged = OnCursorPosChanged;
    Params.MouseScrollCallback = OnMouseScrollChanged;

    int iRet = GLCreateWindow(Params);
    if (iRet < 0)
    {
        printf("Create window failed, ret: %d\n", iRet);
        return EXIT_FAILURE;
    }

    Cube.reset(MainScene->AddCube(solution_base_path + "Assets/Shaders/multilights.object.vs", 
        solution_base_path + "Assets/Shaders/multilights.object.fs"));

    Cube->SetDiffuseTexture(solution_base_path + "Assets/Textures/container2.png");
    Cube->SetSpecularTexture(solution_base_path + "Assets/Textures/container2_specular.png");

    Cube->transform.position = glm::vec3(0.f, 0.f, 0.f);

    MainShader = std::make_unique<Shader>(
        (solution_base_path + "Assets/Shaders/multilights.object.vs").c_str(), 
        (solution_base_path + "Assets/Shaders/multilights.object.fs").c_str());

    glfwSetInputMode(GetGlobalWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    GLWindowTick(OnTick, OnGUI);

    GLDestroyWindow();

    return EXIT_SUCCESS;
}