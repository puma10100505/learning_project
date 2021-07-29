#include <cstdio>
#include <string.h>
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include "learning.h"
#include "PxPhysicsAPI.h"
#include "CommonDefines.h"

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

GLScene MainScene(__back * 6.f);
GLCubic* Cube = nullptr;


static void WindowKeyCallback(GLFWwindow* InWindow, int Key, int ScanCode, int Action, int Mods)
{
    if (glfwGetKey(InWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(InWindow, true);
    }
    else
    {
        if (glfwGetKey(InWindow, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS &&
            glfwGetKey(InWindow, GLFW_KEY_ENTER) == GLFW_PRESS)
        {
            const GLFWvidmode* mode = glfwGetVideoMode(GetPrimaryMonitor());
            if (!mode)
            {
                std::cout << "Not found video mode" << std::endl;
                return;
            }

            if (bIsFullScreen)
            {
                ScreenWidth = WINDOW_WIDTH;
                ScreenHeight = WINDOW_HEIGHT;
                glfwSetWindowMonitor(InWindow, nullptr, 100, 100, ScreenWidth, ScreenHeight, mode->refreshRate);
                bIsFullScreen = false;
                printf("toggle to windowed screen mode, width: %d, height: %d\n", ScreenWidth, ScreenHeight);
            }
            else
            {
                ScreenWidth = mode->width;
                ScreenHeight = mode->height;
                glfwSetWindowMonitor(InWindow, GetPrimaryMonitor(), 0, 0, ScreenWidth, ScreenHeight, mode->refreshRate);
                bIsFullScreen = true;
                printf("toggle to full screen mode, width: %d, height: %d\n", ScreenWidth, ScreenHeight);
            }
        }
    }
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
    PxRigidStatic* GroundPlane = PxCreatePlane(*gPhysics, PxPlane(0, 1, 0, 0), *gMaterial);
    gScene->addActor(*GroundPlane);

    // 创建物理形状
    PxShape* Shape = gPhysics->createShape(PxBoxGeometry(5.f, 5.f, 5.f), *gMaterial);
    if (!Shape)
    {
        printf("Create shape failed\n");
        return;
    }

    PxRigidDynamic* Body = gPhysics->createRigidDynamic(PxTransform(0.f, 0.f, 0.f));
    if (!Body)
    {
        printf("Create body failed\n");
        return;
    }

    Body->attachShape(*Shape);
    PxRigidBodyExt::updateMassAndInertia(*Body, 10.f);
    gScene->addActor(*Body);

    // 因为Shape通过值引用复制到了Body中，所以这里可以先销毁
    Shape->release();

    printf("Initialize physics succeed...\n");
}

void InitializeScene()
{
    Cube = MainScene.AddCube(solution_base_path + "Assets/Shaders/multilights.object.vs", 
       solution_base_path + "Assets/Shaders/multilights.object.fs");
    Cube->SetDiffuseTexture(solution_base_path + "Assets/Textures/container2.png");
    Cube->SetSpecularTexture(solution_base_path + "Assets/Textures/container2_specular.png");

    Cube->transform.position = glm::vec3(0.f, 0.f, 0.f);

    MainScene.MainCamera.get()->Position = glm::vec3(-50.f, 80.f, 30.f);
    MainScene.MainCamera.get()->Pitch = -45.f;

    MainScene.MainCamera.get()->Zoom = 90.f;
}

void StepPhysics()
{
    gScene->simulate(1.f / 120.f);
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
            glScalef(BoxGeom.halfExtents.x, BoxGeom.halfExtents.y, BoxGeom.halfExtents.z);
            // glutSolidCube(2); TODO: 使用OPENGL原生方法渲染 Ref: learning_opengl
            
            printf("Finished draw a cube with box_extent: %f, %f, %f\n", 
                BoxGeom.halfExtents.x, BoxGeom.halfExtents.y, BoxGeom.halfExtents.z);
            break;
        }

        case PxGeometryType::eSPHERE:
        {
            break;
        }

        case PxGeometryType::eCAPSULE:
        {
            break;
        }
    }
}

void RenderActors(PxRigidActor** InActors, const PxU32 NumActors, bool bShadows, const PxVec3& InColor/*, TriggerRender* Callback*/)
{
    //const PxVec3 ShadowDir(0.f, 0.7f, 0.7f);
    //const PxReal ShadowMat[] = {1, 0, 0, 0, -ShadowDir.x/ShadowDir.y, 0, -ShadowDir.z/ShadowDir.y, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    PxShape* Shapes[128];
    for (PxU32 i = 0; i < NumActors; i++)
    {
        const PxU32 NbShapes = InActors[i]->getNbShapes();
        printf("There are %u shaped in actor: %s\n", NbShapes, InActors[i]->getName());

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
            if (bIsSleeping)
            {
                const PxVec3 DarkColor = InColor * .25f;
                glColor4f(DarkColor.x, DarkColor.y, DarkColor.z, 1.f);
                printf("actor is sleeping...\n");
            }
            else
            {
                glColor4f(InColor.x, InColor.y, InColor.z, 1.f);
            }

            //RenderGeometryHolder(Holder.any());
            switch (Holder.any().getType())
            {
                case PxGeometryType::eBOX:
                {
                    const PxBoxGeometry& BoxGeom = static_cast<const PxBoxGeometry&>(Holder.any());
                    glScalef(BoxGeom.halfExtents.x, BoxGeom.halfExtents.y, BoxGeom.halfExtents.z);

                    if (Cube)
                    {
                        Cube->transform.position = glm::vec3(ShapeTrans.p.x, ShapeTrans.p.y, ShapeTrans.p.z);
                        Cube->transform.rotation = glm::eulerAngles(
                            glm::quat(ShapeTrans.q.w, ShapeTrans.q.x, ShapeTrans.q.y, ShapeTrans.q.z));
                        
                        printf("Finished draw a cube with pos: %f, %f, %f\n", 
                            Cube->transform.rotation.x, Cube->transform.rotation.y, Cube->transform.rotation.z);

                        Cube->BeforeDraw();
                        Cube->Draw();
                    }
                    break;
                }

                case PxGeometryType::eSPHERE:
                {
                    break;
                }

                case PxGeometryType::eCAPSULE:
                {
                    break;
                }
            }
            glPopMatrix();
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

            if (bShadows)
            {
                // TODO: Shadow process
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

void OnGUI(float DeltaTime)
{
    // TODO:
}

void OnTick(float DeltaTime)
{
    // TODO:
}

int main()
{
    int iRet = 0;

    do {
        if ((iRet = InitGlfwWindow()) < 0) {
            std::cout << "init window failed\n";
            break;
        }

        FCreateWindowParameters Params {
            ScreenWidth, 
            ScreenHeight, 
            "Sample Window for Px", 
            false, 
            16, 
            WindowKeyCallback};

        if ((iRet = GLCreateWindow(Params)) < 0) {
            std::cout << "create window failed \n";
            break;
        }

        if ((iRet = GLInitGUI()) < 0) {
            std::cout << "init gui failed\n";
            break;
        }
    } while (false);

    if (iRet < 0) {
        std::cout << "init application failed\n";
        return EXIT_FAILURE;
    }

    InitializePhysics();

    InitializeScene();

    GLWindowTick(OnTick, OnGUI);

    GLDestroyGUI();
    GLDestroyWindow();

    return EXIT_SUCCESS;
}