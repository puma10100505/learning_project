/*
 * Nav/NavPathSmooth.cpp
 * ---------------------
 * 路径平滑后处理算法实现。
 * 详细说明见 NavPathSmooth.h。
 */

#include "NavPathSmooth.h"

#include <algorithm>
#include <cmath>

// Detour
#include "DetourNavMeshQuery.h"
#include "DetourStatus.h"

namespace NavPathSmooth
{

// =============================================================================
// 字符串拉直（String Pull）
// =============================================================================

std::vector<float> StringPull(
    const std::vector<float>&     straight,
    const std::vector<dtPolyRef>& corridor,
    const dtNavMeshQuery*         query,
    const dtQueryFilter*          filter)
{
    // 安全检查
    if (!query || straight.size() < 6 || corridor.empty())
        return straight;

    const size_t nPts = straight.size() / 3;

    // 默认过滤器（filter 为 null 时使用）
    dtQueryFilter defaultFilter;
    if (!filter) filter = &defaultFilter;

    std::vector<float> result;
    result.reserve(straight.size());

    // 始终保留起点
    result.push_back(straight[0]);
    result.push_back(straight[1]);
    result.push_back(straight[2]);

    size_t anchor = 0;   // 当前"确认锚点"在 straight 中的索引

    while (anchor + 1 < nPts)
    {
        // 从 anchor 往后尝试尽可能跳远
        size_t best = anchor + 1;  // 至少前进一步

        const float* aPos = &straight[anchor * 3];

        // 找 anchor 所在的 poly
        // 用 straight 点靠近锚点那一端的 poly 作为起始 poly
        dtPolyRef startPoly = corridor[0];
        if (anchor < corridor.size())
            startPoly = corridor[anchor < corridor.size() ? anchor : corridor.size() - 1];

        for (size_t j = anchor + 2; j < nPts; ++j)
        {
            const float* bPos = &straight[j * 3];

            // raycast：从 aPos 沿直线到 bPos，看是否能无遮挡穿越走廊
            float          t = 0.0f;
            float          hitNormal[3] = {};
            dtPolyRef      path[256];
            int            pathCount = 0;

            dtStatus st = query->raycast(startPoly,
                                         aPos, bPos,
                                         filter,
                                         &t,
                                         hitNormal,
                                         path, &pathCount, 256);

            // t >= 1 表示射线全程无碰撞，可以直达
            if (!dtStatusFailed(st) && t >= 1.0f - 1e-4f)
            {
                best = j;   // 可以跳到 j
            }
            else
            {
                break;  // 遇到阻挡，不再往后尝试
            }
        }

        // 将 best 点加入结果
        result.push_back(straight[best * 3 + 0]);
        result.push_back(straight[best * 3 + 1]);
        result.push_back(straight[best * 3 + 2]);
        anchor = best;
    }

    return result;
}

// =============================================================================
// Chaikin 切角细分（均匀 B 样条近似）
// =============================================================================

std::vector<float> BezierSubdivide(
    const std::vector<float>& pts,
    int subdivisions)
{
    if (pts.size() < 6) return pts;

    subdivisions = std::max(1, std::min(subdivisions, 6));

    std::vector<float> cur = pts;

    for (int iter = 0; iter < subdivisions; ++iter)
    {
        const size_t n = cur.size() / 3;
        if (n < 2) break;

        // Chaikin 算法：对每条边生成 Q = 3/4*P0 + 1/4*P1 和 R = 1/4*P0 + 3/4*P1 两点
        // 首尾端点保持不变（open-curve 版本）
        std::vector<float> next;
        next.reserve(((n - 1) * 2 + 2) * 3);

        // 保留起点
        next.push_back(cur[0]);
        next.push_back(cur[1]);
        next.push_back(cur[2]);

        for (size_t i = 0; i + 1 < n; ++i)
        {
            const float x0 = cur[i * 3 + 0], y0 = cur[i * 3 + 1], z0 = cur[i * 3 + 2];
            const float x1 = cur[(i + 1) * 3 + 0];
            const float y1 = cur[(i + 1) * 3 + 1];
            const float z1 = cur[(i + 1) * 3 + 2];

            // Q = 3/4*P0 + 1/4*P1  (如果不是起点才插入，避免与起点重复)
            if (i > 0)
            {
                next.push_back(0.75f * x0 + 0.25f * x1);
                next.push_back(0.75f * y0 + 0.25f * y1);
                next.push_back(0.75f * z0 + 0.25f * z1);
            }

            // R = 1/4*P0 + 3/4*P1  (如果不是最后一段才插入，避免与终点重复)
            if (i + 2 < n)
            {
                next.push_back(0.25f * x0 + 0.75f * x1);
                next.push_back(0.25f * y0 + 0.75f * y1);
                next.push_back(0.25f * z0 + 0.75f * z1);
            }
        }

        // 保留终点
        next.push_back(cur[(n - 1) * 3 + 0]);
        next.push_back(cur[(n - 1) * 3 + 1]);
        next.push_back(cur[(n - 1) * 3 + 2]);

        cur = std::move(next);
    }

    return cur;
}

} // namespace NavPathSmooth
