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

static bool bIsFullScreen = false;
int FullScreenWidth = 0;
int FullScreenHeight = 0;

int ScreenWidth = 1080;
int ScreenHeight = 720;

GLFWwindow* Window = nullptr;
GLFWmonitor* PrimaryMonitor = nullptr;

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


void FramebufferChangedCallback(GLFWwindow* InWindow, int InWidth, int InHeight)
{
    glViewport(0, 0, InWidth, InHeight);
}

void ProcessInput(GLFWwindow* InWindow, GLFWmonitor* InMonitor)
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
            const GLFWvidmode* mode = glfwGetVideoMode(InMonitor);
            if (!mode)
            {
                std::cout << "Not found video mode" << std::endl;
                return;
            }

            if (bIsFullScreen)
            {
                glfwSetWindowMonitor(InWindow, nullptr, 100, 100, 1280, 720, mode->refreshRate);
                bIsFullScreen = false;
                printf("toggle to windowed screen mode, width: %d, height: %d\n", 1280, 720);
            }
            else
            {
                glfwSetWindowMonitor(InWindow, InMonitor, 0, 0, FullScreenWidth, FullScreenHeight, mode->refreshRate);
                bIsFullScreen = true;
                printf("toggle to full screen mode, width: %d, height: %d\n", FullScreenWidth, FullScreenHeight);
            }
        }
    }
}

void WindowCloseCallback(GLFWwindow* InWindow)
{
    printf("Window will be closed...\n");
}

void WindowResizedCallback(GLFWwindow* window, int width, int height)
{
    printf("Window has been resized, width: %d, height: %d\n", width, height);
}

void ClearScreen()
{
    glClearColor(.2f, .3f, .3f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void WindowKeyCallback(GLFWwindow* InWindow, int Key, int ScanCode, int Action, int Mods)
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
            const GLFWvidmode* mode = glfwGetVideoMode(PrimaryMonitor);
            if (!mode)
            {
                std::cout << "Not found video mode" << std::endl;
                return;
            }

            if (bIsFullScreen)
            {
                glfwSetWindowMonitor(InWindow, nullptr, 100, 100, ScreenWidth, ScreenHeight, mode->refreshRate);
                bIsFullScreen = false;
                printf("toggle to windowed screen mode, width: %d, height: %d\n", ScreenWidth, ScreenHeight);
            }
            else
            {
                glfwSetWindowMonitor(InWindow, PrimaryMonitor, 0, 0, FullScreenWidth, FullScreenHeight, mode->refreshRate);
                bIsFullScreen = true;
                printf("toggle to full screen mode, width: %d, height: %d\n", FullScreenWidth, FullScreenHeight);
            }
        }
    }
}

int InitializeWindowAttributes()
{
/* 设置窗口属性 */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
#endif

    PrimaryMonitor = glfwGetPrimaryMonitor();
    if (!PrimaryMonitor)
    {
        std::cout << "Not found primary monitor" << std::endl;        
        return EXIT_FAILURE;
    }

    const GLFWvidmode* mode = glfwGetVideoMode(PrimaryMonitor);
    if (!mode)
    {
        std::cout << "Not found video mode object" << std::endl;
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    FullScreenWidth = mode->width;
    FullScreenHeight = mode->height;
    /* 设置窗口属性 */

    return EXIT_SUCCESS;
}

int CreateAndSetupWindow()
{
    Window = glfwCreateWindow(bIsFullScreen ? FullScreenWidth : ScreenWidth, 
        bIsFullScreen ? FullScreenHeight : ScreenHeight, "Learning PhysX Window", 
        bIsFullScreen ? PrimaryMonitor : nullptr, nullptr);
    if (!Window)
    {
        std::cout << "Failed to create glfw window" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }
    
    glfwMakeContextCurrent(Window);

    // Setup callbacks
    glfwSetWindowCloseCallback(Window, WindowCloseCallback);
    glfwSetWindowSizeCallback(Window, WindowResizedCallback);
    glfwSetFramebufferSizeCallback(Window, FramebufferChangedCallback);
    glfwSetKeyCallback(Window, WindowKeyCallback);

    glfwFocusWindow(Window);

    // 创建窗口之后，使用OPENGL之前要初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to init GLAD" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
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
                        Cube->transform.position.x = ShapeTrans.p.x;
                        Cube->transform.position.y = ShapeTrans.p.y;
                        Cube->transform.position.z = ShapeTrans.p.z;

                        Cube->transform.rotation = glm::eulerAngles(
                            glm::quat(ShapeTrans.q.w, ShapeTrans.q.x, ShapeTrans.q.y, ShapeTrans.q.z));
                        
                        // MainScene.MainCamera.get()->Position.x = ShapeTrans.p.x;
                        // MainScene.MainCamera.get()->Position.y = ShapeTrans.p.y;
                        // MainScene.MainCamera.get()->Position.z = ShapeTrans.p.z + 10.f;


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

int main()
{
    if (!glfwInit())
    {
        return EXIT_FAILURE;
    }

    // 初始化窗口属性
    InitializeWindowAttributes();

    // 创建并设置窗口对象
    CreateAndSetupWindow();

    // 设置OPENGL视口大小（OPENGL渲染区域）
    glViewport(0, 0, bIsFullScreen ? FullScreenWidth : ScreenWidth, 
        bIsFullScreen ? FullScreenHeight : ScreenHeight);

    InitializePhysics();

    InitializeScene();

    // 渲染循环
    while (!glfwWindowShouldClose(Window))
    {
        ClearScreen();

        // 刷新物理
        StepPhysics();

        // 渲染物理对象
        RenderScene(); 

        glfwSwapBuffers(Window);
        glfwPollEvents();
    }

    glfwTerminate();

    return EXIT_SUCCESS;
}