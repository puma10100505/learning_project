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

#include "PxPhysicsAPI.h"

#define PX_RELEASE(x)	if(x)	{ x->release(); x = NULL;	}

using namespace physx;

PxDefaultAllocator gAllocator;
PxDefaultErrorCallback gErrorCallback;

PxFoundation* gFoundation = nullptr;
PxPhysics* gPhysics = nullptr;
PxDefaultCpuDispatcher* gDispatcher = nullptr;
PxScene* gScene = nullptr;
PxMaterial* gMaterial = nullptr;
PxPvd* gPvd = nullptr;

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
static int PhysXBoxSize = 3;
static glm::vec3 PhysXBoxInitPos = {0.f, 0.f, -15.f};   // -Z is forward, +Y is up, +X is right

static void DrawStuffByGeomType()
{
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();

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

    glPopMatrix();
}

void CreatePhysXBoxGeometry(const std::string& Name, float InSize, const glm::vec3& InPosition)
{
    PxShape* Shape = gPhysics->createShape(PxBoxGeometry(InSize / 2, InSize / 2, InSize / 2), *gMaterial);
    if (!Shape)
    {
        printf("Create shape failed\n");
        return;
    }

    PxRigidDynamic* Body = gPhysics->createRigidDynamic(PxTransform(InPosition.x, InPosition.y, InPosition.z));
    if (!Body)
    {
        printf("Create body failed\n");
        return;
    }
    Body->setName(Name.c_str());

    Body->attachShape(*Shape);
    PxRigidBodyExt::updateMassAndInertia(*Body, 10.f);
    gScene->addActor(*Body);

    // 因为Shape通过值引用复制到了Body中，所以这里可以先销毁
    Shape->release();
}

void InitializePhysics()
{
    gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
    gPvd = PxCreatePvd(*gFoundation);
    gPvd->connect(*PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10), PxPvdInstrumentationFlag::eALL);
    gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true, gPvd);

    PxSceneDesc SceneDesc(gPhysics->getTolerancesScale());
    SceneDesc.gravity = PxVec3(0.f, -9.8f, 0.f);
    gDispatcher = PxDefaultCpuDispatcherCreate(2);
    SceneDesc.cpuDispatcher = gDispatcher; //PxDefaultCpuDispatcherCreate(2);
    SceneDesc.filterShader = PxDefaultSimulationFilterShader;
    gScene = gPhysics->createScene(SceneDesc);

    PxPvdSceneClient* PvdClient = gScene->getScenePvdClient();
    if (PvdClient)
    {
        PvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
        PvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
        PvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
    }


    gMaterial = gPhysics->createMaterial(.5f, .5f, .5f);

    // 地面
    PxRigidStatic* GroundPlane = PxCreatePlane(*gPhysics, PxPlane(0, 1, 0, 15), *gMaterial);
    //GroundPlane->setGlobalPose(PxTransform(0.f, 0.f, 0.f));
    GroundPlane->setName("Plane");
    gScene->addActor(*GroundPlane);

    // 创建物理形状
    PxShape* Shape = gPhysics->createShape(PxBoxGeometry(1.f, 1.f, 1.f), *gMaterial);
    if (!Shape)
    {
        printf("Create shape failed\n");
        return;
    }

    PxRigidDynamic* Body = gPhysics->createRigidDynamic(PxTransform(0.f, 1.f, 0.f));
    if (!Body)
    {
        printf("Create body failed\n");
        return;
    }
    Body->setName("CubeActor");

    Body->attachShape(*Shape);
    PxRigidBodyExt::updateMassAndInertia(*Body, 10.f);
    gScene->addActor(*Body);

    // 因为Shape通过值引用复制到了Body中，所以这里可以先销毁
    Shape->release();

    printf("Initialize physics succeed...\n");
}

void StepPhysics()
{
    gScene->simulate(1.f / 30.f);
    gScene->fetchResults(true);
}

