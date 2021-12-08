
#include "SceneManager.h"
#include "GL/freeglut.h"
#include "PxPhysicsAPI.h"
#include "glm/glm.hpp"
#include "PhysicsManager.h"
#include "LearningCamera.h"
#include <gl/GL.h>

using namespace physx;
using namespace std;

SceneManager* SceneManager::Inst = nullptr;

SceneManager::SceneManager(int InWidth, int InHeight, bool InEnableShadow)
    : SceneWidth(InWidth), SceneHeight(InHeight), bEnableShadow(InEnableShadow)
{
    ShadowDir = PxVec3{ 0.f, 0.7f, 0.7f };
    bEnableShadow = false;
    PhysMgr = new PhysicsManager(glm::vec3{0.f, -9.8f, 0.f}, true, true);
    Camera = new LearningCamera(PxVec3{0.f, 0.f, 0.f}, PxVec3{0.f, 0.f, 1.f});
}

SceneManager::~SceneManager()
{
    if (PhysMgr)
    {
        delete PhysMgr;
        PhysMgr = nullptr;
    }

    if (Camera)
    {
        delete Camera;
        Camera = nullptr;
    }
}

SceneManager* SceneManager::GetInstance()
{
    return Inst;
}

SceneManager* SceneManager::GetInstance(int InWidth, int InHeight, bool InEnableShadow) 
{
    if (Inst == nullptr)
    {
        Inst = new SceneManager(InWidth, InHeight, InEnableShadow);
    }

    return Inst;
}

void SceneManager::Update(float DeltaTime)
{
    RenderGrid();
    RenderCamera();
    RenderSceneActors();

    if (PhysMgr)
    {
        PhysMgr->Tick();
    }
}

void SceneManager::RenderLine(const glm::vec3& InBegin, const glm::vec3& InEnd)
{
    glBegin(GL_LINES);
    glVertex3d(InBegin.x, InBegin.y, InBegin.z);
    glVertex3d(InEnd.x, InEnd.y, InEnd.z);
    glEnd();
}

void SceneManager::RenderGrid()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(0.1f,0.9f,0.2f); 
    glPointSize(3.0);  
    for (int x = -1000; x < 1000; x+=10)
    {
        glm::vec3 StartPt = {x, 0, -1000};
        glm::vec3 EndPt = {x , 0, 1000};
        RenderLine(StartPt, EndPt);
    }

    for (int z = -1000; z < 1000; z+=10)
    {
        glm::vec3 StartPt = {-1000, 0, z};
        glm::vec3 EndPt = { 1000, 0, z};
        RenderLine(StartPt, EndPt);
    }
}


void SceneManager::RenderSceneActors()
{
    if (PhysMgr == nullptr)
    {
        return ;
    }
    
    for (const ActorInfo& Actor: AllActors)
    {
        PhysMgr->RenderBodyInstance(reinterpret_cast<PxRigidActor*>(Actor.BodyInstance),
            PxVec3{0.4f, 0.5f, 0.1f}, bEnableShadow, ShadowDir);
    }
}

ActorInfo* SceneManager::CreateCubeActor(const std::string& Name, float InSize, const glm::vec3& InLoc, 
    const glm::vec3& InRot, const bool InIsTrigger, const glm::vec3& InVel)
{
    if (PhysMgr == nullptr) 
    {
        return nullptr;
    }

    PxRigidDynamic* Body = PhysMgr->CreateBoxGeometry(Name, InSize, 
        physx::PxTransform(physx::PxVec3{InLoc.x, InLoc.y, InLoc.z}, physx::PxQuat{PxIDENTITY::PxIdentity}), 
        PxVec3{InVel.x, InVel.y, InVel.z}, InIsTrigger);

    if (Body == nullptr)
    {
        return nullptr;
    }

    AllActors.push_back({ Name, Body });
    ActorInfo* NewActor = &AllActors[AllActors.size() - 1];
    
    Body->userData = NewActor;

    return NewActor;
}

ActorInfo* SceneManager::CreateSphereActor(const std::string& Name, float InRadius, const glm::vec3& InLoc, const int InSlices,
        const int InStacks, const bool InIsTrigger, const glm::vec3& InVel)
{
    if (PhysMgr == nullptr) 
    {
        return nullptr;
    }

    PxRigidDynamic* Body = PhysMgr->CreateSphereGeometry(Name, InRadius, 
        physx::PxTransform(physx::PxVec3{InLoc.x, InLoc.y, InLoc.z}, physx::PxQuat{PxIDENTITY::PxIdentity}), 
        PxVec3{InVel.x, InVel.y, InVel.z}, InIsTrigger);

    if (Body == nullptr)
    {
        return nullptr;
    }

    AllActors.push_back({ Name, Body });
    ActorInfo* NewActor = &AllActors[AllActors.size() - 1];
    NewActor->Slices = InSlices;
    NewActor->Stacks = InStacks;
    Body->userData = NewActor;

    return NewActor;
}

void SceneManager::RenderCamera()
{
    if (Camera == nullptr)
    {
        return; 
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(Camera->mFOV, GLdouble(SceneWidth) / GLdouble(SceneHeight), GLdouble(Camera->mClipNear), GLdouble(Camera->mClipFar));

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(GLdouble(Camera->getEye().x), GLdouble(Camera->getEye().y), GLdouble(Camera->getEye().z), 
        GLdouble(Camera->getEye().x + Camera->getDir().x), GLdouble(Camera->getEye().y + Camera->getDir().y), 
        GLdouble(Camera->getEye().z + Camera->getDir().z), 0.0, 1.0, 0.0);
}