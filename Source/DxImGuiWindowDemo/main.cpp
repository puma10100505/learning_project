
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

static std::vector<float> FrameData;
static bool open = true;
std::unique_ptr<BlueprintNodeManager> BP;

static void WinTick(float Duration)
{

}

static void WinGUI(float Duration)
{
    ImGui::Begin("My Window");
    if (ImPlot::BeginPlot("My Plot"))
    {
        ImPlot::PlotBars("My Bar Plot", FrameData.data(), FrameData.size());
        ImPlot::PlotLine("My Line Chart", FrameData.data(), FrameData.size());       
        ImPlot::EndPlot();
    }
    ImGui::End();

    
    ImGui::Begin("node editor", &open);
    BP->Draw();
    ImGui::End();
}

static void WinPostGUI(float Duration)
{
    // IMPORTANT!!! 更新连接要在EndNodeEditor之后
    BP->UpdateLinks();
}

int main(int argc, char** argv)
{
    loguru::init(argc, argv);
    loguru::add_file(__FILE__".log", loguru::Append, loguru::Verbosity_MAX);
    loguru::g_stderr_verbosity = 1;

    LOG_F(INFO, "entry of app");

    io::CSVReader<3> indata(DefaultDataDirectory + "bardata.csv");
    
    indata.read_header(io::ignore_extra_column, "time", "bytes", "frame");
    std::string name;
    float time;
    int bytes;
    int frame;

    while(indata.read_row(time, bytes, frame))
    {
        FrameData.push_back(time);
    }

    BP = std::make_unique<BlueprintNodeManager>();

    BP->AddBPNode("Add")
        ->AddPin("Param1", ENodePinType::INPUT_PIN)
        ->AddPin("Param2", ENodePinType::INPUT_PIN)
        ->AddPin("Result", ENodePinType::OUTPUT_PIN);

    BP->AddBPNode("Multiple")
        ->AddPin("Num1", ENodePinType::INPUT_PIN)
        ->AddPin("Num2", ENodePinType::INPUT_PIN)
        ->AddPin("Ret", ENodePinType::OUTPUT_PIN);

    return dx::CreateWindowInstance("Hello dx window", 900, 600, 
        0, 0, WinTick, WinGUI, WinPostGUI);
}