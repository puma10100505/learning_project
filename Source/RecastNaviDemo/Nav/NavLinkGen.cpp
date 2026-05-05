/*
 * Nav/NavLinkGen.cpp
 * ------------------
 * 基于 NavMesh 边界轮廓自动生成 Off-mesh NavLink 实现。
 *
 * 算法说明：
 *   1. 遍历所有 tile/poly/edge，收集 neis[j]==0 的外边界边中点
 *   2. O(N²) 对搜索：XZ 距离 < EdgeSearchRadius 的边对进入候选
 *   3. 方向过滤：两条边方向点积 <= 0.5（面对面 or 垂直）
 *   4. 高度差 adY 决定类型：
 *        adY < MinHeightDiff          → 跳过（平地噪声）
 *        MinHeightDiff ≤ adY ≤ JumpUp → 双向 NavLink
 *        JumpUp < adY ≤ DropDownMax   → 单向向下 NavLink
 *   5. EdgeMergeRadius 去重
 */

#include "NavLinkGen.h"
#include "DetourNavMesh.h"
#include "DetourStatus.h"

#include <cmath>
#include <vector>

namespace NavLinkGen
{

// =============================================================================
// 内部辅助
// =============================================================================

static inline void Midpoint(const float* a, const float* b, float out[3])
{
    out[0] = (a[0] + b[0]) * 0.5f;
    out[1] = (a[1] + b[1]) * 0.5f;
    out[2] = (a[2] + b[2]) * 0.5f;
}

static inline float Dist2XZ(const float a[3], const float b[3])
{
    const float dx = a[0] - b[0];
    const float dz = a[2] - b[2];
    return dx * dx + dz * dz;
}

// =============================================================================
// 边界边中点收集
// =============================================================================
struct EdgeMidpt
{
    float pos[3];   // 世界坐标中点
    float dir[2];   // 边方向（归一化 XZ）
};

static std::vector<EdgeMidpt> CollectBoundaryEdgeMidpoints(const dtNavMesh* nm)
{
    std::vector<EdgeMidpt> result;
    if (!nm) return result;

    const int maxTiles = nm->getMaxTiles();
    for (int ti = 0; ti < maxTiles; ++ti)
    {
        const dtMeshTile* tile = nm->getTile(ti);
        if (!tile || !tile->header || tile->header->polyCount == 0) continue;

        for (int pi = 0; pi < tile->header->polyCount; ++pi)
        {
            const dtPoly* poly = &tile->polys[pi];
            if (poly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION) continue;

            const int nv = poly->vertCount;
            for (int j = 0; j < nv; ++j)
            {
                // neis[j] == 0 表示外边界（无相邻多边形）
                if (poly->neis[j] != 0) continue;

                const int ja = j;
                const int jb = (j + 1) % nv;

                const float* va = &tile->verts[poly->verts[ja] * 3];
                const float* vb = &tile->verts[poly->verts[jb] * 3];

                // 过滤掉极短的退化边
                const float edx = vb[0] - va[0];
                const float edz = vb[2] - va[2];
                const float elen2 = edx * edx + edz * edz;
                if (elen2 < 1e-8f) continue;

                EdgeMidpt em;
                Midpoint(va, vb, em.pos);

                const float elen = std::sqrt(elen2);
                em.dir[0] = edx / elen;
                em.dir[1] = edz / elen;

                result.push_back(em);
            }
        }
    }
    return result;
}

// =============================================================================
// 主生成函数
// =============================================================================
std::vector<OffMeshLink> GenerateEdgeNavLinks(
    const NavRuntime&        runtime,
    const AutoNavLinkConfig& cfg)
{
    std::vector<OffMeshLink> links;

    if (!cfg.bEnabled)                    return links;
    if (!runtime.bBuilt || !runtime.NavMesh) return links;

    const std::vector<EdgeMidpt> midpts = CollectBoundaryEdgeMidpoints(runtime.NavMesh);
    const int N = static_cast<int>(midpts.size());
    if (N == 0) return links;

    // 使用独立的搜索半径（大），NavLink 端点用 LinkRadius（小）
    const float searchR2    = cfg.EdgeSearchRadius  * cfg.EdgeSearchRadius;
    const float mergeR2     = cfg.EdgeMergeRadius   * cfg.EdgeMergeRadius;
    const float jumpUp      = cfg.JumpUpHeight;
    const float dropDownMax = cfg.DropDownMaxHeight;
    const float minDY       = cfg.MinHeightDiff;

    for (int i = 0; i < N; ++i)
    {
        const float* A = midpts[i].pos;

        for (int j = i + 1; j < N; ++j)
        {
            const float* B = midpts[j].pos;

            // 1. XZ 距离过滤
            if (Dist2XZ(A, B) > searchR2) continue;

            // 2. 方向过滤：两条边方向点积 > 0.5 → 同向（相同侧的平行边，跳过）
            const float dotDir = midpts[i].dir[0] * midpts[j].dir[0]
                               + midpts[i].dir[1] * midpts[j].dir[1];
            if (dotDir > 0.5f) continue;

            // 3. 高度差
            const float dY  = B[1] - A[1];
            const float adY = dY < 0.0f ? -dY : dY;

            // 平地噪声过滤：高度差太小（可配置）
            if (adY < minDY) continue;

            // 4. 按高度差分类
            if (adY <= jumpUp)
            {
                // 双向 NavLink
                OffMeshLink lk{};
                lk.Start[0] = A[0]; lk.Start[1] = A[1]; lk.Start[2] = A[2];
                lk.End  [0] = B[0]; lk.End  [1] = B[1]; lk.End  [2] = B[2];
                lk.Radius   = cfg.LinkRadius;
                lk.Flags    = 0x01;
                lk.Area     = RC_WALKABLE_AREA;
                lk.Dir      = 1;  // 双向

                bool dup = false;
                for (const auto& ex : links)
                {
                    if ((Dist2XZ(ex.Start, lk.Start) < mergeR2 && Dist2XZ(ex.End, lk.End) < mergeR2) ||
                        (Dist2XZ(ex.Start, lk.End  ) < mergeR2 && Dist2XZ(ex.End, lk.Start) < mergeR2))
                    { dup = true; break; }
                }
                if (!dup) links.push_back(lk);
            }
            else if (adY <= dropDownMax)
            {
                // 单向向下：高点 Start → 低点 End
                const float* hi = (A[1] >= B[1]) ? A : B;
                const float* lo = (A[1] >= B[1]) ? B : A;

                OffMeshLink lk{};
                lk.Start[0] = hi[0]; lk.Start[1] = hi[1]; lk.Start[2] = hi[2];
                lk.End  [0] = lo[0]; lk.End  [1] = lo[1]; lk.End  [2] = lo[2];
                lk.Radius   = cfg.LinkRadius;
                lk.Flags    = 0x01;
                lk.Area     = RC_WALKABLE_AREA;
                lk.Dir      = 0;  // 单向 Start→End（高→低）

                bool dup = false;
                for (const auto& ex : links)
                {
                    if (Dist2XZ(ex.Start, lk.Start) < mergeR2 && Dist2XZ(ex.End, lk.End) < mergeR2)
                    { dup = true; break; }
                }
                if (!dup) links.push_back(lk);
            }
        }
    }

    return links;
}

} // namespace NavLinkGen
