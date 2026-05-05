/*
 * Nav/NavGeometry.cpp
 * -------------------
 * 输入几何管理实现。
 */

#include "NavGeometry.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>

// Recast
#include "Recast.h"

namespace NavGeometry
{

// =============================================================================
// 内部辅助 — 追加三角形
// =============================================================================
static void PushTri(InputGeometry& geom, int a, int b, int c, unsigned char area)
{
    geom.Triangles.push_back(a);
    geom.Triangles.push_back(b);
    geom.Triangles.push_back(c);
    geom.AreaTypes.push_back(area);
}

static int PushVert(InputGeometry& geom, float x, float y, float z)
{
    const int idx = static_cast<int>(geom.Vertices.size() / 3);
    geom.Vertices.push_back(x);
    geom.Vertices.push_back(y);
    geom.Vertices.push_back(z);
    return idx;
}

// =============================================================================
// 障碍工厂
// =============================================================================

Obstacle MakeBox(float cx, float cz, float sx, float sz, float h)
{
    Obstacle o;
    o.Shape  = ObstacleShape::Box;
    o.CX     = cx;
    o.CZ     = cz;
    o.SX     = sx;
    o.SZ     = sz;
    o.Height = h;
    return o;
}

Obstacle MakeCylinder(float cx, float cz, float r, float h)
{
    Obstacle o;
    o.Shape  = ObstacleShape::Cylinder;
    o.CX     = cx;
    o.CZ     = cz;
    o.Radius = r;
    o.Height = h;
    return o;
}

// =============================================================================
// 障碍约束
// =============================================================================

void ClampObstacleToScene(Obstacle& o, float halfSize)
{
    constexpr float margin = 1.0f;
    const float halfX = (o.Shape == ObstacleShape::Box) ? o.SX : o.Radius;
    const float halfZ = (o.Shape == ObstacleShape::Box) ? o.SZ : o.Radius;
    const float maxAbsX = std::max(0.0f, halfSize - margin - halfX);
    const float maxAbsZ = std::max(0.0f, halfSize - margin - halfZ);
    o.CX = std::max(-maxAbsX, std::min(maxAbsX, o.CX));
    o.CZ = std::max(-maxAbsZ, std::min(maxAbsZ, o.CZ));
}

void ClampAllObstaclesToScene(InputGeometry& geom, float halfSize)
{
    for (Obstacle& o : geom.Obstacles)
        ClampObstacleToScene(o, halfSize);
}

// =============================================================================
// 三角形辅助
// =============================================================================

bool TriCenterInsideAnyObstacle(const float v0[3], const float v1[3], const float v2[3],
                                 const std::vector<Obstacle>& obs)
{
    const float cx = (v0[0] + v1[0] + v2[0]) / 3.0f;
    const float cz = (v0[2] + v1[2] + v2[2]) / 3.0f;
    for (const Obstacle& o : obs)
    {
        if (PointInsideObstacle(cx, cz, o)) return true;
    }
    return false;
}

// =============================================================================
// Procedural 几何
// =============================================================================

void RebuildProceduralGeometry(InputGeometry& geom, const SceneConfig& scene)
{
    // 保留障碍列表，清空其余
    std::vector<Obstacle> savedObs = geom.Obstacles;
    geom         = InputGeometry{};
    geom.Obstacles = std::move(savedObs);
    geom.Source    = GeomSource::Procedural;

    const float HalfSize = std::max(2.0f, scene.HalfSize);
    const int   Cells    = std::max(2,    scene.GridCells);
    const float step     = (HalfSize * 2.0f) / static_cast<float>(Cells);

    // 顶点网格
    geom.Vertices.reserve((Cells + 1) * (Cells + 1) * 3);
    for (int z = 0; z <= Cells; ++z)
    {
        for (int x = 0; x <= Cells; ++x)
        {
            geom.Vertices.push_back(-HalfSize + x * step);
            geom.Vertices.push_back(0.0f);
            geom.Vertices.push_back(-HalfSize + z * step);
        }
    }

    // 三角形 + Area 标记
    geom.Triangles.reserve(Cells * Cells * 6);
    geom.AreaTypes.reserve(Cells * Cells * 2);

    for (int z = 0; z < Cells; ++z)
    {
        for (int x = 0; x < Cells; ++x)
        {
            const int i00 = z * (Cells + 1) + x;
            const int i10 = i00 + 1;
            const int i01 = i00 + (Cells + 1);
            const int i11 = i01 + 1;

            auto AddTri = [&](int a, int b, int c)
            {
                const float* va = &geom.Vertices[a * 3];
                const float* vb = &geom.Vertices[b * 3];
                const float* vc = &geom.Vertices[c * 3];
                const bool blocked = TriCenterInsideAnyObstacle(va, vb, vc, geom.Obstacles);
                geom.Triangles.push_back(a);
                geom.Triangles.push_back(b);
                geom.Triangles.push_back(c);
                geom.AreaTypes.push_back(blocked ? RC_NULL_AREA : RC_WALKABLE_AREA);
            };

            AddTri(i00, i01, i11);
            AddTri(i00, i11, i10);
        }
    }

    rcCalcBounds(geom.Vertices.data(),
                 static_cast<int>(geom.Vertices.size() / 3),
                 &geom.Bounds[0], &geom.Bounds[3]);

    // 记录"纯地面三角数"：障碍实体网格之后追加的不算地面，
    // 让渲染层能用独立颜色区分地面 vs 障碍侧/顶。
    geom.GroundTriCount = static_cast<int>(geom.Triangles.size() / 3);

    // 追加障碍实体网格（侧面 + 顶面，RC_WALKABLE_AREA）
    if (scene.bObstacleSolid && !geom.Obstacles.empty())
        AppendObstacleSolidMesh(geom);
}

// =============================================================================
// 障碍实体网格
// =============================================================================

void AppendObstacleSolidMesh(InputGeometry& geom)
{
    constexpr int   CylSegments = 16;
    constexpr float Pi          = 3.14159265358979323846f;

    for (const Obstacle& o : geom.Obstacles)
    {
        const float y0 = 0.0f;
        const float y1 = o.Height;

        if (o.Shape == ObstacleShape::Box)
        {
            // 8 个角顶点
            const float x0 = o.CX - o.SX, x1 = o.CX + o.SX;
            const float z0 = o.CZ - o.SZ, z1 = o.CZ + o.SZ;

            // 底部四角
            const int b00 = PushVert(geom, x0, y0, z0);
            const int b10 = PushVert(geom, x1, y0, z0);
            const int b11 = PushVert(geom, x1, y0, z1);
            const int b01 = PushVert(geom, x0, y0, z1);
            // 顶部四角
            const int t00 = PushVert(geom, x0, y1, z0);
            const int t10 = PushVert(geom, x1, y1, z0);
            const int t11 = PushVert(geom, x1, y1, z1);
            const int t01 = PushVert(geom, x0, y1, z1);

            const unsigned char W = RC_WALKABLE_AREA;

            // 4 个侧面（每面 2 个三角形，顺时针法线向外）
            // 前面 (z=z0)
            PushTri(geom, b00, t00, t10, W);  PushTri(geom, b00, t10, b10, W);
            // 右面 (x=x1)
            PushTri(geom, b10, t10, t11, W);  PushTri(geom, b10, t11, b11, W);
            // 后面 (z=z1)
            PushTri(geom, b11, t11, t01, W);  PushTri(geom, b11, t01, b01, W);
            // 左面 (x=x0)
            PushTri(geom, b01, t01, t00, W);  PushTri(geom, b01, t00, b00, W);
            // 顶面
            PushTri(geom, t00, t01, t11, W);  PushTri(geom, t00, t11, t10, W);
        }
        else // Cylinder
        {
            // 生成顶/底圆周顶点
            std::vector<int> botIdx(CylSegments), topIdx(CylSegments);
            for (int i = 0; i < CylSegments; ++i)
            {
                const float a = (2.0f * Pi * i) / CylSegments;
                const float cx = o.CX + o.Radius * std::cos(a);
                const float cz = o.CZ + o.Radius * std::sin(a);
                botIdx[i] = PushVert(geom, cx, y0, cz);
                topIdx[i] = PushVert(geom, cx, y1, cz);
            }
            const int centerTop = PushVert(geom, o.CX, y1, o.CZ);

            const unsigned char W = RC_WALKABLE_AREA;
            for (int i = 0; i < CylSegments; ++i)
            {
                const int ni = (i + 1) % CylSegments;
                // 侧面四边形（2 三角形）
                PushTri(geom, botIdx[i], topIdx[i], topIdx[ni], W);
                PushTri(geom, botIdx[i], topIdx[ni], botIdx[ni], W);
                // 顶面扇形（绕序 CCW 俯视 → 法线朝 +Y，Recast 识别为可行走面）
                PushTri(geom, centerTop, topIdx[ni], topIdx[i], W);
            }
        }
    }

    // 重新计算包围盒（实体网格可能超出原始地面范围）
    if (!geom.Vertices.empty())
        rcCalcBounds(geom.Vertices.data(),
                     static_cast<int>(geom.Vertices.size() / 3),
                     &geom.Bounds[0], &geom.Bounds[3]);
}

// =============================================================================
void InitDefaultGeometry(InputGeometry& geom, const SceneConfig& scene)
{
    geom.Obstacles = {
        MakeBox(      -5.0f,  0.0f, 3.0f, 3.0f, 1.8f),
        MakeBox(       6.5f,  6.0f, 3.5f, 3.0f, 2.5f),
        MakeBox(      -1.5f,  9.0f, 2.5f, 3.0f, 1.2f),
        MakeCylinder( -9.0f,  8.0f, 2.0f,       2.2f),
        MakeCylinder(  9.0f, -6.0f, 2.5f,       3.0f),
    };
    RebuildProceduralGeometry(geom, scene);
}

// =============================================================================
// OBJ 加载
// =============================================================================

bool LoadObjGeometry(const char* path, InputGeometry& geom, std::string& errOut)
{
    std::ifstream fs(path);
    if (!fs.is_open()) { errOut = "open failed"; return false; }

    std::vector<float> verts;
    std::vector<int>   tris;
    std::string        line;

    while (std::getline(fs, line))
    {
        if (line.size() < 2) continue;

        if (line[0] == 'v' && line[1] == ' ')
        {
            std::istringstream is(line.substr(2));
            float x, y, z;
            if (is >> x >> y >> z)
            {
                verts.push_back(x);
                verts.push_back(y);
                verts.push_back(z);
            }
        }
        else if (line[0] == 'f' && line[1] == ' ')
        {
            // 收集每个 face vertex 的位置索引
            std::istringstream is(line.substr(2));
            std::string        token;
            std::vector<int>   poly;

            while (is >> token)
            {
                int        vi    = 0;
                const auto slash = token.find('/');
                if (slash != std::string::npos) token = token.substr(0, slash);
                try { vi = std::stoi(token); } catch (...) { vi = 0; }
                if (vi != 0)
                {
                    const int n = static_cast<int>(verts.size() / 3);
                    if (vi < 0) vi = n + vi;  // OBJ 负数索引（从末尾倒数）
                    else        vi = vi - 1;  // OBJ 1-based → 0-based
                    if (vi >= 0 && vi < n) poly.push_back(vi);
                }
            }
            // fan triangulation
            for (size_t i = 1; i + 1 < poly.size(); ++i)
            {
                tris.push_back(poly[0]);
                tris.push_back(poly[i]);
                tris.push_back(poly[i + 1]);
            }
        }
    }

    if (verts.empty() || tris.empty()) { errOut = "no v/f lines"; return false; }

    geom             = InputGeometry{};
    geom.Source      = GeomSource::ObjFile;
    geom.ObjPath     = path;
    geom.Vertices    = std::move(verts);
    geom.Triangles   = std::move(tris);
    geom.AreaTypes.assign(geom.Triangles.size() / 3, RC_WALKABLE_AREA);
    // OBJ 模式没有"障碍实体网格"，所有三角都按地面渲染
    geom.GroundTriCount = static_cast<int>(geom.Triangles.size() / 3);

    rcCalcBounds(geom.Vertices.data(),
                 static_cast<int>(geom.Vertices.size() / 3),
                 &geom.Bounds[0], &geom.Bounds[3]);

    return true;
}

} // namespace NavGeometry
