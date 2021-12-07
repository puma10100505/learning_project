#include "PhysicsManager.h"
#include "PxPhysicsAPI.h"

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
        CreateWorldPlane(10.f);
    }
}

PxRigidStatic* PhysicsManager::CreateWorldPlane(float normx, float normy, float normz, float distance)
{
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
    Scene->simulate(1.f / FPS);
    Scene->fetchResults(true);
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

physx::PxRigidDynamic* PhysicsManager::CreateBoxGeometry(const std::string& Name, float InSize, const PxTransform& InTransform, const PxVec3& InVel)
{
    if (PhysicsInterface == nullptr)
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