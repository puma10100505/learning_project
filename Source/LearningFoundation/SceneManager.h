#pragma once

#include <vector>
#include "glm/glm.hpp"

using namespace std;

typedef struct _stActorInfo
{
    // 暂未使用
    // struct FTransform
    // {
    //     glm::vec3 Location;
    //     glm::vec3 Rotation;
    //     glm::vec3 Scale;
    // } Transform;

    class physx::PxActor* BodyInstance;
} ActorInfo;

class SceneManager
{
protected:
    SceneManager() {}

public:
    static SceneManager* GetInstance();
    ~SceneManager(){}

    void Update(float DeltaTime);
    void SetPhysicsManager(class PhysicsManager* InMgr) { PhysMgr = InMgr; }

    const ActorInfo* CreateCubeActor(float InSize, const glm::vec3& InLoc, const glm::vec3& InRot);

protected:
    void DrawLine(const glm::vec3& StartPt, const glm::vec3& StopPt);
    void DrawGrid();
    void RenderGeometryHolder(const PxGeometry& Geom);
    void RenderActors(PxRigidActor** InActors, const PxU32 NumActors, bool bShadows, const PxVec3& InColor);
    void RenderScene();

protected:
    SceneManager* Inst = nullptr;
    class PhysicsManager* PhysMgr = nullptr;
    std::vector<ActorInfo> AllActors;
};