void CleanupPhysics()
{
    PX_RELEASE(gScene);
    PX_RELEASE(gDispatcher);
    PX_RELEASE(gPhysics);
    if (gPvd)
    {
        PxPvdTransport* Transport = gPvd->getTransport();
        gPvd->release();
        gPvd = nullptr;
        PX_RELEASE(Transport);
    }

    PX_RELEASE(gFoundation);
    printf("simple demo done\n");
}

void RenderGeometryHolder(const PxGeometry& Geom)
{
    switch (Geom.getType())
    {
        case PxGeometryType::eBOX:
            {
                const PxBoxGeometry& BoxGeom = static_cast<const PxBoxGeometry&>(Geom);
                //glScalef(BoxGeom.halfExtents.x, BoxGeom.halfExtents.y, BoxGeom.halfExtents.z);

                glutSolidCube(PhysXBoxSize);
            
                // printf("Finished draw a cube with box_extent: %f, %f, %f\n", 
                //     BoxGeom.halfExtents.x, BoxGeom.halfExtents.y, BoxGeom.halfExtents.z);
                break;
            }

        case PxGeometryType::eSPHERE:
        {
            const PxSphereGeometry& SphereGeom = static_cast<const PxSphereGeometry&>(Geom);
            glutSolidSphere(GLdouble(SphereGeom.radius), 10, 10);
            break;
        }

        case PxGeometryType::eCAPSULE:
        {
            break;
        }
    default: ;
    }
}

