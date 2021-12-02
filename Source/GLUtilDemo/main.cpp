# pragma warning (disable:4819)

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cstdio>
#include <string.h>
#include <iostream>
#include <memory>
#include <vector>
#include "learning.h"
#include "PxPhysicsAPI.h"
#include "CommonDefines.h"
#include "GL/glut.h"
#include "gl/GLU.h"

extern glm::vec4 BackgroundColor;

#define PX_RELEASE(x)	if(x)	{ x->release(); x = NULL;	}

using namespace physx;

LearningCamera* GlobalCamera = nullptr;

PxDefaultAllocator gAllocator;
PxDefaultErrorCallback gErrorCallback;

PxFoundation* gFoundation = nullptr;
PxPhysics* gPhysics = nullptr;
PxDefaultCpuDispatcher* gDispatcher = nullptr;
PxScene* gScene = nullptr;
PxMaterial* gMaterial = nullptr;
PxPvd* gPvd = nullptr;

static float Shininess = 0.01f;
GLScene MainScene(__back * 6.f);

//std::unique_ptr<GLCubic> Cube = nullptr;
//std::unique_ptr<Shader> GlobalShader = nullptr;

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
    GroundPlane->setName("Plane");
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
    Body->setName("CubeActor");

    Body->attachShape(*Shape);
    PxRigidBodyExt::updateMassAndInertia(*Body, 10.f);
    gScene->addActor(*Body);

    // 因为Shape通过值引用复制到了Body中，所以这里可以先销毁
    Shape->release();

    printf("Initialize physics succeed...\n");
}

void InitializeScene()
{
    InitializePhysics();
    
    GlobalCamera = new LearningCamera(PxVec3(50.0f, 50.0f, 50.0f), PxVec3(-0.6f,-0.2f,-0.7f));
}

void StepPhysics()
{
    gScene->simulate(1.f / 90.f);
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

                glutSolidCube(2);
            
                printf("Finished draw a cube with box_extent: %f, %f, %f\n", 
                    BoxGeom.halfExtents.x, BoxGeom.halfExtents.y, BoxGeom.halfExtents.z);
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
            printf("shape pose x: %f, y: %f, z: %f\n", ShapePose.column0.x , 
                ShapePose.column0.y, ShapePose.column0.z);
            if (bIsSleeping)
            {
                const PxVec3 DarkColor = InColor * .25f;
                glColor4f(DarkColor.x, DarkColor.y, DarkColor.z, 1.f);
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
    ImGui::Begin("Px Simple Demo"); 
    
    ImGui::Text("Material:");               // Display some text (you can use a format strings too)
    ImGui::SliderFloat("Shininess", &Shininess, 0.0f, 256.0f);

    ImGui::Text("Light - Phong:");
    ImGui::DragFloat3("phong.ambient", reinterpret_cast<float*>(&(MainScene.phong_model.ambient)), .01f, 0.0f, 1.0f);
    ImGui::DragFloat3("phong.diffuse", reinterpret_cast<float*>(&(MainScene.phong_model.diffuse)), .01f, 0.0f, 1.0f);
    ImGui::DragFloat3("phong.specular", reinterpret_cast<float*>(&(MainScene.phong_model.specular)), .01f, 0.0f, 1.0f);

    ImGui::Text("Light - Direction:");
    ImGui::DragFloat3("direction-light.direction", reinterpret_cast<float*>(&(MainScene.FirstDirectionLight()->direction)), .1f);

    ImGui::Text("Light - Point:");
    ImGui::DragFloat3("point-light.position", reinterpret_cast<float*>(&(MainScene.FirstPointLight()->transform.position)), .1f);
    ImGui::SliderFloat("point-light.constant", static_cast<float*>(&(MainScene.FirstPointLight()->constant)), 0.001f, 1.0f);
    ImGui::SliderFloat("point-light.linear", static_cast<float*>(&(MainScene.FirstPointLight()->linear)), 0.001f, 1.0f);
    ImGui::SliderFloat("point-light.quadratic", static_cast<float*>(&(MainScene.FirstPointLight()->quadratic)), 0.001f, 1.0f);

    ImGui::Text("Light - Spot:");
    ImGui::DragFloat3("spot-light.direction", reinterpret_cast<float*>(&(MainScene.FirstSpotLight()->direction)), .1f);
    ImGui::DragFloat3("spot-light.position", reinterpret_cast<float*>(&(MainScene.FirstSpotLight()->transform.position)), .1f);
    ImGui::SliderFloat("spot-light.cutoff", &(MainScene.FirstSpotLight()->cutoff), 0.0f, 179.0f);
    ImGui::SliderFloat("spot-light.outerCutoff", &(MainScene.FirstSpotLight()->outerCutoff), 0.0f, 179.0f);
    ImGui::SliderFloat("spot-light.constant", static_cast<float*>(&(MainScene.FirstSpotLight()->constant)), 0.001f, 1.0f);
    ImGui::SliderFloat("spot-light.linear", static_cast<float*>(&(MainScene.FirstSpotLight()->linear)), 0.001f, 1.0f);
    ImGui::SliderFloat("spot-light.quadratic", static_cast<float*>(&(MainScene.FirstSpotLight()->quadratic)), 0.001f, 1.0f);

    ImGui::Text("Camera:");
    ImGui::SliderFloat("FOV", &(MainScene.MainCamera.get()->Zoom), 0.0f, 170.0f);
    ImGui::DragFloat3("camera.position", reinterpret_cast<float*>(&(MainScene.MainCamera.get()->Position)), .1f);
    ImGui::SliderFloat("Camera.Yaw", static_cast<float*>(&MainScene.MainCamera->Yaw), 0.f, 360.f, "%.2f");
    ImGui::SliderFloat("Camera.Pitch", static_cast<float*>(&MainScene.MainCamera->Pitch), 0.f, 360.f, "%.2f");
    
    ImGui::Text("Background Color:");
    ImGui::ColorEdit3("Background Color", reinterpret_cast<float*>(&BackgroundColor)); // Edit 3 floats representing a color

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
}

void RenderCamera(const PxVec3& cameraEye, const PxVec3& cameraDir, physx::PxReal clipNear = 1.f, physx::PxReal clipFar = 10000.f)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Setup camera
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
    printf("hit here...1\n");
	gluPerspective(60.0, GLdouble(glutGet(GLUT_WINDOW_WIDTH)) / GLdouble(glutGet(GLUT_WINDOW_HEIGHT)), GLdouble(clipNear), GLdouble(clipFar));
    printf("hit here...2\n");

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(GLdouble(cameraEye.x), GLdouble(cameraEye.y), GLdouble(cameraEye.z), GLdouble(cameraEye.x + cameraDir.x), GLdouble(cameraEye.y + cameraDir.y), GLdouble(cameraEye.z + cameraDir.z), 0.0, 1.0, 0.0);

	glColor4f(0.4f, 0.4f, 0.4f, 1.0f);
}

void OnTick(float DeltaTime)
{
    StepPhysics();   

    if (GlobalCamera)
    {
        RenderCamera(GlobalCamera->getEye(), GlobalCamera->getDir());
    }

    RenderScene();
}

int main()
{
    // 创建窗口
    if (GLCreateWindow(FCreateWindowParameters::DefaultWindowParameters()) < 0)
    {
        return EXIT_FAILURE;
    }
    
    InitializeScene();
    
    // Logic Loop
    GLWindowTick(OnTick, OnGUI);

    GLDestroyWindow();

    return EXIT_SUCCESS;
}