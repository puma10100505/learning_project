
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
#include "implot/implot.h"

extern glm::vec4 BackgroundColor;
extern float DeltaTime;

static float BarData[10] = {372.945, 383.463, 414.9, 11.25, 1101.63, 1055.4, 390.836, 1060.87, 1039.71, 5.836};

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
    ImGui::Begin("My Window");

    if (ImPlot::BeginPlot("My Plot")) {
        
         ImPlot::PlotBars("My Bar Plot", BarData, 10);
         ImPlot::EndPlot();
    }

    ImGui::End();
}

int main()
{
    if (GLCreateWindow(FCreateWindowParameters::DefaultWindowParameters()) < 0)
    {
        return EXIT_FAILURE;
    }

    GLWindowTick(OnTick, OnGUI);

    GLDestroyWindow();
    
    return EXIT_SUCCESS;
}