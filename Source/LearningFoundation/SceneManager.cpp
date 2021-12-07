#include "SceneManager.h"
#include "GL/freeglut.h"
#include "PxPhysicsAPI.h"

using namespace physx;

SceneManager* SceneManager::GetInstance() 
{
    if (Inst == nullptr)
    {
        Inst = new SceneManager();
    }

    return Inst;
}

void SceneManager::Update(float DeltaTime)
{

}

void SceneManager::DrawLine(const glm::vec3& StartPt, const glm::vec3& StopPt)
{
    glBegin(GL_LINES);
    glVertex3d(InBegin.x, InBegin.y, InBegin.z);
    glVertex3d(InEnd.x, InEnd.y, InEnd.z);
    glEnd();
}

void SceneManager::DrawGrid()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(0.1f,0.9f,0.2f); 
    glPointSize(3.0);  
    for (int x = -1000; x < 1000; x+=10)
    {
        glm::vec3 StartPt = {x, 0, -1000};
        glm::vec3 EndPt = {x , 0, 1000};
        DrawLine(StartPt, EndPt);
    }

    for (int z = -1000; z < 1000; z+=10)
    {
        glm::vec3 StartPt = {-1000, 0, z};
        glm::vec3 EndPt = { 1000, 0, z};
        DrawLine(StartPt, EndPt);
    }
}

void SceneManager::RenderGeometryHolder(const PxGeometry& Geom)
{
    switch (Geom.getType())
    {
        case PxGeometryType::eBOX:
            {
                const PxBoxGeometry& BoxGeom = static_cast<const PxBoxGeometry&>(Geom);
                glScalef(BoxGeom.halfExtents.x, BoxGeom.halfExtents.y, BoxGeom.halfExtents.z);
                glutSolidCube(PhysXBoxSize);            
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

void SceneManager::RenderShape(const physx::PxShape& InShape, const physx::PxRigidActor& InActor, PxVec3& InColor)
{
    const PxTransform ShapeTrans = PxShapeExt::getGlobalPose(InShape, InActor);
    const PxMat44 ShapePose(ShapeTrans);
    const PxGeometryHolder Holder = InShape.getGeometry();
    const bool bIsTrigger = InShape.getFlags() & PxShapeFlag::eTRIGGER_SHAPE;
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

void SceneManager::RenderBodyInstance(PxRigidActor* InActor, bool bShadows, const PxVec3& InColor)
{
    const PxVec3 ShadowDir(0.f, 0.7f, 0.7f);
    const PxReal ShadowMat[] = {1, 0, 0, 0, -ShadowDir.x/ShadowDir.y, 0, -ShadowDir.z/ShadowDir.y, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    PxShape* Shapes[128];

    const PxU32 NbShapes = InActor->getNbShapes();
    
    PX_ASSERT(NbShapes <= 128);
    InActor->getShapes(Shapes, NbShapes);

    const bool bIsSleeping = InActor->is<PxRigidDynamic>() ? InActors[i]->is<PxRigidDynamic>()->isSleeping() : false;

}

void SceneManager::RenderActors(PxRigidActor** InActors, const PxU32 NumActors, bool bShadows, const PxVec3& InColor)
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

void SceneManager::RenderScene()
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
        RenderActors(&Actors[0], static_cast<PxU32>(Actors.size()), true, {0.f, 0.7f, 0.f});
    }

    for (const ActorInfo& Actor: AllActors)
    {
        RenderActors
    }
}

const ActorInfo* SceneManager::CreateCubeActor(float InSize, const glm::vec3& InLoc, const glm::vec3& InRot)
{

}