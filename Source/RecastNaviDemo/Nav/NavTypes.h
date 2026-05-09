#pragma once
/*
 * Nav/NavTypes.h
 * --------------
 * 导航系统所有数据结构定义。
 *
 * 依赖：
 *   - Recast / Detour SDK（rcPolyMesh, dtNavMesh 等）
 *   - Shared/Profiling.h（CapturedRcContext 继承自 rcContext）
 *
 * 设计原则：
 *   - 本头文件不依赖 UI/ 或 Render/ 的任何内容
 *   - 所有导航相关类型均在此集中定义，其他模块 #include 此头即可
 */

#include <string>
#include <vector>
#include <cmath>
#include <limits>

// Recast & Detour
#include "Recast.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "DetourStatus.h"
#include "DetourTileCache.h"

// =============================================================================
// CapturedRcContext — 抓取 Recast 日志的上下文子类
// =============================================================================
class CapturedRcContext : public rcContext
{
public:
    std::vector<std::string> LogLines;

protected:
    void doLog(const rcLogCategory category, const char* msg, const int len) override
    {
        if (!msg || len <= 0) return;
        const char* tag = (category == RC_LOG_PROGRESS) ? "[INFO ] "
                        : (category == RC_LOG_WARNING)  ? "[WARN ] "
                        : (category == RC_LOG_ERROR)    ? "[ERROR] " : "[????] ";
        std::string line = std::string(tag) + std::string(msg, len);
        LogLines.push_back(std::move(line));
        // 防止日志无限增长
        if (LogLines.size() > 512)
            LogLines.erase(LogLines.begin(), LogLines.begin() + 256);
    }
};

// =============================================================================
// 障碍物类型
// =============================================================================
enum class ObstacleShape : int
{
    Box      = 0,
    Cylinder = 1,
};

/// 可编辑障碍。
/// 形状落在 XZ 平面，Y 方向从 BaseY 拉伸到 (BaseY + Height)。
/// BaseY = 0 时与最初的"始终贴地"行为完全一致；
/// BaseY > 0 表示障碍悬浮在空中（地面下方的导航网格仍可走）。
/// YawDeg 是绕 Y 轴的水平朝向（度，逆时针正向 -- 与 Recast/Detour TileCache 旋转向一致）；
/// 仅对 Box 形状有视觉/几何影响（Cylinder 关于 Y 轴对称，YawDeg 不改变其几何）。
struct Obstacle
{
    ObstacleShape Shape  = ObstacleShape::Box;
    float         CX     = 0.0f;   ///< 中心 X
    float         CZ     = 0.0f;   ///< 中心 Z
    float         BaseY  = 0.0f;   ///< 底部 Y（默认 0，向下兼容"贴地"语义）
    float         Height = 1.6f;   ///< 自身高度（顶部 Y = BaseY + Height）
    float         YawDeg = 0.0f;   ///< 绕 Y 轴的旋转（度）；正值=逆时针俯视
    // Box 半边长
    float         SX     = 1.0f;
    float         SZ     = 1.0f;
    // Cylinder 半径
    float         Radius = 1.0f;
};

/// 把世界坐标 (x, z) 变换到障碍局部坐标系（以 (CX, CZ) 为原点，YawDeg 反向旋转回轴对齐）。
inline void WorldToObstacleLocalXZ(float x, float z, const Obstacle& o,
                                   float& outLX, float& outLZ)
{
    const float dx = x - o.CX;
    const float dz = z - o.CZ;
    const float rad = -o.YawDeg * 3.14159265358979323846f / 180.0f; // 反向旋转
    const float c = std::cos(rad);
    const float s = std::sin(rad);
    outLX = dx * c - dz * s;
    outLZ = dx * s + dz * c;
}

/// 把障碍局部坐标 (lx, lz) 变换回世界坐标。
inline void ObstacleLocalToWorldXZ(float lx, float lz, const Obstacle& o,
                                   float& outX, float& outZ)
{
    const float rad = o.YawDeg * 3.14159265358979323846f / 180.0f;
    const float c = std::cos(rad);
    const float s = std::sin(rad);
    outX = o.CX + lx * c - lz * s;
    outZ = o.CZ + lx * s + lz * c;
}

