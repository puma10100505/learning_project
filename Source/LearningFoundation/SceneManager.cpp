
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
    Camera = new LearningCamera(PxVec3{0.f, 100.f, 0.f}, PxVec3{0.f, 0.f, 1.f});
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

    if (bPhysicsSimulateEnabled && PhysMgr)
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
    
    for (GameObject& Actor: AllActors)
    {
        PxRigidActor* RigidActor =  static_cast<PxRigidActor*>(Actor.BodyInstance);
        if (RigidActor == nullptr)
        {
            continue;
        }

        PxShape* Shapes[64];
        const PxU32 NbShapes = RigidActor->getNbShapes();
        PX_ASSERT(NbShapes <= 64);
        RigidActor->getShapes(Shapes, NbShapes);

        if (NbShapes > 0)
        {
            PxShape* MainShape = Shapes[0];

            PxTransform Trans = PxShapeExt::getGlobalPose(*MainShape, *RigidActor);

            Actor.Transform.Location = Trans.p;
            Actor.Transform.Rotation = Trans.q;
    
            RenderActor(*MainShape, *RigidActor, Actor);        
        }

    }
}

void SceneManager::RenderActor(const PxShape& InShape, const PxRigidActor& InPxActor, const GameObject& InGameObj)
{
    const PxTransform ShapeTrans = PxShapeExt::getGlobalPose(InShape, InPxActor);
    const PxMat44 ShapePose(ShapeTrans);
    const PxGeometryHolder Holder = InShape.getGeometry();
    const bool bIsTrigger = InShape.getFlags() & PxShapeFlag::eTRIGGER_SHAPE;
    const PxReal ShadowMat[] = {1, 0, 0, 0, -ShadowDir.x/ShadowDir.y, 0, -ShadowDir.z/ShadowDir.y, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    if (bIsTrigger)
    {
        // 如果Shape是Trigger区域，则只渲染外框
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    glPushMatrix();
    glMultMatrixf(&ShapePose.column0.x);
    glColor4f(InGameObj.Color.x, InGameObj.Color.y, InGameObj.Color.z, 1.f);

    RenderGeometryHolder(Holder.any(), InPxActor);
    
    glPopMatrix();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if (bEnableShadow)
    {
        glPushMatrix();
        glMultMatrixf(ShadowMat);
        glMultMatrixf(&ShapePose.column0.x);
        glDisable(GL_LIGHTING);
        glColor4f(0.1f, 0.2f, 0.3f, 1.f);

        RenderGeometryHolder(Holder.any(), InPxActor);

        glEnable(GL_LIGHTING);
        glPopMatrix();
    }
}

void SceneManager::RenderGeometryHolder(const PxGeometry& Geom, const PxRigidActor& InActor)
{
    switch (Geom.getType())
    {
    case PxGeometryType::eBOX:
        {
            const PxBoxGeometry& BoxGeom = static_cast<const PxBoxGeometry&>(Geom);
            glScalef(BoxGeom.halfExtents.x, BoxGeom.halfExtents.y, BoxGeom.halfExtents.z);
            glutSolidCube(BoxGeom.halfExtents.x * 2);
            
            break;
        }
    case PxGeometryType::eSPHERE:

        {
            GameObject* Info = static_cast<GameObject*>(InActor.userData);
            if (Info)
            {
                const PxSphereGeometry& SphereGeom = static_cast<const PxSphereGeometry&>(Geom);                
                glutSolidSphere(SphereGeom.radius, Info->Slices, Info->Stacks);
            }
            break;
        }

    case PxGeometryType::eCAPSULE:
        {
            break;
        }
    default: ;
    }
}



GameObject* SceneManager::CreateCubeActor(const std::string& Name, float InSize, const PxVec3& InLoc, 
    const PxQuat& InRot, const bool InIsTrigger, const bool IsStatic)
{
    if (PhysMgr == nullptr) 
    {
        return nullptr;
    }

    PxRigidActor* Body = PhysMgr->CreateBoxGeometry(Name, InSize, PxTransform(InLoc, InRot), InIsTrigger, IsStatic);

    if (Body == nullptr)
    {
        return nullptr;
    }

    AllActors.push_back({ Name, Body });
    GameObject* NewActor = &AllActors[AllActors.size() - 1];
    NewActor->ActorIndex = AllActors.size() - 1;
    NewActor->Transform.Location = InLoc;
    NewActor->Transform.Rotation = InRot;
    
    Body->userData = NewActor;

    return NewActor;
}

GameObject* SceneManager::CreateSphereActor(const std::string& Name, float InRadius, const PxVec3& InLoc, const int InSlices,
        const int InStacks, const bool InIsTrigger, const bool IsStatic)
{
    if (PhysMgr == nullptr) 
    {
        return nullptr;
    }

    PxRigidActor* Body = PhysMgr->CreateSphereGeometry(Name, InRadius, 
        physx::PxTransform(physx::PxVec3{InLoc.x, InLoc.y, InLoc.z}, physx::PxQuat{PxIDENTITY::PxIdentity}), InIsTrigger, IsStatic);

    if (Body == nullptr)
    {
        return nullptr;
    }

    AllActors.push_back({ Name, Body });
    GameObject* NewActor = &AllActors[AllActors.size() - 1];
    NewActor->ActorIndex = AllActors.size() - 1;
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