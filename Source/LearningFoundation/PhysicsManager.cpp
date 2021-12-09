#include "PhysicsManager.h"

#include <SceneManager.h>
#include <GL/freeglut_std.h>

#include "PxPhysicsAPI.h"
#include "gl/GL.h"

using namespace physx;

void PhysicsManager::Initialize()
{
    Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, DefaultAllocator, ErrorCallback);
    Pvd = PxCreatePvd(*Foundation);
    Pvd->connect(*PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10), PxPvdInstrumentationFlag::eALL);
    PhysicsInterface = PxCreatePhysics(PX_PHYSICS_VERSION, *Foundation, PxTolerancesScale(), true, Pvd);

    physx::PxSceneDesc SceneDesc(PhysicsInterface->getTolerancesScale());
    SceneDesc.gravity = PxVec3(Gravity.x, Gravity.y, Gravity.z);
    CpuDispatcher = PxDefaultCpuDispatcherCreate(2);
    SceneDesc.cpuDispatcher = CpuDispatcher;
    SceneDesc.filterShader = PxDefaultSimulationFilterShader;
    Scene = PhysicsInterface->createScene(SceneDesc);

    if (bUsePvd)
    {
        PvdClient = Scene->getScenePvdClient();
        if (PvdClient)
        {
            PvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
            PvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
            PvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
        }
    }

    PhysicsMaterial = PhysicsInterface->createMaterial(StaticFriction, DynamicFriction, Restitution);

    if (bUseWorldPlane)
    {
        CreateWorldPlane(0.f);
    }
}

PxRigidStatic* PhysicsManager::CreateWorldPlane(float normx, float normy, float normz, float distance)
{
    if (Scene == nullptr)
    {
        return nullptr;
    }
    
    physx::PxRigidStatic* GroundPlane = PxCreatePlane(*PhysicsInterface, PxPlane(normx, normy, normz, distance), *PhysicsMaterial);
    GroundPlane->setName("WorldPlane");
    Scene->addActor(*GroundPlane);

    return GroundPlane;
}

PxRigidStatic* PhysicsManager::CreateWorldPlane(const glm::vec3& normal, float distance)
{
    return CreateWorldPlane(normal.x, normal.y, normal.z, distance);
}

PxRigidStatic* PhysicsManager::CreateWorldPlane(float distance)
{
    return CreateWorldPlane(0.f, 1.f, 0.f, distance);
}

void PhysicsManager::Tick()
{
    if (Scene)
    {
        Scene->simulate(1.f / FPS);
        Scene->fetchResults(true);
    }
}

PhysicsManager::~PhysicsManager()
{
    PX_RELEASE(Scene);
    PX_RELEASE(CpuDispatcher);
    PX_RELEASE(PhysicsInterface);
    if (bUsePvd && Pvd)
    {
        physx::PxPvdTransport* Transport = Pvd->getTransport();
        Pvd->release();
        Pvd = nullptr;
        PX_RELEASE(Transport);
    }

    PX_RELEASE(Foundation);
}

PxRigidDynamic* PhysicsManager::CreateSphereGeometry(const std::string& Name, float InRadius, const PxTransform& InTransform, const PxVec3& InVel, const bool IsTrigger) const 
{
    if (PhysicsInterface == nullptr)
    {
        return nullptr;
    }

    if (Scene == nullptr)
    {
        return nullptr;
    }

    PxShape* Shape = PhysicsInterface->createShape(PxSphereGeometry(InRadius / 2), *PhysicsMaterial);
    if (Shape == nullptr)
    {
        printf("Create shape failed\n");
        return nullptr;
    }

    Shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, !IsTrigger);
    Shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, IsTrigger);

    PxRigidDynamic* Body = PhysicsInterface->createRigidDynamic(InTransform);
    if (Body == nullptr)
    {
        printf("Create body failed\n");
        return nullptr;
    }

    Body->setName(Name.c_str());
    Body->setAngularDamping(0.5f);
    Body->setLinearVelocity(InVel);
    Body->attachShape(*Shape);    

    PxRigidBodyExt::updateMassAndInertia(*Body, 10.f);
    Scene->addActor(*Body);

    // 因为Shape通过值引用复制到了Body中，所以这里可以先销毁
    Shape->release();

    return Body;
}