/// 计算障碍的 AABB（XZ 平面，世界轴对齐）。
/// 旋转的 Box 会膨胀为其 4 个角点的 AABB。
inline void GetObstacleAABB(const Obstacle& o, float& minX, float& minZ, float& maxX, float& maxZ)
{
    if (o.Shape == ObstacleShape::Box)
    {
        const float rad = o.YawDeg * 3.14159265358979323846f / 180.0f;
        const float c   = std::cos(rad);
        const float s   = std::sin(rad);
        const float lx[4] = { -o.SX, +o.SX, +o.SX, -o.SX };
        const float lz[4] = { -o.SZ, -o.SZ, +o.SZ, +o.SZ };
        minX = minZ = std::numeric_limits<float>::infinity();
        maxX = maxZ = -std::numeric_limits<float>::infinity();
        for (int i = 0; i < 4; ++i)
        {
            const float wx = o.CX + lx[i] * c - lz[i] * s;
            const float wz = o.CZ + lx[i] * s + lz[i] * c;
            if (wx < minX) minX = wx;
            if (wx > maxX) maxX = wx;
            if (wz < minZ) minZ = wz;
            if (wz > maxZ) maxZ = wz;
        }
    }
    else
    {
        minX = o.CX - o.Radius; maxX = o.CX + o.Radius;
        minZ = o.CZ - o.Radius; maxZ = o.CZ + o.Radius;
    }
}

/// 判断点 (x, z) 是否在障碍 XZ 投影内部（不考虑 Y）。
/// Box: 先把 (x,z) 反向旋转回障碍局部空间再做轴对齐比较。
inline bool PointInsideObstacle(float x, float z, const Obstacle& o)
{
    if (o.Shape == ObstacleShape::Box)
    {
        float lx, lz;
        WorldToObstacleLocalXZ(x, z, o, lx, lz);
        return std::fabs(lx) <= o.SX && std::fabs(lz) <= o.SZ;
    }
    const float dx = x - o.CX;
    const float dz = z - o.CZ;
    return dx * dx + dz * dz <= o.Radius * o.Radius;
}

/// 判断点 (x, y, z) 是否在障碍体内（含 BaseY..BaseY+Height 区间）
inline bool PointInsideObstacle3D(float x, float y, float z, const Obstacle& o)
{
    if (y < o.BaseY || y > o.BaseY + o.Height) return false;
    return PointInsideObstacle(x, z, o);
}

/// 判断障碍是否"贴地"（底部足够接近 y=0 的地平面）
/// 用于 Procedural 模式：仅贴地障碍才把脚下三角形标记为不可走，
/// 悬浮障碍不应破坏其下方地面 NavMesh。
inline bool ObstacleSitsOnGround(const Obstacle& o, float groundY = 0.0f, float eps = 1e-3f)
{
    return o.BaseY <= groundY + eps;
}

// =============================================================================
// Off-mesh 连接（跳跃/传送点）
// =============================================================================
struct OffMeshLink
{
    float          Start[3]  = {};
    float          End[3]    = {};
    float          Radius    = 0.6f;
    unsigned short Flags     = 0x01;
    unsigned char  Area      = RC_WALKABLE_AREA;
    unsigned char  Dir       = 1;  ///< 1 = 双向, 0 = 单向 (Start→End)
};

// =============================================================================
// 输入几何来源
// =============================================================================
enum class GeomSource { Procedural, ObjFile };

/// 喂给 Recast 的几何数据（顶点、三角形、障碍列表、包围盒）
struct InputGeometry
{
    std::vector<float>         Vertices;      ///< x,y,z 交替存储
    std::vector<int>           Triangles;     ///< 每 3 个索引组成一个三角形
    std::vector<unsigned char> AreaTypes;     ///< 每三角形 1 字节：RC_WALKABLE_AREA / RC_NULL_AREA
    std::vector<Obstacle>      Obstacles;     ///< 仅 Procedural 模式有效
    std::vector<OffMeshLink>   OffMeshLinks;  ///< Off-mesh 连接列表（跨模式均有效）
    float                      Bounds[6];     ///< bmin.xyz, bmax.xyz
    GeomSource                 Source = GeomSource::Procedural;
    std::string                ObjPath;       ///< 仅 ObjFile 模式有效
    /// "纯地面"三角数量，即 Triangles 中前多少个属于地面（不含 Obstacle 实体网格）。
    /// Procedural 模式：在 AppendObstacleSolidMesh 之前置位；
    /// OBJ 模式：等于 Triangles.size()/3。
    /// 渲染时区分地面着色与障碍着色；< 0 视为"全部按地面渲染"（向后兼容默认）。
    int                        GroundTriCount = -1;
};

