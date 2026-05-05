/*
 * Phys/PhysWorld.cpp
 * ------------------
 */

#include "PhysWorld.h"

#include "../Nav/NavTypes.h"

#if RECAST_NAVI_USE_PHYSX

#  include "PxPhysicsAPI.h"

#  include <cmath>
#  include <vector>

#  ifndef PX_RELEASE
#    define PX_RELEASE(x) \
        do               \
        {                \
            if (x)       \
            {            \
                (x)->release(); \
                (x) = nullptr;  \
            }            \
        } while (0)
#  endif

using namespace physx;

namespace
{

PxDefaultAllocator       gAllocator;
PxDefaultErrorCallback   gErrorCb;

PxFoundation*            gFoundation = nullptr;
PxPhysics*               gPhysics    = nullptr;
PxCooking*               gCooking    = nullptr;
PxDefaultCpuDispatcher*  gDispatcher = nullptr;
PxScene*                 gScene      = nullptr;
PxMaterial*              gMaterial   = nullptr;

bool gActive = false;

void ClearRigidStatics()
{
    if (!gScene) return;
    PxSceneWriteLock lock(*gScene);
    const PxU32 n = gScene->getNbActors(PxActorTypeFlag::eRIGID_STATIC);
    if (n == 0) return;
    std::vector<PxRigidActor*> actors(n);
    gScene->getActors(PxActorTypeFlag::eRIGID_STATIC, reinterpret_cast<PxActor**>(actors.data()), n);
    for (PxRigidActor* a : actors)
    {
        gScene->removeActor(*a);
        a->release();
    }
}

bool CookInputTriangleMesh(const InputGeometry& geom, PxTriangleMesh*& outMesh)
{
    outMesh = nullptr;
    const int nVerts = static_cast<int>(geom.Vertices.size() / 3);
    const int nTris  = static_cast<int>(geom.Triangles.size() / 3);
    if (nVerts < 3 || nTris < 1 || !gCooking || !gPhysics) return false;

    std::vector<PxVec3> pxVerts(static_cast<size_t>(nVerts));
    for (int i = 0; i < nVerts; ++i)
    {
        pxVerts[static_cast<size_t>(i)] = PxVec3(
            geom.Vertices[i * 3 + 0],
            geom.Vertices[i * 3 + 1],
            geom.Vertices[i * 3 + 2]);
    }

    std::vector<PxU32> pxIdx(static_cast<size_t>(nTris * 3));
    for (int t = 0; t < nTris * 3; ++t)
        pxIdx[static_cast<size_t>(t)] = static_cast<PxU32>(geom.Triangles[t]);

    PxTriangleMeshDesc desc;
    desc.points.count           = static_cast<PxU32>(nVerts);
    desc.points.stride          = sizeof(PxVec3);
    desc.points.data            = pxVerts.data();
    desc.triangles.count        = static_cast<PxU32>(nTris);
    desc.triangles.stride       = 3 * sizeof(PxU32);
    desc.triangles.data         = pxIdx.data();
    desc.flags                  = PxMeshFlags(); // 32-bit 索引

    if (!desc.isValid()) return false;

    PxDefaultMemoryOutputStream writeBuf(gAllocator);
    PxTriangleMeshCookingResult::Enum cookRes = PxTriangleMeshCookingResult::eSUCCESS;
    if (!gCooking->cookTriangleMesh(desc, writeBuf, &cookRes)) return false;

    PxDefaultMemoryInputData readBuf(writeBuf.getData(), writeBuf.getSize());
    outMesh = gPhysics->createTriangleMesh(readBuf);
    return outMesh != nullptr;
}

void AddTerrainActor(PxTriangleMesh* mesh)
{
    if (!mesh || !gPhysics || !gScene || !gMaterial) return;

    PxTriangleMeshGeometry geom(mesh, PxMeshScale(), PxMeshGeometryFlag::eDOUBLE_SIDED);
    PxRigidStatic*         body = gPhysics->createRigidStatic(PxTransform(PxIdentity));
    PxShape*               sh  = gPhysics->createShape(geom, *gMaterial, true);
    body->attachShape(*sh);
    sh->release();
    body->setName("NavTerrain");
    body->userData = nullptr;
    gScene->addActor(*body);
}

void AddObstacleActor(int index, const Obstacle& o)
{
    if (!gPhysics || !gScene || !gMaterial) return;

    PxRigidStatic* body = nullptr;
    PxShape*       sh   = nullptr;

    if (o.Shape == ObstacleShape::Box)
    {
        const PxVec3 he(o.SX, std::max(1e-4f, o.Height * 0.5f), o.SZ);
        PxBoxGeometry box(he);
        body = gPhysics->createRigidStatic(PxTransform(PxVec3(o.CX, o.Height * 0.5f, o.CZ)));
        sh     = gPhysics->createShape(box, *gMaterial, true);
    }
    else
    {
        // PhysX 胶囊默认沿 +X；绕 Z 转 90° 使其沿 +Y
        const float halfH = std::max(1e-4f, o.Height * 0.5f - o.Radius);
        PxCapsuleGeometry cap(o.Radius, halfH);
        const PxQuat      q = PxQuat(PxHalfPi, PxVec3(0.f, 0.f, 1.f));
        body                = gPhysics->createRigidStatic(PxTransform(PxVec3(o.CX, o.Height * 0.5f, o.CZ), q));
        sh                  = gPhysics->createShape(cap, *gMaterial, true);
    }

    if (!body || !sh) return;
    body->attachShape(*sh);
    sh->release();
    body->userData = reinterpret_cast<void*>(static_cast<uintptr_t>(index + 1));
    gScene->addActor(*body);
}

} // namespace

