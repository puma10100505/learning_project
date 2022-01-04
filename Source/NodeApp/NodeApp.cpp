
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
#include "implot/implot.h"
#include "ImGuizmo.h"
#include "imnodes/imnodes.h"
#include "BlueprintNodeManager.h"

static int item_current = 1;
const char* items[] = { "Add", "Multiple", "Substract", "Divide", "Output", "Begin", "Constant" };

/* *********************************** */
#define INITIALIZE_BLUEPRINT_MANAGER std::unique_ptr<BlueprintNodeManager> BP = std::make_unique<BlueprintNodeManager>();

#define UPDATE_BLUEPRINT_LINKS BP->UpdateLinks();

#define DRAW_BLUEPRINT \
{\
    const ImGuiViewport* viewport = ImGui::GetMainViewport();\
    ImGui::SetNextWindowPos(viewport->Pos);\
    ImGui::SetNextWindowSize(viewport->Size);\
    ImGui::Begin("node editor", nullptr, /*, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | */ImGuiWindowFlags_NoBringToFrontOnFocus);\
    BP->Draw();\
    ImGui::End();\
}

#define BPNode_ADD \
{\
    BP->AddBPNode("Add")\
        ->AddTextPin("Param1", ENodePinType::INPUT_PIN)\
        ->AddTextPin("Param2", ENodePinType::INPUT_PIN)\
        ->AddPin("Result", ENodePinType::OUTPUT_PIN);\
}

#define BPNode_Multiple \
{\
    BP->AddBPNode("Mul")\
        ->AddTextPin("Param1", ENodePinType::INPUT_PIN)\
        ->AddTextPin("Param2", ENodePinType::INPUT_PIN)\
        ->AddPin("Result", ENodePinType::OUTPUT_PIN);\
}

#define BPNode_Substract \
{\
    BP->AddBPNode("Sub")\
        ->AddTextPin("Param1", ENodePinType::INPUT_PIN)\
        ->AddTextPin("Param2", ENodePinType::INPUT_PIN)\
        ->AddPin("Result", ENodePinType::OUTPUT_PIN);\
}

#define BPNode_Divide \
{\
    BP->AddBPNode("Div")\
        ->AddTextPin("Param1", ENodePinType::INPUT_PIN)\
        ->AddTextPin("Param2", ENodePinType::INPUT_PIN)\
        ->AddPin("Result", ENodePinType::OUTPUT_PIN);\
}

/* *********************************** */

INITIALIZE_BLUEPRINT_MANAGER;

static void WinTick(float Duration)
{
    
}

static void WinInput(float Duration)
{
    if (GetAsyncKeyState(VK_UP))
    {
        LOG_F(INFO, "Up key pressed");
    }

    
}

static void WinGUI(float Duration)
{
    {
        ImGui::Begin("Node Selector");

        if (ImGui::ListBox("", &item_current, items, IM_ARRAYSIZE(items), 10))
        {
            LOG_F(INFO, "Selected: Index: %d, Val: %s", item_current, items[item_current]);
        }
        ImGui::End();
    }

    DRAW_BLUEPRINT;
}

static void WinPostGUI(float Duration)
{
   UPDATE_BLUEPRINT_LINKS;
}

int main(int argc, char** argv)
{
    loguru::init(argc, argv);
    loguru::add_file(__FILE__".log", loguru::Append, loguru::Verbosity_MAX);
    loguru::g_stderr_verbosity = 1;

    LOG_F(INFO, "entry of app");

    BPNode_ADD;
    BPNode_Multiple;

    
    return dx::CreateWindowInstance("Hello dx window", 900, 600, 0, 0, WinTick, WinGUI, WinPostGUI, WinInput);
}