#pragma once
#include "PxPhysicsAPI.h"
#include "glm/glm.hpp"
#include <string> 

#define PX_RELEASE(x)	if(x)	{ x->release(); x = NULL;	}

using namespace physx;

class PhysicsManager 
{
public:
    PhysicsManager(const glm::vec3& InGravity, bool HasWorldPlane, const glm::vec3& PhyMat, bool UsePvd)
        : Gravity(InGravity), bUseWorldPlane(HasWorldPlane), bUsePvd(UsePvd) 
    {
        StaticFriction = PhyMat.x;
        DynamicFriction = PhyMat.y;
        Restitution = PhyMat.z;
    }

    PhysicsManager(const glm::vec3& InGravity): Gravity(InGravity) { }

    PhysicsManager(const glm::vec3& InGravity, bool HasWorldPlane)
        : Gravity(InGravity), bUseWorldPlane(HasWorldPlane) { }

    PhysicsManager(const glm::vec3& InGravity, bool HasWorldPlane, bool UsePvd)
        : Gravity(InGravity), bUseWorldPlane(HasWorldPlane), bUsePvd(UsePvd) { }

    ~PhysicsManager();

protected:
    void Initialize();

public:
    class physx::PxRigidStatic* CreateWorldPlane(float distance);
    class physx::PxRigidStatic* CreateWorldPlane(float normx, float normy, float normz, float distance);
    class physx::PxRigidStatic* CreateWorldPlane(const glm::vec3& normal, float distance);
    class physx::PxRigidDynamic* CreateBoxGeometry(const std::string& Name, float InSize, const PxTransform& InTransform, const PxVec3& InVel);

    void Tick();

    void SetPhysicsFPS(int InVal) { FPS = InVal; } 

protected:
    class physx::PxPhysics* PhysicsInterface = nullptr;
    class physx::PxFoundation* Foundation = nullptr;
    class physx::PxDefaultAllocator DefaultAllocator;
    class physx::PxDefaultErrorCallback ErrorCallback; 
    class physx::PxDefaultCpuDispatcher* CpuDispatcher = nullptr;
    class physx::PxScene* Scene = nullptr; 
    class physx::PxPvd* Pvd = nullptr;
    class physx::PxPvdSceneClient* PvdClient = nullptr;
    class physx::PxMaterial* PhysicsMaterial = nullptr;

    float StaticFriction = .5f;
    float DynamicFriction = .5f; 
    float Restitution = .5f;

    glm::vec3 Gravity = glm::vec3(0.0f, -9.8f, 0.0f);

    bool bUsePvd = false; 
    bool bUseWorldPlane = true;

    float FPS = 30.f;
};