namespace PhysWorld
{

bool Init()
{
    if (gActive) return true;

    gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCb);
    if (!gFoundation) return false;

    const PxTolerancesScale scale;
    gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, scale, false, nullptr);
    if (!gPhysics)
    {
        gFoundation->release();
        gFoundation = nullptr;
        return false;
    }

    PxCookingParams cookParams(scale);
    cookParams.meshWeldTolerance = 0.001f;
    gCooking                     = PxCreateCooking(PX_PHYSICS_VERSION, *gFoundation, cookParams);
    if (!gCooking)
    {
        gPhysics->release();
        gPhysics = nullptr;
        gFoundation->release();
        gFoundation = nullptr;
        return false;
    }

    PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.f, -9.81f, 0.f);
    gDispatcher       = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.cpuDispatcher   = gDispatcher;
    sceneDesc.filterShader    = PxDefaultSimulationFilterShader;
    if (!sceneDesc.isValid())
    {
        gCooking->release();
        gCooking = nullptr;
        gPhysics->release();
        gPhysics = nullptr;
        gFoundation->release();
        gFoundation = nullptr;
        return false;
    }

    gScene = gPhysics->createScene(sceneDesc);
    if (!gScene)
    {
        PX_RELEASE(gDispatcher);
        gCooking->release();
        gCooking = nullptr;
        gPhysics->release();
        gPhysics = nullptr;
        gFoundation->release();
        gFoundation = nullptr;
        return false;
    }

    gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.1f);
    gActive   = true;
    return true;
}

void Shutdown()
{
    if (!gActive && !gFoundation && !gPhysics) return;

    ClearRigidStatics();

    PX_RELEASE(gMaterial);
    PX_RELEASE(gScene);
    PX_RELEASE(gDispatcher);
    PX_RELEASE(gCooking);
    PX_RELEASE(gPhysics);
    PX_RELEASE(gFoundation);
    gActive = false;
}

bool IsActive() { return gActive && gScene != nullptr; }

void RebuildFromInputGeometry(const InputGeometry& geom)
{
    if (!IsActive()) return;

    ClearRigidStatics();

    PxTriangleMesh* triMesh = nullptr;
    if (CookInputTriangleMesh(geom, triMesh))
        AddTerrainActor(triMesh);

    for (int i = 0; i < static_cast<int>(geom.Obstacles.size()); ++i)
        AddObstacleActor(i, geom.Obstacles[static_cast<size_t>(i)]);
}

bool RaycastClosest(const float origin[3], const float dir[3], float maxDistance,
                    float& outHitX, float& outHitY, float& outHitZ,
                    int& outObstacleIndex)
{
    outObstacleIndex = -1;
    if (!IsActive() || maxDistance <= 0.f) return false;

    PxVec3 o(origin[0], origin[1], origin[2]);
    PxVec3 d(dir[0], dir[1], dir[2]);
    const float len = d.magnitude();
    if (len < 1e-8f) return false;
    d /= len;

    PxRaycastBuffer buf;
    const PxHitFlags flags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;
    if (!gScene->raycast(o, d, maxDistance, buf, flags)) return false;

    const PxRaycastHit& h = buf.block;
    outHitX = h.position.x;
    outHitY = h.position.y;
    outHitZ = h.position.z;
    if (h.actor && h.actor->userData)
        outObstacleIndex = static_cast<int>(reinterpret_cast<uintptr_t>(h.actor->userData)) - 1;
    else
        outObstacleIndex = -1;
    return true;
}

bool RaycastVerticalTopDown(float wx, float wz, const float boundsMinMax[6],
                            float& outHitY, int& outObstacleIndex)
{
    outObstacleIndex = -1;
    if (!IsActive()) return false;

    const float yTop = std::max(boundsMinMax[4], boundsMinMax[1]) + 500.f;
    const float o[3] = { wx, yTop, wz };
    const float d[3] = { 0.f, -1.f, 0.f };
    float       hx, hy, hz;
    if (!RaycastClosest(o, d, 2000.f, hx, hy, hz, outObstacleIndex)) return false;
    outHitY = hy;
    return true;
}

} // namespace PhysWorld

#else // !RECAST_NAVI_USE_PHYSX

namespace PhysWorld
{

bool Init() { return false; }
void Shutdown() {}
bool IsActive() { return false; }

void RebuildFromInputGeometry(const InputGeometry&) {}

bool RaycastClosest(const float*, const float*, float, float&, float&, float&, int&)
{
    return false;
}

bool RaycastVerticalTopDown(float, float, const float*, float&, int&) { return false; }

} // namespace PhysWorld

#endif // RECAST_NAVI_USE_PHYSX
