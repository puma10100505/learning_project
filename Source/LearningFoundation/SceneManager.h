#pragma once

#include <vector>
#include <memory>
#include "glm/glm.hpp"
#include "PxPhysicsAPI.h"
#include "PhysicsManager.h"

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

    std::string Name;
    class physx::PxActor* BodyInstance;
} ActorInfo;

class SceneManager
{
private:
    SceneManager(int InWidth, int InHeight, bool InEnableShadow = false);

public:
    static SceneManager* GetInstance();
    static SceneManager* GetInstance(int InWidth, int InHeight, bool InEnableShadow = false);
    ~SceneManager();

    void Update(float DeltaTime);
    void SetEnableShadow(bool InEnableShadow) { bEnableShadow = InEnableShadow; }

    ActorInfo* CreateCubeActor(const std::string& Name, float InSize, const glm::vec3& InLoc, 
        const glm::vec3& InRot, const glm::vec3& InVel = {0.f, 0.f, 0.f});

    class LearningCamera* GetCamera() { return Camera; }

protected:
    void RenderLine(const glm::vec3& InBegin, const glm::vec3& InEnd);
    void RenderGrid();
    void RenderGeometryHolder(const physx::PxGeometry& Geom);
    void RenderSceneActors();
    void RenderBodyInstance(physx::PxRigidActor* InActor, const physx::PxVec3& InColor);
    void RenderCamera();
    void RenderShape(const physx::PxShape& InShape, const physx::PxRigidActor& InActor, physx::PxVec3& InColor);

protected:
    static SceneManager* Inst;
    class PhysicsManager* PhysMgr = nullptr;
    std::vector<ActorInfo> AllActors;
    physx::PxVec3 ShadowDir;
    bool bEnableShadow = false;
    class LearningCamera* Camera = nullptr;
    int SceneWidth = 0;
    int SceneHeight = 0;
};