#pragma once

#include <vector>
#include <memory>
#include "glm/glm.hpp"
#include "PxPhysicsAPI.h"
#include "PhysicsManager.h"

using namespace std;

typedef struct _stActorInfo
{
public:
    struct FTransform
    {
    public:
        PxVec3 Location;
        PxQuat Rotation;
        PxVec3 Scale;
    } Transform;
    
    std::string Name;
    class physx::PxActor* BodyInstance;
    int Slices;
    int Stacks;
    int ActorIndex;
    bool IsStatic;
    enum EGeomType
    {
        Cube,
        Sphere,
        Cone,
        Teapot
    };

    EGeomType GeomType;
    PxVec3 Color;

    _stActorInfo(const std::string& InName, class physx::PxActor* BodyInst)
        : Name(InName), BodyInstance(BodyInst) { }
} GameObject;

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

    GameObject* CreateCubeActor(const std::string& Name, float InSize, const PxVec3& InLoc, 
        const PxQuat& InRot, const bool InIsTrigger = false, const bool IsStatic = true);

    GameObject* CreateSphereActor(const std::string& Name, float InRadius, const PxVec3& InLoc,
        const int InSlices = 12,  const int InStacks = 12, const bool InIsTrigger = false, const bool IsStatic = true);

    GameObject* CreateTorusActor(const std::string& Name, const float InInnerRadius, const float InOuterRadius,
        int InSides, int InRings, const bool InIsTrigger = false, const bool IsStatic = true);

    GameObject* CreateCylinderActor(const std::string& Name, const float InRadius, const float InHeight,
        const int InSlices, const int InStacks, const bool InIsTrigger = false, const bool IsStatic = true);

    GameObject* CreateConeActor(const std::string& Name, const float InBase, const float InHeight,
        const int InSlices, const int InStacks, const bool InIsTrigger = false, const bool IsStatic = true);

    class LearningCamera* GetCamera() { return Camera; }

    class PhysicsManager* GetPhysics() const { return PhysMgr; }

    uint32_t GetActorCount() { return AllActors.size(); }

    const GameObject* GetActorByIndex(int Idx) 
    {
        if (Idx < 0 || Idx >= AllActors.size())
        {
            return nullptr;
        }

        return &AllActors[Idx]; 
    }

    void RenderLine(const glm::vec3& InBegin, const glm::vec3& InEnd);

    const bool IsPhysicsEnabled() { return bPhysicsSimulateEnabled; }
    void SetPhysicsEnabled(const bool InEnable) { bPhysicsSimulateEnabled = InEnable; }

protected:    
    void RenderGrid();    
    void RenderSceneActors();
    void RenderCamera();

    void RenderGeometryHolder(const PxGeometry& Geom, const PxRigidActor& InActor);
    void RenderActor(const PxShape& InShape, const PxRigidActor& InPxActor, const GameObject& InGameObj);

protected:
    static SceneManager* Inst;
    class PhysicsManager* PhysMgr = nullptr;
    std::vector<GameObject> AllActors;
    physx::PxVec3 ShadowDir;
    bool bEnableShadow = false;
    class LearningCamera* Camera = nullptr;
    int SceneWidth = 0;
    int SceneHeight = 0;
    bool bPhysicsSimulateEnabled = false;
};