void RenderActors(PxRigidActor** InActors, const PxU32 NumActors, bool bShadows, const PxVec3& InColor/*, TriggerRender* Callback*/)
{
    const PxVec3 ShadowDir(0.f, 0.7f, 0.7f);
    const PxReal ShadowMat[] = {1, 0, 0, 0, -ShadowDir.x/ShadowDir.y, 0, -ShadowDir.z/ShadowDir.y, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    PxShape* Shapes[128];
    for (PxU32 i = 0; i < NumActors; i++)
    {
        const PxU32 NbShapes = InActors[i]->getNbShapes();
        //printf("There are %u shaped in actor: %s\n", NbShapes, InActors[i]->getName());

        PX_ASSERT(NbShapes <= 128);
        InActors[i]->getShapes(Shapes, NbShapes);
        
        const bool bIsSleeping = InActors[i]->is<PxRigidDynamic>() ? InActors[i]->is<PxRigidDynamic>()->isSleeping() : false;

        for (PxU32 j = 0; j < NbShapes; j++)
        {
            const PxTransform ShapeTrans = PxShapeExt::getGlobalPose(*Shapes[j], *InActors[i]);
            const PxMat44 ShapePose(ShapeTrans);
            const PxGeometryHolder Holder = Shapes[j]->getGeometry();
            const bool bIsTrigger = Shapes[j]->getFlags() & PxShapeFlag::eTRIGGER_SHAPE;
            if (bIsTrigger)
            {
                // 如果Shape是Trigger区域，则只渲染外框
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            }

            glPushMatrix();
            glMultMatrixf(&ShapePose.column0.x);
            // printf("shape pose x: %f, y: %f, z: %f\n", ShapePose.column0.x , 
            //     ShapePose.column0.y, ShapePose.column0.z);
            if (bIsSleeping)
            {
                const PxVec3 DarkColor = InColor * .25f;
                glColor4f(DarkColor.x, DarkColor.y, DarkColor.z, 1.f);
                //printf("actor is sleeping...\n");
            }
            else
            {
                glColor4f(InColor.x, InColor.y, InColor.z, 1.f);
            }

            RenderGeometryHolder(Holder.any());
            
            glPopMatrix();
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            if (bShadows)
            {
                // TODO: Shadow process
                glPushMatrix();
                glMultMatrixf(ShadowMat);
                glMultMatrixf(&ShapePose.column0.x);
                glDisable(GL_LIGHTING);
                glColor4f(0.1f, 0.2f, 0.3f, 1.f);
                RenderGeometryHolder(Holder.any());
                glEnable(GL_LIGHTING);
                glPopMatrix();
            }
        }
    }
}

void RenderScene()
{
    PxScene* CurrentScene;
    PxGetPhysics().getScenes(&CurrentScene, 1);
    PxU32 nbActors = CurrentScene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC);
    //printf("there are %u actors in current scene\n", nbActors);
    
    if (nbActors > 0)
    {
        std::vector<PxRigidActor*> Actors(nbActors);
        CurrentScene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC, 
            reinterpret_cast<PxActor**>(&Actors[0]), nbActors);

        // TODO: Render Actors
        RenderActors(&Actors[0], static_cast<PxU32>(Actors.size()), false, {0.f, 0.7f, 0.f});
    }
}

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
    
    // if (ImGui::RadioButton("SolidCube: ", GeomType == 0))
    // {
    //     GeomType = 0;
    // }
    // ImGui::SliderInt("Cube Size: ", &CubeSize, 1, 50);
    // ImGui::Separator();

    // if (ImGui::RadioButton("WireCube: ", GeomType == 1))
    // {
    //     GeomType = 1;
    // }
    // ImGui::SliderInt("Wire Cube Size: ", &WireCubeSize, 1, 50);
    // ImGui::Separator();

    // if (ImGui::RadioButton("WireTeapot: ", GeomType == 2))
    // {        
    //     GeomType = 2;
    // }
    // ImGui::SliderInt("Wire Teapot Size: ", &WireTeapotSize, 1, 50);
    // ImGui::Separator();

    // if (ImGui::RadioButton("SolidTeapot: ", GeomType == 3))
    // {        
    //     GeomType = 3;
    // }
    // ImGui::SliderInt("Solid Teapot Size: ", &SolidTeapotSize, 1, 50);
    // ImGui::Separator();

    // if (ImGui::RadioButton("SolidSphere: ", GeomType == 4))
    // {        
    //     GeomType = 4;
    // }
    // ImGui::SliderInt("Solid Sphere Size: ", &SolidSphereRadius, 1, 50);
    // ImGui::SliderInt("Solid Sphere Slice: ", &SolidSphereSlice, 12, 64);
    // ImGui::SliderInt("Solid Sphere Stack: ", &SolidSphereStack, 12, 64);
    // ImGui::Separator();

    // if (ImGui::RadioButton("WireSphere: ", GeomType == 5))
    // {        
    //     GeomType = 5;
    // }
    // ImGui::SliderInt("Wire Sphere Size: ", &WireSphereRadius, 1, 50);
    // ImGui::SliderInt("Wire Sphere Slice: ", &WireSphereSlice, 12, 64);
    // ImGui::SliderInt("Wire Sphere Stack: ", &WireSphereStack, 12, 64);
    // ImGui::Separator();

    ImGui::SliderInt("PhysX Box Size: ", &PhysXBoxSize, 1, 10);
    ImGui::DragFloat3("PhysX Box Init Position: ", (float*)&PhysXBoxInitPos, 0.1f);
    if (ImGui::Button("Add PhysX Box"))
    {
        CreatePhysXBoxGeometry("Box", PhysXBoxSize, PhysXBoxInitPos);
    }

    ImGui::End();
}



static void OnCustomTick(float DeltaTime)
{
    //printf("DeltaTime: %f, %d\n", DeltaTime, glutGet(GLUT_ELAPSED_TIME));

    RenderScene();
    StepPhysics();
}

int main(int argc, char** argv)
{
    GlutWindow::bUseGUI = true;
    GlutWindow::bUseDarkStyle = false;

    GlutWindow::OnDrawCallback = OnCustomTick;
    GlutWindow::OnGUICallback = OnCustomGUI;

    InitializePhysics();

    GlutWindow::GetInstance(argc, argv, "Window Title", 1280, 720, false)->Show();
    
    CleanupPhysics();

    return 0;
}