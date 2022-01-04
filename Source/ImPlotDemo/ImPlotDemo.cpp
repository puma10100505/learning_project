
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
#include "loguru.hpp"
#include "ImGuizmo.h"
#include "csv.h"

extern glm::vec4 BackgroundColor;
extern float DeltaTime;

float cameraView[16] =
   { 1.f, 0.f, 0.f, 0.f,
     0.f, 1.f, 0.f, 0.f,
     0.f, 0.f, 1.f, 0.f,
     0.f, 0.f, 0.f, 1.f };

static const float identityMatrix[16] =
{ 1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f };

static float cameraProjection[16];

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

std::vector<float> FrameData;


static void OnGUI(float DeltaTime)
{
    ImGuizmo::BeginFrame();

    ImGui::Begin("My Window");

    if (ImPlot::BeginPlot("My Plot"))
    {
        ImPlot::PlotBars("My Bar Plot", FrameData.data(), FrameData.size());
        ImPlot::PlotLine("My Line Chart", FrameData.data(), FrameData.size());       
        ImPlot::EndPlot();
    }

    ImGui::End();

    //ImGui::Begin("Gizmo");
    //ImGuizmo::DrawGrid(cameraView, cameraProjection, identityMatrix, 100.f);
    //ImGui::End();
}

int main(int argc, char** argv)
{
    loguru::init(argc, argv);
    loguru::add_file("ImPlotDemo.log", loguru::Append, loguru::Verbosity_MAX);
    loguru::g_stderr_verbosity = 1;

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

    if (GLCreateWindow(FCreateWindowParameters::DefaultWindowParameters()) < 0)
    {
        return EXIT_FAILURE;
    }

    GLWindowTick(OnTick, OnGUI);

    GLDestroyWindow();
    
    return EXIT_SUCCESS;
}