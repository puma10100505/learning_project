
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

static std::vector<float> FrameData;

static std::vector<std::pair<int, int>> Links;

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

    
    ImGui::Begin("node editor");

    ImNodes::BeginNodeEditor();
    ImNodes::BeginNode(1);
    ImGui::Dummy(ImVec2(80.0f, 45.0f));
    ImNodes::EndNode();

    ImNodes::BeginNode(2);
    ImNodes::BeginOutputAttribute(2);
    ImGui::Text("output pin");
    ImNodes::EndOutputAttribute();
    ImNodes::EndNode();

    ImNodes::BeginNode(3);
    ImNodes::BeginInputAttribute(3, ImNodesPinShape_Circle);
    ImGui::Text("Input Pin");
    ImNodes::EndInputAttribute();

    ImNodes::BeginOutputAttribute(4);
    ImGui::Text("Output Pin");
    ImNodes::EndOutputAttribute();
    ImNodes::EndNode();

    ImNodes::BeginNode(4);
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted("output node");
    ImNodes::EndNodeTitleBar();
    ImNodes::EndNode();

    ImNodes::BeginNode(5);
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted("Filter");
    ImNodes::EndNodeTitleBar();
    ImNodes::BeginInputAttribute(6);
    ImGui::Text("Input Filter Item");
    ImNodes::EndInputAttribute();
    ImNodes::BeginOutputAttribute(7);
    ImGui::Text("Filter Result");
    ImNodes::EndOutputAttribute();
    ImNodes::EndNode();

    ImNodes::MiniMap(0.2f, ImNodesMiniMapLocation_TopRight);
    
    // IMPORTANT!!! 绘制连接要在EndNodeEditor之前
    for (int i = 0; i < Links.size(); i++)
    {
        const std::pair<int, int> p = Links[i];
        ImNodes::Link(i, p.first, p.second);
    }

    ImNodes::EndNodeEditor();
    ImGui::End();

    // IMPORTANT!!! 更新连接要在EndNodeEditor之后
    int StartAttrId, EndAttrId;
    if (ImNodes::IsLinkCreated(&StartAttrId, &EndAttrId))
    {
        Links.push_back(std::make_pair(StartAttrId, EndAttrId));
    }
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

    return dx::CreateWindowInstance("Hello dx window", 900, 600, 0, 0, WinTick, WinGUI);
}