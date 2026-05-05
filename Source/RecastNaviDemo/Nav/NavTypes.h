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

/// 可编辑障碍（在 XZ 平面有形状，Y 方向从 0 拉伸到 Height）
struct Obstacle
{
    ObstacleShape Shape  = ObstacleShape::Box;
    float         CX     = 0.0f;   ///< 中心 X
    float         CZ     = 0.0f;   ///< 中心 Z
    float         Height = 1.6f;   ///< 顶部 Y（底部 = 0）
    // Box 半边长
    float         SX     = 1.0f;
    float         SZ     = 1.0f;
    // Cylinder 半径
    float         Radius = 1.0f;
};

/// 计算障碍的 AABB（XZ 平面）
inline void GetObstacleAABB(const Obstacle& o, float& minX, float& minZ, float& maxX, float& maxZ)
{
    if (o.Shape == ObstacleShape::Box)
    {
        minX = o.CX - o.SX; maxX = o.CX + o.SX;
        minZ = o.CZ - o.SZ; maxZ = o.CZ + o.SZ;
    }
    else
    {
        minX = o.CX - o.Radius; maxX = o.CX + o.Radius;
        minZ = o.CZ - o.Radius; maxZ = o.CZ + o.Radius;
    }
}

/// 判断点 (x, z) 是否在障碍内部
inline bool PointInsideObstacle(float x, float z, const Obstacle& o)
{
    if (o.Shape == ObstacleShape::Box)
        return std::fabs(x - o.CX) <= o.SX && std::fabs(z - o.CZ) <= o.SZ;
    const float dx = x - o.CX;
    const float dz = z - o.CZ;
    return dx * dx + dz * dz <= o.Radius * o.Radius;
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
