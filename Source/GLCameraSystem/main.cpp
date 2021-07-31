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

extern glm::vec4 BackgroundColor;

// static std::unique_ptr<Shader> GlobalShader(new Shader(
//     (solution_base_path + "Assets/Shaders/multilights.object.vs").c_str(), 
//     (solution_base_path + "Assets/Shaders/multilights.object.fs").c_str())
// );

static void WindowKeyCallback(GLFWwindow* InWindow, int Key, int ScanCode, int Action, int Mods)
{

}

static void OnGUI(float DeltaTime)
{

    ImGui::Begin("Test");                          // Create a window called "Hello, world!" and append into it.

    ImGui::Text("Background Color:");
    ImGui::ColorEdit3("Background Color", (float*)&BackgroundColor); // Edit 3 floats representing a color

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
}

static void OnTick(float DeltaTime)
{
    
}

int main()
{
    int iRet = GLCreateWindow({
        ScreenWidth,
        ScreenHeight,
        "Camera System",
        false,
        true,
        30,
        WindowKeyCallback
    });

    if (iRet < 0)
    {
        printf("Create window failed, ret: %d\n", iRet);
        return EXIT_FAILURE;
    }

    GLWindowTick(OnTick, OnGUI);

    GLDestroyWindow();

    return EXIT_SUCCESS;
}