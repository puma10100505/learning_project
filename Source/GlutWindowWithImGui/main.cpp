#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#endif

#define USE_GLUT

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

#include <gl/GL.h>
#include "GL/freeglut.h"
#include "CommonDefines.h"
#include "GlutWindow.h"

static int CubeSize = 5;
static int WireCubeSize = 5;
static int WireTeapotSize = 5;
static int SolidTeapotSize = 5;
static int SolidSphereRadius = 5;
static int SolidSphereSlice = 12;
static int SolidSphereStack = 12;
static int WireSphereRadius = 5;
static int WireSphereSlice = 12;
static int WireSphereStack = 12;
static int GeomType = 0;

static void OnCustomGUI(float DeltaTime)
{
    if (GlutWindow::GetInstance() == nullptr)
    {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(450, 480), ImGuiCond_FirstUseEver);
    ImGui::Begin("Appearence");
    ImGui::NewLine();    
    if (ImGui::Button("Switch GUI Style"))
    {
        if (GlutWindow::GetInstance()->bUseDarkStyle)
        {
            GlutWindow::GetInstance()->bUseDarkStyle = false;
            ImGui::StyleColorsClassic();
        }
        else
        {
            GlutWindow::GetInstance()->bUseDarkStyle = true;
            ImGui::StyleColorsDark();
        }
    }

    ImGui::NewLine();
    ImGui::ColorPicker4("BackgroundColor: ", (float*)&GlutWindow::GetInstance()->WindowBackgroundColor);
    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(350, 480), ImGuiCond_FirstUseEver);
    ImGui::Begin("Choose Geometries");
    ImGui::NewLine();
    
    if (ImGui::RadioButton("SolidCube: ", true))
    {
        GeomType = 0;
    }
    ImGui::SliderInt("Cube Size: ", &CubeSize, 1, 50);
    ImGui::Separator();

    if (ImGui::RadioButton("WireCube: ", false))
    {
        GeomType = 1;
    }
    ImGui::SliderInt("Wire Cube Size: ", &WireCubeSize, 1, 50);
    ImGui::Separator();

    if (ImGui::RadioButton("WireTeapot: ", false))
    {        
        GeomType = 2;
    }
    ImGui::SliderInt("Wire Teapot Size: ", &WireTeapotSize, 1, 50);
    ImGui::Separator();

    if (ImGui::RadioButton("SolidTeapot: ", false))
    {        
        GeomType = 3;
    }
    ImGui::SliderInt("Solid Teapot Size: ", &SolidTeapotSize, 1, 50);
    ImGui::Separator();

    if (ImGui::RadioButton("SolidSphere: ", false))
    {        
        GeomType = 4;
    }
    ImGui::SliderInt("Solid Sphere Size: ", &SolidSphereRadius, 1, 50);
    ImGui::SliderInt("Solid Sphere Slice: ", &SolidSphereSlice, 12, 64);
    ImGui::SliderInt("Solid Sphere Stack: ", &SolidSphereStack, 12, 64);
    ImGui::Separator();

    if (ImGui::RadioButton("WireSphere: ", false))
    {        
        GeomType = 5;
    }
    ImGui::SliderInt("Wire Sphere Size: ", &WireSphereRadius, 1, 50);
    ImGui::SliderInt("Wire Sphere Slice: ", &WireSphereSlice, 12, 64);
    ImGui::SliderInt("Wire Sphere Stack: ", &WireSphereStack, 12, 64);
    ImGui::Separator();

    ImGui::End();
}

static void OnCustomTick(float DeltaTime)
{
    //printf("DeltaTime: %f, %d\n", DeltaTime, glutGet(GLUT_ELAPSED_TIME));

    const float Speed = 80.f;
    const GLfloat Time = glutGet(GLUT_ELAPSED_TIME) / 1000.f * Speed;

    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();

    glRotatef( Time, 0, 0, 1 );
    glRotatef( Time, 0, 1, 0 );
    glRotatef( Time, 1, 0, 0 );

    /*
    * And then drawn...
    */
    switch (GeomType)
    {
        case 0: glutSolidCube(CubeSize); break;
        case 1: glutWireCube(WireCubeSize); break;
        case 2: glutWireTeapot(WireTeapotSize); break;
        case 3: glutSolidTeapot(SolidTeapotSize); break;
        case 4: glutSolidSphere(SolidSphereRadius, SolidSphereSlice, SolidSphereStack); break;
        case 5: glutWireSphere(WireSphereRadius, WireSphereSlice, WireSphereStack); break;
        default: glutSolidCube(CubeSize);
    }

    /*
    * Don't forget about the model-view matrix
    */
    glPopMatrix();
}

int main(int argc, char** argv)
{
    GlutWindow::bUseGUI = true;
    GlutWindow::bUseDarkStyle = false;

    GlutWindow::OnDrawCallback = OnCustomTick;
    GlutWindow::OnGUICallback = OnCustomGUI;

    GlutWindow::GetInstance(argc, argv, "Window Title", 1280, 720, false)->Show();
    
    return 0;
}