#pragma once
/*
 * Render/CollisionRender.h
 * ------------------------
 * 碰撞可视化辅助：射线与三角/盒/柱求交，用于地面被障碍遮挡剔除。
 */

#include "../Nav/NavTypes.h"
#include "../Shared/MathUtils.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace CollisionRender
{

/// 轴对齐盒：返回射线进入盒的最近参数 t（从 t≥0 段与盒的交）；无命中返回 false
inline bool RayAABBNearEnter(const Vec3& o, const Vec3& d,
                             float minX, float minY, float minZ,
                             float maxX, float maxY, float maxZ,
                             float& outTEnter)
{
    float tMin = 0.0f;
    float tMax = 1e30f;
    const auto slab = [&](float oa, float da, float mn, float mx)
    {
        if (std::fabs(da) < 1e-8f) return !(oa < mn || oa > mx);
        const float invD = 1.0f / da;
        float       t0   = (mn - oa) * invD;
        float       t1   = (mx - oa) * invD;
        if (t0 > t1) std::swap(t0, t1);
        tMin = std::max(tMin, t0);
        tMax = std::min(tMax, t1);
        return tMin <= tMax;
    };
    if (!slab(o.x, d.x, minX, maxX)) return false;
    if (!slab(o.y, d.y, minY, maxY)) return false;
    if (!slab(o.z, d.z, minZ, maxZ)) return false;
    if (tMax < 0.0f) return false;
    outTEnter = tMin >= 0.0f ? tMin : 0.0f;
    return true;
}

/// 竖直圆柱 y∈[y0,y1]，轴过 (cx,cz)
inline bool RayYCylinderNearEnter(const Vec3& o, const Vec3& d,
                                  float cx, float cz, float r, float y0, float y1,
                                  float& outTEnter)
{
    const float ox = o.x - cx;
    const float oz = o.z - cz;
    const float dx = d.x;
    const float dz = d.z;
    const float A = dx * dx + dz * dz;
    if (A < 1e-12f)
    {
        if (ox * ox + oz * oz > r * r * 1.0001f) return false;
        if (o.y < y0 || o.y > y1) return false;
        outTEnter = 0.0f;
        return true;
    }
    const float B = 2.0f * (ox * dx + oz * dz);
    const float C = ox * ox + oz * oz - r * r;
    const float disc = B * B - 4.0f * A * C;
    if (disc < 0.0f) return false;
    const float s  = std::sqrt(disc);
    const float t0 = (-B - s) / (2.0f * A);
    const float t1 = (-B + s) / (2.0f * A);
    float best     = 1e30f;
    for (float t : { t0, t1 })
    {
        if (t < 1e-5f) continue;
        const float y = o.y + t * d.y;
        if (y >= y0 - 1e-4f && y <= y1 + 1e-4f) best = std::min(best, t);
    }
    if (best >= 1e29f) return false;
    outTEnter = best;
    return true;
}

/// 线段 eye→p（世界空间）：若沿视线先进入任一障碍体再到达 p，则 p 被遮挡
inline bool SegmentEyeToPointOccluded(const Vec3& eye, const Vec3& p,
                                      const std::vector<Obstacle>& obstacles)
{
    const Vec3    toP = p - eye;
    const float   tP  = Length(toP);
    if (tP < 1e-5f) return false;
    const Vec3    dir = toP * (1.0f / tP);
    constexpr float eps = 2e-3f;
    for (const Obstacle& o : obstacles)
    {
        float tObs = 0.0f;
        if (o.Shape == ObstacleShape::Box)
        {
            const float minX = o.CX - o.SX;
            const float maxX = o.CX + o.SX;
            const float minZ = o.CZ - o.SZ;
            const float maxZ = o.CZ + o.SZ;
            if (!RayAABBNearEnter(eye, dir, minX, 0.0f, minZ, maxX, o.Height, maxZ, tObs)) continue;
        }
        else
        {
            if (!RayYCylinderNearEnter(eye, dir, o.CX, o.CZ, o.Radius, 0.0f, o.Height, tObs)) continue;
        }
        if (tObs + eps < tP) return true;
    }
    return false;
}

/// 单用重心判断（粗粒度）；精细绘制请用 SegmentEyeToPointOccluded + 三角细分
inline bool GroundTriOccluded(const Vec3& eye, const Vec3& va, const Vec3& vb, const Vec3& vc,
                              const std::vector<Obstacle>& obstacles)
{
    const Vec3 g = (va + vb + vc) * (1.0f / 3.0f);
    return SegmentEyeToPointOccluded(eye, g, obstacles);
}

} // namespace CollisionRender