PxRigidDynamic* PhysicsManager::CreateBoxGeometry(const std::string& Name, float InSize, const PxTransform& InTransform, const PxVec3& InVel, const bool IsTrigger) const
{
    if (PhysicsInterface == nullptr)
    {
        return nullptr;
    }

    if (Scene == nullptr)
    {
        return nullptr;
    }

    PxShape* Shape = PhysicsInterface->createShape(
        PxBoxGeometry(InSize / 2, InSize / 2, InSize / 2), *PhysicsMaterial);
    if (Shape == nullptr)
    {
        printf("Create shape failed\n");
        return nullptr;
    }

    Shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, !IsTrigger);
    Shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, IsTrigger);
    
    PxRigidDynamic* Body = PhysicsInterface->createRigidDynamic(InTransform);
    if (Body == nullptr)
    {
        printf("Create body failed\n");
        return nullptr;
    }

    Body->setName(Name.c_str());
    Body->setAngularDamping(0.5f);
	Body->setLinearVelocity(InVel);
    Body->attachShape(*Shape);

    PxRigidBodyExt::updateMassAndInertia(*Body, 10.f);
    Scene->addActor(*Body);

    // 因为Shape通过值引用复制到了Body中，所以这里可以先销毁
    Shape->release();

    return Body;
}

void PhysicsManager::RenderGeometryHolder(const PxGeometry& Geom, const PxRigidActor& InActor)
{
    switch (Geom.getType())
    {
    case PxGeometryType::eBOX:
        {
            const PxBoxGeometry& BoxGeom = static_cast<const PxBoxGeometry&>(Geom);
            glScalef(BoxGeom.halfExtents.x, BoxGeom.halfExtents.y, BoxGeom.halfExtents.z);
            if (Geom.getType() & PxShapeFlag::eTRIGGER_SHAPE)
            {
                glutWireCube(BoxGeom.halfExtents.x * 2);
                
            }
            else
            {
                glutSolidCube(BoxGeom.halfExtents.x * 2);            
            }
            
            break;
        }

    case PxGeometryType::eSPHERE:
        {
            ActorInfo* Info = static_cast<ActorInfo*>(InActor.userData);
            if (Info)
            {
                const PxSphereGeometry& SphereGeom = static_cast<const PxSphereGeometry&>(Geom);                
                if (SphereGeom.getType() & PxShapeFlag::eTRIGGER_SHAPE)
                {
                    glutWireSphere(SphereGeom.radius, Info->Slices, Info->Stacks);
                }
                else
                {
                    glutSolidSphere(SphereGeom.radius, Info->Slices, Info->Stacks);
                }                
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

void PhysicsManager::RenderShape(const physx::PxShape& InShape, const PxRigidActor& InActor, PxVec3& InColor,
    bool bEnableShadow, physx::PxVec3 InShadowDir)
{
    const PxTransform ShapeTrans = PxShapeExt::getGlobalPose(InShape, InActor);
    const PxMat44 ShapePose(ShapeTrans);
    const PxGeometryHolder Holder = InShape.getGeometry();
    const bool bIsTrigger = InShape.getFlags() & PxShapeFlag::eTRIGGER_SHAPE;
    const PxReal ShadowMat[] = {1, 0, 0, 0, -InShadowDir.x/InShadowDir.y, 0, -InShadowDir.z/InShadowDir.y, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    if (bIsTrigger)
    {
        // 如果Shape是Trigger区域，则只渲染外框
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    glPushMatrix();
    glMultMatrixf(&ShapePose.column0.x);
    glColor4f(InColor.x, InColor.y, InColor.z, 1.f);

    RenderGeometryHolder(Holder.any(), InActor);
    
    glPopMatrix();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if (bEnableShadow)
    {
        glPushMatrix();
        glMultMatrixf(ShadowMat);
        glMultMatrixf(&ShapePose.column0.x);
        glDisable(GL_LIGHTING);
        glColor4f(0.1f, 0.2f, 0.3f, 1.f);

        RenderGeometryHolder(Holder.any(), InActor);

        glEnable(GL_LIGHTING);
        glPopMatrix();
    }
}

void PhysicsManager::RenderBodyInstance(PxRigidActor* InActor, const PxVec3& InColor, const bool InEnableShadow, const PxVec3& InShadowDir)
{
    PxShape* Shapes[128];
    const PxU32 NbShapes = InActor->getNbShapes();
    
    PX_ASSERT(NbShapes <= 128);
    InActor->getShapes(Shapes, NbShapes);

    const bool bIsSleeping = InActor->is<PxRigidDynamic>() ? InActor->is<PxRigidDynamic>()->isSleeping() : false;

    PxVec3 ActorColor = InColor;
    if (bIsSleeping)
    {
        ActorColor = ActorColor * .25f;
    }

    for (PxU32 ShapeIndex = 0; ShapeIndex < NbShapes; ++ShapeIndex)
    {
        RenderShape(*Shapes[ShapeIndex], *InActor, ActorColor, InEnableShadow, InShadowDir);
    }
}
