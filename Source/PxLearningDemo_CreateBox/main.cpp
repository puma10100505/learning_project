

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#endif

#define USE_GLUT

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>

#include <gl/GL.h>
#include "GL/freeglut.h"
#include "CommonDefines.h"
#include "GlutWindow.h"
#include "glm/glm.hpp"
#include "LearningCamera.h"
#include "SceneManager.h"

#include "PxPhysicsAPI.h"

#define PX_RELEASE(x)	if(x)	{ x->release(); x = NULL;	}

using namespace physx;

static char Name[128];
static int PhysXBoxSize = 3;
static int PhysXSphereSize = 3;
static int SphereSlices = 12;
static int SphereStacks = 12;
static glm::vec3 InitVelocity = glm::vec3{0.f, 0.f, 0.f};
static glm::vec3 PhysXBoxInitPos = {0.f, 0.f, -15.f};   // -Z is forward, +Y is up, +X is right
static float PhysXFPS = 30.f;


static void OnCustomGUI(float DeltaTime)
{
    if (GlutWindow::GetInstance() == nullptr)
    {
        return;
    }

    // Window 1
    ImGui::SetNextWindowSize(ImVec2(350, 480));
    ImGui::Begin("Options", nullptr);
    ImGui::NewLine();    
    if (ImGui::Button("Switch GUI Style"))
    {
        if (GlutWindow::GetInstance()->bUseDarkStyle)
        {
            GlutWindow::GetInstance()->bUseDarkStyle = false;
            ImGui::StyleColorsLight();
        }
        else
        {
            GlutWindow::GetInstance()->bUseDarkStyle = true;
            ImGui::StyleColorsDark();
        }
    }

    ImGui::NewLine();
    ImGui::ColorEdit4("BackgroundColor: ", reinterpret_cast<float*>(&GlutWindow::GetInstance()->WindowBackgroundColor));

    ImGui::NewLine();
    ImGui::SliderInt("Render FPS", &GlutWindow::FPS, 15, 120);
    ImGui::SliderFloat("PhysX FPS", &PhysXFPS, 15.f, 120.f);
    ImGui::SliderInt("PhysX Box Size: ", &PhysXBoxSize, 1, 10);
    ImGui::DragFloat3("PhysX Box Init Position: ", reinterpret_cast<float*>(&PhysXBoxInitPos), 0.1f);

    glm::vec3 CameraDir = {GlutWindow::GetScene()->GetCamera()->getDir().x, GlutWindow::GetScene()->GetCamera()->getDir().y, GlutWindow::GetScene()->GetCamera()->getDir().z};
    ImGui::InputText("Object Name", Name, 128);
    if (ImGui::Button("Add PhysX Box"))
    {
        physx::PxVec3 PxPos = GlutWindow::GetScene()->GetCamera()->getTransform().p;
        physx::PxVec3 PxRot = GlutWindow::GetScene()->GetCamera()->getTransform().rotate(PxVec3(0.f, 0.f, -1.f)*200);
        PxVec3 NewPos = PxPos + GlutWindow::GetScene()->GetCamera()->getDir() * 100.f;
        
        GlutWindow::GetScene()->CreateCubeActor("Box...xx-", PhysXBoxSize * 1.f, 
            {NewPos.x, NewPos.y, NewPos.z}, PxIDENTITY::PxIdentity, false, true);
    }

    ImGui::SliderInt("PhysX Sphere Radius: ", &PhysXSphereSize, 1, 30);
    ImGui::SliderInt("PhysX Sphere Slices: ", &SphereSlices, 1, 32);
    ImGui::SliderInt("PhysX Sphere Stacks: ", &SphereStacks, 1, 32);

    if (ImGui::Button("Add PhysX Sphere"))
    {
        PxVec3 PxPos = GlutWindow::GetScene()->GetCamera()->getTransform().p;
        PxVec3 NewPos = PxPos + GlutWindow::GetScene()->GetCamera()->getDir() * 100.f;
        GlutWindow::GetScene()->CreateSphereActor("Sphere...zz-", PhysXSphereSize * 1.f,
            {NewPos.x, NewPos.y, NewPos.z}, SphereSlices, SphereStacks, false, false);
    }

    if (ImGui::Button("Play"))
    {
        GlutWindow::GetInstance()->GetScene()->SetPhysicsEnabled(true);
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop"))
    {
        GlutWindow::GetInstance()->GetScene()->SetPhysicsEnabled(false);
    }
    ImGui::SameLine();
    if (ImGui::Button(("Reset Camera")))
    {
        GlutWindow::GetInstance()->GetScene()->GetCamera()->mEye = PxVec3{0.f, 100.f, 0.f};
    }

    ImGui::End();

    // for (const int Idx: GlutWindow::SelectedActorIndices)
    // {
    //     const GameObject* Info = GlutWindow::GetScene()->GetActorByIndex(Idx);
    //     if (Info)
    //     {
    //         ImGui::Begin(Info->Name.c_str());
    //         ImGui::TextColored({0.3f, 0.4f, 0.2f, 1.f}, "%s", Info->Name.c_str());
    //         ImGui::End();
    //     }
    // }

    ImGui::Begin("Global Info:");
    ImGui::LabelText("Total Object Count", "GameObject Count: %d", GlutWindow::GetInstance()->GetScene()->GetActorCount());
    ImGui::End();

    
}

int main(int argc, char** argv)
{
    GlutWindow::bUseGUI = true;
    GlutWindow::bUseDarkStyle = false;
    GlutWindow::FPS = 30;

    GlutWindow::OnGUICallback = OnCustomGUI;

    GlutWindow::GetInstance(argc, argv, "Window Title", 1280, 720, false)->Show();
    

    return 0;
}