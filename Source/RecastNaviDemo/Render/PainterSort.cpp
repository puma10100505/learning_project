/*
 * Render/PainterSort.cpp
 */

#include "PainterSort.h"

#include "Primitives.h"

#include <algorithm>
#include <vector>

namespace PainterSort
{
namespace
{
struct TriRec
{
    Vec3    a, b, c;
    ImU32   col{};
    float   key{}; ///< 距相机越远越大，先画
};

struct LineRec
{
    Vec3    a, b;
    ImU32   col{};
    float   thickness{};
};

bool               g_collect = false;
Mat4               g_vp{};
ImVec2             g_vmin{};
ImVec2             g_vsize{};
Vec3               g_eye{};
std::vector<TriRec> g_tris;
std::vector<LineRec> g_lines;
} // namespace

bool IsCollecting() { return g_collect; }

void Begin(const Vec3& eyeWorld, const Mat4& vp, ImVec2 vMin, ImVec2 vSize)
{
    g_collect = true;
    g_eye     = eyeWorld;
    g_vp      = vp;
    g_vmin    = vMin;
    g_vsize   = vSize;
    g_tris.clear();
    g_lines.clear();
}

void RecordTri(const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
               Vec3 a, Vec3 b, Vec3 c, ImU32 col)
{
    (void)vp;
    (void)vMin;
    (void)vSize;
    const Vec3 ctr = (a + b + c) * (1.0f / 3.0f);
    const float key = Length(ctr - g_eye);
    g_tris.push_back(TriRec{ a, b, c, col, key });
}

void RecordLine(const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
                Vec3 a, Vec3 b, ImU32 col, float thickness)
{
    (void)vp;
    (void)vMin;
    (void)vSize;
    g_lines.push_back(LineRec{ a, b, col, thickness });
}

void Flush(ImDrawList* dl)
{
    if (!g_collect)
        return;
    g_collect = false;

    std::sort(g_tris.begin(), g_tris.end(),
              [](const TriRec& A, const TriRec& B) { return A.key > B.key; });

    for (const TriRec& t : g_tris)
        Render3D::TriFilled3D_DrawImmediate(dl, g_vp, g_vmin, g_vsize, t.a, t.b, t.c, t.col);

    for (const LineRec& L : g_lines)
        Render3D::Line3D_DrawImmediate(dl, g_vp, g_vmin, g_vsize, L.a, L.b, L.col, L.thickness);

    g_tris.clear();
    g_lines.clear();
}

} // namespace PainterSort
