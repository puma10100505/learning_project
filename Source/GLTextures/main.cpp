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

extern glm::vec4 BackgroundColor;
extern float DeltaTime;


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
    ImGui::Text("Background Color:");
    ImGui::ColorEdit3("Background Color", (float*)&BackgroundColor);
}

int main(int argc, char** argv)
{
    bool bShowGUI = true;
    if (argc > 1)
    {
        bShowGUI = (atoi(argv[1]) == 1);
    }

    FCreateWindowParameters Parameters
    {
        ScreenWidth, 
        ScreenHeight,
        "GL Textures Test",
        false, 
        bShowGUI, 
        30
    };

    Parameters.KeyEventCallback = OnKeyboardEvent;

    int iRet = GLCreateWindow(Parameters);
    if (iRet < 0)
    {
        return EXIT_FAILURE;
    }

    GLWindowTick(OnTick, OnGUI);

    GLDestroyWindow();

    return EXIT_SUCCESS;
}