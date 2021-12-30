
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

    
float cameraProjection[16];

float objectMatrix[4][16] = {
  { 1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f },

  { 1.f, 0.f, 0.f, 0.f,
  0.f, 1.f, 0.f, 0.f,
  0.f, 0.f, 1.f, 0.f,
  2.f, 0.f, 0.f, 1.f },

  { 1.f, 0.f, 0.f, 0.f,
  0.f, 1.f, 0.f, 0.f,
  0.f, 0.f, 1.f, 0.f,
  2.f, 0.f, 2.f, 1.f },

  { 1.f, 0.f, 0.f, 0.f,
  0.f, 1.f, 0.f, 0.f,
  0.f, 0.f, 1.f, 0.f,
  0.f, 0.f, 2.f, 1.f }
};

void Cross(const float* a, const float* b, float* r)
{
   r[0] = a[1] * b[2] - a[2] * b[1];
   r[1] = a[2] * b[0] - a[0] * b[2];
   r[2] = a[0] * b[1] - a[1] * b[0];
}

float Dot(const float* a, const float* b)
{
   return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void Normalize(const float* a, float* r)
{
   float il = 1.f / (sqrtf(Dot(a, a)) + FLT_EPSILON);
   r[0] = a[0] * il;
   r[1] = a[1] * il;
   r[2] = a[2] * il;
}

void LookAt(const float* eye, const float* at, const float* up, float* m16)
{
   float X[3], Y[3], Z[3], tmp[3];

   tmp[0] = eye[0] - at[0];
   tmp[1] = eye[1] - at[1];
   tmp[2] = eye[2] - at[2];
   Normalize(tmp, Z);
   Normalize(up, Y);

   Cross(Y, Z, tmp);
   Normalize(tmp, X);

   Cross(Z, X, tmp);
   Normalize(tmp, Y);

   m16[0] = X[0];
   m16[1] = Y[0];
   m16[2] = Z[0];
   m16[3] = 0.0f;
   m16[4] = X[1];
   m16[5] = Y[1];
   m16[6] = Z[1];
   m16[7] = 0.0f;
   m16[8] = X[2];
   m16[9] = Y[2];
   m16[10] = Z[2];
   m16[11] = 0.0f;
   m16[12] = -Dot(X, eye);
   m16[13] = -Dot(Y, eye);
   m16[14] = -Dot(Z, eye);
   m16[15] = 1.0f;
}

void Frustum(float left, float right, float bottom, float top, float znear, float zfar, float* m16)
{
   float temp, temp2, temp3, temp4;
   temp = 2.0f * znear;
   temp2 = right - left;
   temp3 = top - bottom;
   temp4 = zfar - znear;
   m16[0] = temp / temp2;
   m16[1] = 0.0;
   m16[2] = 0.0;
   m16[3] = 0.0;
   m16[4] = 0.0;
   m16[5] = temp / temp3;
   m16[6] = 0.0;
   m16[7] = 0.0;
   m16[8] = (right + left) / temp2;
   m16[9] = (top + bottom) / temp3;
   m16[10] = (-zfar - znear) / temp4;
   m16[11] = -1.0f;
   m16[12] = 0.0;
   m16[13] = 0.0;
   m16[14] = (-temp * zfar) / temp4;
   m16[15] = 0.0;
}

void Perspective(float fovyInDegrees, float aspectRatio, float znear, float zfar, float* m16)
{
   float ymax, xmax;
   ymax = znear * tanf(fovyInDegrees * 3.141592f / 180.0f);
   xmax = ymax * aspectRatio;
   Frustum(-xmax, xmax, -ymax, ymax, znear, zfar, m16);
}

 // Camera projection
bool isPerspective = true;
float fov = 27.f;
float viewWidth = 10.f; // for orthographic
float camYAngle = 165.f / 180.f * 3.14159f;
float camXAngle = 32.f / 180.f * 3.14159f;

bool useWindow = true;
int gizmoCount = 1;
float camDistance = 8.f;
static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);

static void WinTick(float Duration)
{
    
}

static void WinGUI(float Duration)
{
    /*
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
    */

    ImGui::Begin("Spector");
    ImGui::SliderFloat("Fov", &fov, 20.f, 110.f);
    ImGui::End();

    static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::LOCAL);
    static bool useSnap = false;
    static float snap[3] = { 1.f, 1.f, 1.f };
    static float bounds[] = { -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f };
    static float boundsSnap[] = { 0.1f, 0.1f, 0.1f };
    static bool boundSizing = false;
    static bool boundSizingSnap = false;

    ImGuizmo::BeginFrame();
    //ImGui::Begin("Gizmo", 0, ImGuiWindowFlags_NoMove);
    ImGuiIO& io = ImGui::GetIO();
    Perspective(fov, io.DisplaySize.x / io.DisplaySize.y, 0.1f, 100.f, cameraProjection);
    float viewManipulateRight = io.DisplaySize.x;
    float viewManipulateTop = 0;
    //ImGui::SetNextWindowSize(ImVec2(800, 400));
    //ImGui::SetNextWindowPos(ImVec2(400,20));
    //ImGui::PushStyleColor(ImGuiCol_WindowBg, (ImVec4)ImColor(0.35f, 0.3f, 0.3f));
    //ImGuizmo::SetDrawlist();
    //float windowWidth = (float)ImGui::GetWindowWidth();
    //float windowHeight = (float)ImGui::GetWindowHeight();
    //ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);
    //viewManipulateRight = ImGui::GetWindowPos().x + windowWidth;
    //viewManipulateTop = ImGui::GetWindowPos().y;

    // Full window
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

/*
    float eye[] = { cosf(camYAngle) * cosf(camXAngle) * camDistance, sinf(camXAngle) * camDistance, sinf(camYAngle) * cosf(camXAngle) * camDistance };
    float at[] = { 0.f, 0.f, 0.f };
    float up[] = { 0.f, 1.f, 0.f };
    LookAt(eye, at, up, cameraView);
    */
    
    ImGuizmo::DrawGrid(cameraView, cameraProjection, identityMatrix, 100.f);
    ImGuizmo::DrawCubes(cameraView, cameraProjection, &objectMatrix[0][0], 1);
    ImGuizmo::Manipulate(cameraView, cameraProjection, mCurrentGizmoOperation, mCurrentGizmoMode, &objectMatrix[0][0], 
        NULL, useSnap ? &snap[0] : NULL, boundSizing ? bounds : NULL, boundSizingSnap ? boundsSnap : NULL);
    ImGuizmo::ViewManipulate(cameraView, camDistance, ImVec2(viewManipulateRight - 128, viewManipulateTop), ImVec2(128, 128), 0x10101010);

    //ImGui::End();
    //ImGui::PopStyleColor(1);
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