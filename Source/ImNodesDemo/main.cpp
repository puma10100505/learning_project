
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
#include "imnodes/imnodes.h"


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
    ImGui::Begin("node editor");

    ImNodes::BeginNodeEditor();
    ImNodes::BeginNode(1);
    ImGui::Dummy(ImVec2(80.0f, 45.0f));
    ImNodes::EndNode();

    ImNodes::BeginNode(2);
    const int output_attr_id = 2;
    ImNodes::BeginOutputAttribute(output_attr_id);
    // in between Begin|EndAttribute calls, you can call ImGui
    // UI functions
    ImGui::Text("output pin");
    ImNodes::EndOutputAttribute();
    ImNodes::EndNode();

    ImNodes::BeginNode(3);
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted("output node");
    ImNodes::EndNodeTitleBar();
    ImNodes::EndNode();

    ImNodes::MiniMap(0.2f, ImNodesMiniMapLocation_TopRight);

    ImNodes::EndNodeEditor();

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