// =============================================================================
// 场景配置（Procedural 地面参数）
// =============================================================================
struct SceneConfig
{
    float HalfSize      = 15.0f;   ///< 地面以原点为中心，边长 = 2 * HalfSize
    int   GridCells     = 30;      ///< 每边网格数（几何三角形密度）
    bool  bObstacleSolid = true;   ///< 障碍是否追加实体网格（侧面 + 顶面可走）
};

// =============================================================================
// NavMesh 构建参数
// =============================================================================
struct NavBuildConfig
{
    float CellSize             = 0.30f;
    float CellHeight           = 0.20f;
    float AgentHeight          = 2.00f;
    float AgentRadius          = 0.60f;
    float AgentMaxClimb        = 0.90f;
    float AgentMaxSlope        = 45.0f;
    int   RegionMinSize        = 8;
    int   RegionMergeSize      = 20;
    float EdgeMaxLen           = 12.0f;
    float EdgeMaxError         = 1.30f;
    int   VertsPerPoly         = 6;
    float DetailSampleDist     = 6.0f;
    float DetailSampleMaxError = 1.0f;
    bool  bUseTileCache        = false;  ///< 使用 TileCache 动态障碍模式（Tiled NavMesh）
};

// =============================================================================
// 自定义 NavMesh 生成区域（AABB）
// =============================================================================
/// 用户可手动指定一个三维包围盒，限制 NavMesh 仅在此范围内生成。
/// bActive = false 时忽略，使用几何自身包围盒。
struct BuildVolume
{
    bool  bActive = false;
    float Min[3]  = { -15.f, -0.5f, -15.f };
    float Max[3]  = {  15.f,  5.0f,  15.f };
};

// =============================================================================
// 自动 NavLink 生成配置（基于 NavMesh 边界轮廓）
// =============================================================================
struct AutoNavLinkConfig
{
    bool  bEnabled          = false;  ///< 是否启用自动 NavLink 生成
    float JumpUpHeight      = 1.2f;   ///< 双向跳跃高度阈值（|dY| ≤ 此值 → 双向）
    float DropDownMaxHeight = 4.0f;   ///< 单向向下最大高度阈值（dY > JumpUpHeight 且 ≤ 此值 → 单向）
    float EdgeSearchRadius  = 3.0f;   ///< XZ 配对搜索半径（应 > AgentRadius + CellSize，推荐 2-5m）
    float LinkRadius        = 0.6f;   ///< NavLink 端点连接半径（同 OffMeshLink.Radius）
    float EdgeMergeRadius   = 1.0f;   ///< 重复边中点合并阈值
    float MinHeightDiff     = 0.15f;  ///< 最小有效高度差：小于此值的边对不生成（避免平地噪声）
};

// =============================================================================
// NavMesh 运行时（构建结果 + Query 接口）
// =============================================================================
struct NavRuntime
{
    CapturedRcContext          Ctx;
    rcPolyMesh*                PolyMesh       = nullptr;
    rcPolyMeshDetail*          PolyMeshDetail = nullptr;
    dtNavMesh*                 NavMesh        = nullptr;
    dtNavMeshQuery*            NavQuery       = nullptr;
    dtTileCache*               TileCache      = nullptr;  ///< TileCache 模式下有效
    bool                       bBuilt         = false;
    bool                       bTileMode      = false;    ///< true = 使用 TileCache / Tiled NavMesh
    /// 保留一份原始 navData 副本，用于 "Save NavMesh"
    std::vector<unsigned char> NavMeshData;
};
