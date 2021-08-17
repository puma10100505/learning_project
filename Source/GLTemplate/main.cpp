
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