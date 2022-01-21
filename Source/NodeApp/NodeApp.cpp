
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
#include "glm/glm.hpp"
#include "csv.h"
#include "implot/implot.h"
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
    ImGui::Begin("node editor", nullptr, ImGuiWindowFlags_NoBringToFrontOnFocus);\
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

// Command Move: Direction Distance
#define BPNode_Move \
{\
    BP->AddBPNode("Move")\
        ->AddExecPin("", ENodePinType::INPUT_PIN)\
        ->AddVector3Pin("Direction", ENodePinType::INPUT_PIN)\
        ->AddFloatPin("Distance", ENodePinType::INPUT_PIN)\
        ->AddExecPin("", ENodePinType::OUTPUT_PIN);\
}

// Command Fire: Direction Times
#define BPNode_Fire \
{\
    BP->AddBPNode("Fire")\
        ->AddExecPin("", ENodePinType::INPUT_PIN)\
        ->AddVector3Pin("Direction", ENodePinType::INPUT_PIN)\
        ->AddIntPin("Times", ENodePinType::INPUT_PIN)\
        ->AddExecPin("", ENodePinType::OUTPUT_PIN);\
}

// Begin
#define BPNode_Begin \
{\
    BP->AddBPNode("Begin")\
        ->AddExecPin("", ENodePinType::OUTPUT_PIN);\
}

// End
#define BPNode_End \
{\
    BP->AddBPNode("End")\
        ->AddExecPin("", ENodePinType::INPUT_PIN);\
}

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
        // ImGui::Begin("Node Selector");

        // if (ImGui::ListBox("", &item_current, items, IM_ARRAYSIZE(items), 10))
        // {
        //     LOG_F(INFO, "Selected: Index: %d, Val: %s", item_current, items[item_current]);
        // }
        // ImGui::End();
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

    BPNode_Move;
    BPNode_Begin;
    BPNode_End;
    BPNode_Fire;
    
    return dx::CreateWindowInstance("Hello dx window", 900, 600, 0, 0, 
        WinTick, WinGUI, WinPostGUI, WinInput);
}