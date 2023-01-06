
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
#include "CommonDefines.h"
#include "DirectX12Window.h"
#include "loguru.hpp"
#include <vector>
#include "csv.h"

static bool open = true;
static float fov = 60.f;

struct FWindowParameters
{
    bool BoolVal;
    int IntVal;

} WinParameters;

struct FMainMenuParameters
{
    bool bMenuItem1Selected;
} MainMenuParameters;

static void WinTick(float Duration)
{
    
}

static void WinInput(float Duration)
{
    
}

static void WinGUI(float Duration)
{    
    ImGui::SetNextWindowSize(ImVec2(500, 500));

    if (ImGui::Begin("Spector", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar))
    {
        ImGui::SliderFloat("Fov", &fov, 20.f, 110.f);
        ImGui::Checkbox("Test Bool Val: ", &WinParameters.BoolVal);
        if (ImGui::Button("Click Me!"))
        {

        }

        ImGui::SliderInt("Chose Int", &WinParameters.IntVal, 1, 25);

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                ImGui::MenuItem("MenuItem1", "MI1", &MainMenuParameters.bMenuItem1Selected);
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
    } 
    ImGui::End();

    if (WinParameters.BoolVal && WinParameters.IntVal == 25)
    {
        ImGui::SetNextWindowSize(ImVec2(200, 200));
        if (ImGui::Begin("Another Window", nullptr))
        {

        }

        ImGui::End();
    }
}

static void WinPostGUI(float Duration)
{
}

int main(int argc, char** argv)
{
    loguru::init(argc, argv);
    loguru::add_file(__FILE__".log", loguru::Append, loguru::Verbosity_MAX);
    loguru::g_stderr_verbosity = 1;

    LOG_F(INFO, "entry of app");

    return DirectX::CreateWindowInstance("Hello dx window", 1028, 720, 
        0, 0, WinTick, WinGUI, WinPostGUI, WinInput);
}