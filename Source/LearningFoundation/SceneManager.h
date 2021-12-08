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
public:
    std::string Name;
    class physx::PxActor* BodyInstance;
    int Slices;
    int Stacks;

    _stActorInfo(const std::string& InName, class physx::PxActor* BodyInst)
        : Name(InName), BodyInstance(BodyInst) { }
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
        const glm::vec3& InRot, const bool InIsTrigger = false, const glm::vec3& InVel = {0.f, 0.f, 0.f});

    ActorInfo* CreateSphereActor(const std::string& Name, float InRadius, const glm::vec3& InLoc, const int InSlices = 12,
        const int InStacks = 12, const bool InIsTrigger = false, const glm::vec3& InVel = {0.f, 0.f, 0.f});

    ActorInfo* CreateTorusActor(const std::string& Name, const float InInnerRadius, const float InOuterRadius,
        int InSides, int InRings, const bool InIsTrigger = false, const glm::vec3& InVel = {0.f, 0.f, 0.f});

    ActorInfo* CreateCylinderActor(const std::string& Name, const float InRadius, const float InHeight,
        const int InSlices, const int InStacks, const bool InIsTrigger = false, const glm::vec3& InVel = {0.f, 0.f, 0.f});

    ActorInfo* CreateConeActor(const std::string& Name, const float InBase, const float InHeight,
        const int InSlices, const int InStacks, const bool InIsTrigger = false, const glm::vec3& InVel = {0.f, 0.f, 0.f});

    class LearningCamera* GetCamera() { return Camera; }

protected:
    void RenderLine(const glm::vec3& InBegin, const glm::vec3& InEnd);
    void RenderGrid();    
    void RenderSceneActors();
    void RenderCamera();
    

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