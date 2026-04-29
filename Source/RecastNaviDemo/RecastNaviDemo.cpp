/*
 * RecastNaviDemo
 * --------------
 * RecastNavigation 学习 Demo (扩展版):
 *  - 使用 LearningFoundation 的 DirectX12 + ImGui 集成窗口呈现
 *  - 左侧: View / Edit Mode / Build Config / Path Query / Import & Export / Stats & Log
 *  - 右侧: 导航演示画布, 支持 2D 俯视图与 3D 轨道视角(软件投影到 ImDrawList,
 *          仍由 D3D12 后端真正提交渲染)
 *
 * 已实现:
 *  - 障碍编辑(Create/Move/Delete) - 鼠标在画布上交互
 *  - 交互式起终点 - PlaceStart / PlaceEnd 模式点击放置, 可选自动重新寻路
 *  - 离线 IO - OBJ 几何导入, dtNavMesh 二进制保存/加载
 *  - 可视化 - 多边形填充 / Region 色 / DetailMesh / 路径走廊(corridor)高亮
 *  - 性能 - 各 Recast/Detour 阶段耗时面板
 *  - 调试 - rcContext 日志重定向到 ImGui 面板
 *
 * 工程约定: 入口文件名与目录名一致, 依赖通过 SharedDef.cmake 宏组装。
 */

#ifdef __linux__
#include <unistd.h>
#endif

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#pragma comment(lib, "comdlg32.lib")
#endif

#include <algorithm>
#include <array>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "CommonDefines.h"
#include "DirectX12Window.h"
#include "loguru.hpp"

// Recast & Detour
#include "Recast.h"
#include "DetourCommon.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "DetourNavMeshQuery.h"
#include "DetourStatus.h"

namespace
{
    // =================================================================================
    // 1. 轻量数学 (无 glm 依赖)
    // =================================================================================
    struct Vec3 { float x, y, z; };

    inline Vec3  V3(float x, float y, float z)             { return { x, y, z }; }
    inline Vec3  operator+(Vec3 a, Vec3 b)                  { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
    inline Vec3  operator-(Vec3 a, Vec3 b)                  { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
    inline Vec3  operator*(Vec3 a, float s)                 { return { a.x * s, a.y * s, a.z * s }; }
    inline float Dot(Vec3 a, Vec3 b)                        { return a.x * b.x + a.y * b.y + a.z * b.z; }
    inline Vec3  Cross(Vec3 a, Vec3 b)                      { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
    inline float Length(Vec3 a)                             { return std::sqrt(Dot(a, a)); }
    inline Vec3  Normalize(Vec3 a) { float l = Length(a); return l > 1e-6f ? a * (1.0f / l) : V3(0, 0, 0); }

    // 4x4 矩阵, 行主存储 (m[row][col])
    struct Mat4 { float m[4][4]; };

    inline Mat4 MakeIdentity()
    {
        Mat4 r{};
        for (int i = 0; i < 4; ++i) r.m[i][i] = 1.0f;
        return r;
    }

    inline Mat4 MakeLookAtRH(Vec3 eye, Vec3 target, Vec3 up)
    {
        Vec3 f = Normalize(target - eye);
        Vec3 s = Normalize(Cross(f, up));
        Vec3 u = Cross(s, f);
        Mat4 r{};
        r.m[0][0] =  s.x; r.m[0][1] =  s.y; r.m[0][2] =  s.z; r.m[0][3] = -Dot(s, eye);
        r.m[1][0] =  u.x; r.m[1][1] =  u.y; r.m[1][2] =  u.z; r.m[1][3] = -Dot(u, eye);
        r.m[2][0] = -f.x; r.m[2][1] = -f.y; r.m[2][2] = -f.z; r.m[2][3] =  Dot(f, eye);
        r.m[3][3] =  1.0f;
        return r;
    }

    // RH, NDC z in [-1,1]
    inline Mat4 MakePerspectiveRH(float fovyRad, float aspect, float znear, float zfar)
    {
        Mat4 r{};
        const float t  = std::tan(fovyRad * 0.5f);
        const float zn = znear;
        const float zf = zfar;
        r.m[0][0] = 1.0f / (aspect * t);
        r.m[1][1] = 1.0f / t;
        r.m[2][2] = -(zf + zn) / (zf - zn);
        r.m[2][3] = -(2.0f * zf * zn) / (zf - zn);
        r.m[3][2] = -1.0f;
        return r;
    }

    inline Mat4 MatMul(const Mat4& a, const Mat4& b)
    {
        Mat4 r{};
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                r.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j] + a.m[i][3] * b.m[3][j];
        return r;
    }

    // 把 (x,y,z,1) 投到 clip 空间
    inline void TransformPoint(const Mat4& m, Vec3 p, float out[4])
    {
        out[0] = m.m[0][0] * p.x + m.m[0][1] * p.y + m.m[0][2] * p.z + m.m[0][3];
        out[1] = m.m[1][0] * p.x + m.m[1][1] * p.y + m.m[1][2] * p.z + m.m[1][3];
        out[2] = m.m[2][0] * p.x + m.m[2][1] * p.y + m.m[2][2] * p.z + m.m[2][3];
        out[3] = m.m[3][0] * p.x + m.m[3][1] * p.y + m.m[3][2] * p.z + m.m[3][3];
    }

    // =================================================================================
    // 2. rcContext 子类 - 抓 log + 阶段计时
    // =================================================================================
    struct PhaseTimings
    {
        double TotalMs        = 0.0;
        double RasterizeMs    = 0.0;
        double FilterMs       = 0.0;
        double CompactMs      = 0.0;
        double ErodeMs        = 0.0;
        double DistFieldMs    = 0.0;
        double RegionsMs      = 0.0;
        double ContoursMs     = 0.0;
        double PolyMeshMs     = 0.0;
        double DetailMeshMs   = 0.0;
        double DetourCreateMs = 0.0;
    };

    // 单次构建的快照: 时间 + 规模, 用于趋势图
    struct BuildStat
    {
        int          Index        = 0;     // 第几次构建 (1-based)
        PhaseTimings T            {};
        int          InputVerts   = 0;
        int          InputTris    = 0;
        int          PolyCount    = 0;
        int          PolyVerts    = 0;
        int          DetailTris   = 0;
        int          NavDataBytes = 0;
        bool         bOk          = false;
    };

    // 寻路阶段耗时
    struct PathTimings
    {
        double TotalMs        = 0.0;
        double NearestStartMs = 0.0;
        double NearestEndMs   = 0.0;
        double FindPathMs     = 0.0;
        double StraightPathMs = 0.0;
    };

    // 单次寻路的快照
    struct PathStat
    {
        int         Index           = 0;
        PathTimings T               {};
        int         CorridorPolys   = 0;
        int         StraightCorners = 0;
        bool        bOk             = false;
        std::string Status;
    };

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
            // 简单上限
            if (LogLines.size() > 512)
                LogLines.erase(LogLines.begin(), LogLines.begin() + 256);
        }
    };

    // 范围耗时辅助
    struct ScopedTimer
    {
        std::chrono::high_resolution_clock::time_point t0;
        double*                                        out;
        explicit ScopedTimer(double* p) : t0(std::chrono::high_resolution_clock::now()), out(p) {}
        ~ScopedTimer()
        {
            const auto t1 = std::chrono::high_resolution_clock::now();
            *out = std::chrono::duration<double, std::milli>(t1 - t0).count();
        }
    };

    // =================================================================================
    // 3. 输入几何 / 配置 / 运行时
    // =================================================================================
    enum class ObstacleShape : int
    {
        Box      = 0,
        Cylinder = 1,
    };

    // 障碍物 (可编辑): 在 XZ 平面有形状, 在 Y 方向从 0 拉伸到 Height
    struct Obstacle
    {
        ObstacleShape Shape  = ObstacleShape::Box;
        float         CX     = 0.0f;   // 中心 (XZ)
        float         CZ     = 0.0f;
        float         Height = 1.6f;   // 顶部 Y, 底部 = 0
        // Box 半边长
        float         SX     = 1.0f;
        float         SZ     = 1.0f;
        // Cylinder 半径
        float         Radius = 1.0f;
    };

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

    inline bool PointInsideObstacle(float x, float z, const Obstacle& o)
    {
        if (o.Shape == ObstacleShape::Box)
            return std::fabs(x - o.CX) <= o.SX && std::fabs(z - o.CZ) <= o.SZ;
        const float dx = x - o.CX;
        const float dz = z - o.CZ;
        return dx * dx + dz * dz <= o.Radius * o.Radius;
    }

    enum class GeomSource { Procedural, ObjFile };

    struct InputGeometry
    {
        std::vector<float>         Vertices;   // x,y,z
        std::vector<int>           Triangles;  // 3i 一个三角形
        std::vector<unsigned char> AreaTypes;  // 1 字节 / 三角形
        std::vector<Obstacle>      Obstacles;  // 仅 Procedural 模式有效
        float                      Bounds[6];  // bmin.xyz, bmax.xyz
        GeomSource                 Source = GeomSource::Procedural;
        std::string                ObjPath;    // 仅 ObjFile 模式有效
    };

    // 场景尺寸 (procedural 地面所占的世界范围 + 网格密度)
    struct SceneConfig
    {
        float HalfSize  = 15.0f;   // 地面以原点为中心, 边长 = 2 * HalfSize
        int   GridCells = 30;      // 每边网格数 (geometry 三角形密度)
    };

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
    };

    struct NavRuntime
    {
        CapturedRcContext          Ctx;
        rcPolyMesh*                PolyMesh       = nullptr;
        rcPolyMeshDetail*          PolyMeshDetail = nullptr;
        dtNavMesh*                 NavMesh        = nullptr;
        dtNavMeshQuery*            NavQuery       = nullptr;
        bool                       bBuilt         = false;
        // 保留一份原始 navData 副本, 用于 "Save NavMesh"
        std::vector<unsigned char> NavMeshData;
    };

    enum class ViewMode { TopDown2D, Orbit3D };

    enum class EditMode
    {
        None,
        PlaceStart,
        PlaceEnd,
        CreateBox,
        MoveBox,
        DeleteBox,
    };

    struct ViewSettings
    {
        bool bShowGrid          = true;
        bool bShowObstacles     = true;
        bool bShowInputMesh     = true;   // 输入几何 (procedural 地面 / OBJ 模型) 线框
        bool bFillPolygons      = true;
        bool bRegionColors      = true;
        bool bShowDetailMesh    = false;
        bool bShowPolyEdges     = true;
        bool bShowPathCorridor  = true;
        bool bAutoReplan        = true;
        bool bAutoRebuild       = false;
    };

    struct OrbitCamera
    {
        Vec3  Target   = { 0.0f, 0.0f, 0.0f };
        float Yaw      = 0.7f;
        float Pitch    = 0.9f;
        float Distance = 36.0f;
        float Fovy     = 60.0f * 3.1415926f / 180.0f;
    };

    struct CreateDraftBox
    {
        bool   bActive = false;
        float  StartX  = 0.0f;
        float  StartZ  = 0.0f;
        float  CurX    = 0.0f;
        float  CurZ    = 0.0f;
    };

    struct MoveBoxState
    {
        int   Index    = -1;   // 选中的 obstacle 索引
        float OffsetX  = 0.0f; // 起点相对其中心 CX 的偏移
        float OffsetZ  = 0.0f;
    };

    // ---------- 全局状态 ----------
    InputGeometry  g_Geom;
    SceneConfig    g_Scene;
    NavBuildConfig g_Config;
    NavRuntime     g_Nav;
    PhaseTimings   g_Timings;
    ViewSettings   g_View;
    OrbitCamera    g_Cam;
    ViewMode       g_ViewMode = ViewMode::TopDown2D;
    EditMode       g_EditMode = EditMode::None;

    // 构建性能历史 (用于浮动趋势图)
    constexpr int          kBuildHistoryMax = 128;
    std::deque<BuildStat>  g_BuildHistory;
    int                    g_BuildCounter   = 0;
    // 寻路性能历史
    constexpr int          kPathHistoryMax  = 128;
    std::deque<PathStat>   g_PathHistory;
    int                    g_PathCounter    = 0;
    bool                   g_ShowStatsWindow = true;
    float                  g_StatsDockHeight = 280.0f; // 贴底高度 (鼠标可拖)
    float                  g_LeftPaneWidth   = 400.0f; // 主窗口左侧配置区宽度 (鼠标可拖)
    float                  g_StatsLegendWidth = 200.0f; // 性能图右侧图例宽度 (鼠标可拖)

    // 新建障碍时使用的形状/高度
    ObstacleShape  g_DefaultObstacleShape  = ObstacleShape::Box;
    float          g_DefaultObstacleHeight = 1.6f;
    // 路径主色 / 走廊高亮色 (独立可配)
    ImVec4         g_PathColor             = ImVec4(1.00f, 0.86f, 0.24f, 1.00f);
    ImVec4         g_CorridorColor         = ImVec4(0.30f, 0.85f, 1.00f, 0.35f);
    // 当前选中的障碍 (跨编辑模式持久, -1 表示无)
    int            g_SelectedObstacle = -1;

    float                  g_Start[3] = { -10.0f, 0.0f, -10.0f };
    float                  g_End[3]   = {  12.0f, 0.0f,  12.0f };
    std::vector<float>     g_StraightPath;          // x,y,z 三元组
    std::vector<dtPolyRef> g_PathCorridor;          // findPath 返回的 polygon 走廊
    int                    g_PathPolyCount = 0;
    std::string            g_PathStatus    = "(none)";
    std::string            g_BuildStatus   = "(not built)";

    bool                   g_bGeomDirty = false;     // 几何改动 -> 需要 rebuild navmesh

    CreateDraftBox         g_CreateDraft;
    MoveBoxState           g_MoveState;

    // 当前画布最近一次的世界->像素映射, 供 mouse 在 2D 模式下转换世界坐标用
    struct Map2D
    {
        ImVec2 CanvasMin{};
        ImVec2 CanvasSize{};
        float  Bmin[3]{};
        float  Bmax[3]{};
        bool   bValid = false;
    };
    Map2D g_LastMap2D;

    // 3D 投影上一帧的视口/矩阵, 供拣取使用
    struct Map3D
    {
        ImVec2 ViewportMin{};
        ImVec2 ViewportSize{};
        Vec3   EyeWorld{};
        Mat4   InvVP{}; // 这里只记 V 和 P, 实际拣取用 eye + 反向射线即可
        Mat4   View{};
        Mat4   Proj{};
        bool   bValid = false;
    };
    Map3D g_LastMap3D;

    // 文件路径 (UI 用)
    char g_ObjPathInput[256]      = "";
    char g_NavSavePathInput[256]  = "navmesh.bin";
    char g_NavLoadPathInput[256]  = "navmesh.bin";

    // =================================================================================
    // 4. 颜色辅助
    // =================================================================================
    inline ImU32 HashColor(unsigned int id, float alpha = 0.55f)
    {
        // 简单哈希得到稳定颜色
        unsigned int h = id * 2654435761u;
        const float r = ((h >> 16) & 0xFF) / 255.0f;
        const float g = ((h >> 8)  & 0xFF) / 255.0f;
        const float b = ((h)       & 0xFF) / 255.0f;
        const float boost = 0.35f;
        return IM_COL32(
            (int)((r * (1.0f - boost) + boost) * 255),
            (int)((g * (1.0f - boost) + boost) * 255),
            (int)((b * (1.0f - boost) + boost) * 255),
            (int)(alpha * 255));
    }

    // =================================================================================
    // 5. 输入几何构建 / OBJ 加载
    // =================================================================================
    inline ImU32 ColU32(const ImVec4& c, float alphaOverride = -1.0f)
    {
        const float a = alphaOverride >= 0.0f ? alphaOverride : c.w;
        return IM_COL32(static_cast<int>(c.x * 255),
                        static_cast<int>(c.y * 255),
                        static_cast<int>(c.z * 255),
                        static_cast<int>(a   * 255));
    }

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

    // 把单个障碍的中心 clamp 到指定 halfSize 范围内 (考虑 box/cylinder 自身半尺寸 + 1 单位边距)
    inline void ClampObstacleToScene(Obstacle& o, float halfSize)
    {
        constexpr float margin = 1.0f;
        const float halfX = (o.Shape == ObstacleShape::Box) ? o.SX : o.Radius;
        const float halfZ = (o.Shape == ObstacleShape::Box) ? o.SZ : o.Radius;
        const float maxAbsX = std::max(0.0f, halfSize - margin - halfX);
        const float maxAbsZ = std::max(0.0f, halfSize - margin - halfZ);
        o.CX = std::max(-maxAbsX, std::min(maxAbsX, o.CX));
        o.CZ = std::max(-maxAbsZ, std::min(maxAbsZ, o.CZ));
    }
    inline void ClampAllObstaclesToScene(float halfSize)
    {
        for (Obstacle& o : g_Geom.Obstacles)
            ClampObstacleToScene(o, halfSize);
    }

    // 重新生成 Procedural 模式的输入几何 (保留当前 Obstacles 列表)
    void RebuildProceduralInputGeometry()
    {
        std::vector<Obstacle> savedObs = g_Geom.Obstacles;
        g_Geom = InputGeometry{};
        g_Geom.Obstacles = std::move(savedObs);
        g_Geom.Source    = GeomSource::Procedural;

        const float HalfSize = std::max(2.0f, g_Scene.HalfSize);
        const int   Cells    = std::max(2,    g_Scene.GridCells);
        const float step     = (HalfSize * 2.0f) / Cells;

        g_Geom.Vertices.reserve((Cells + 1) * (Cells + 1) * 3);
        for (int z = 0; z <= Cells; ++z)
        {
            for (int x = 0; x <= Cells; ++x)
            {
                g_Geom.Vertices.push_back(-HalfSize + x * step);
                g_Geom.Vertices.push_back(0.0f);
                g_Geom.Vertices.push_back(-HalfSize + z * step);
            }
        }

        g_Geom.Triangles.reserve(Cells * Cells * 6);
        g_Geom.AreaTypes.reserve(Cells * Cells * 2);
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
                    const float* va = &g_Geom.Vertices[a * 3];
                    const float* vb = &g_Geom.Vertices[b * 3];
                    const float* vc = &g_Geom.Vertices[c * 3];
                    const bool blocked = TriCenterInsideAnyObstacle(va, vb, vc, g_Geom.Obstacles);
                    g_Geom.Triangles.push_back(a);
                    g_Geom.Triangles.push_back(b);
                    g_Geom.Triangles.push_back(c);
                    g_Geom.AreaTypes.push_back(blocked ? RC_NULL_AREA : RC_WALKABLE_AREA);
                };
                AddTri(i00, i01, i11);
                AddTri(i00, i11, i10);
            }
        }

        rcCalcBounds(g_Geom.Vertices.data(),
                     static_cast<int>(g_Geom.Vertices.size() / 3),
                     &g_Geom.Bounds[0], &g_Geom.Bounds[3]);
    }

    inline Obstacle MakeBox(float cx, float cz, float sx, float sz, float h = 1.6f)
    {
        Obstacle o;
        o.Shape = ObstacleShape::Box;
        o.CX = cx; o.CZ = cz;
        o.SX = sx; o.SZ = sz;
        o.Height = h;
        return o;
    }
    inline Obstacle MakeCylinder(float cx, float cz, float r, float h = 1.6f)
    {
        Obstacle o;
        o.Shape = ObstacleShape::Cylinder;
        o.CX = cx; o.CZ = cz;
        o.Radius = r;
        o.Height = h;
        return o;
    }

    // 初次构建: 给若干默认障碍 (box + cylinder 混合, 高度各异)
    void InitDefaultGeometry()
    {
        g_Geom.Obstacles = {
            MakeBox(      -5.0f,  0.0f, 3.0f, 3.0f, 1.8f),
            MakeBox(       6.5f,  6.0f, 3.5f, 3.0f, 2.5f),
            MakeBox(      -1.5f,  9.0f, 2.5f, 3.0f, 1.2f),
            MakeCylinder( -9.0f,  8.0f, 2.0f,        2.2f),
            MakeCylinder(  9.0f, -6.0f, 2.5f,        3.0f),
        };
        RebuildProceduralInputGeometry();
        g_bGeomDirty = true;
    }

    // 根据当前 g_Geom.Bounds 把相机重新对位 + 估算 distance, 让模型整体在画布内
    void FrameCameraToBounds()
    {
        const float* bmin = g_Geom.Bounds + 0;
        const float* bmax = g_Geom.Bounds + 3;
        const float dx = bmax[0] - bmin[0];
        const float dy = bmax[1] - bmin[1];
        const float dz = bmax[2] - bmin[2];
        if (dx <= 0.0f || dz <= 0.0f) return;

        g_Cam.Target = V3((bmin[0] + bmax[0]) * 0.5f,
                          (bmin[1] + bmax[1]) * 0.5f,
                          (bmin[2] + bmax[2]) * 0.5f);

        // 视角张角换算: 取斜对角线 / (2*tan(fovy/2)) * 余量
        const float diag = std::sqrt(dx * dx + dy * dy + dz * dz);
        const float fitH = (diag * 0.5f) / std::tan(g_Cam.Fovy * 0.5f);
        const float fitD = std::max(diag * 0.6f, fitH * 1.15f);
        g_Cam.Distance = std::max(2.0f, std::min(800.0f, fitD));

        g_Cam.Yaw   = 0.7f;
        g_Cam.Pitch = 0.9f;
    }

    // 简单 OBJ 加载: 仅解析 v / f, f 行支持 "a", "a/b", "a/b/c", "a//c" 形式
    bool LoadObjGeometry(const char* path, std::string& errOut)
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
                    int        vi = 0;
                    const auto slash = token.find('/');
                    if (slash != std::string::npos) token = token.substr(0, slash);
                    try { vi = std::stoi(token); } catch (...) { vi = 0; }
                    if (vi != 0)
                    {
                        const int n = static_cast<int>(verts.size() / 3);
                        if (vi < 0) vi = n + vi; else vi = vi - 1; // OBJ 1-based
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

        g_Geom = InputGeometry{};
        g_Geom.Source    = GeomSource::ObjFile;
        g_Geom.ObjPath   = path;
        g_Geom.Vertices  = std::move(verts);
        g_Geom.Triangles = std::move(tris);
        g_Geom.AreaTypes.assign(g_Geom.Triangles.size() / 3, RC_WALKABLE_AREA);
        rcCalcBounds(g_Geom.Vertices.data(),
                     static_cast<int>(g_Geom.Vertices.size() / 3),
                     &g_Geom.Bounds[0], &g_Geom.Bounds[3]);
        g_bGeomDirty = true;
        return true;
    }

    // =================================================================================
    // 6. NavMesh 构建/释放/序列化
    // =================================================================================
    void DestroyNavRuntime()
    {
        if (g_Nav.NavQuery)       { dtFreeNavMeshQuery(g_Nav.NavQuery); g_Nav.NavQuery = nullptr; }
        if (g_Nav.NavMesh)        { dtFreeNavMesh(g_Nav.NavMesh);       g_Nav.NavMesh  = nullptr; }
        if (g_Nav.PolyMeshDetail) { rcFreePolyMeshDetail(g_Nav.PolyMeshDetail); g_Nav.PolyMeshDetail = nullptr; }
        if (g_Nav.PolyMesh)       { rcFreePolyMesh(g_Nav.PolyMesh);     g_Nav.PolyMesh = nullptr; }
        g_Nav.bBuilt = false;
        g_Nav.NavMeshData.clear();
        g_StraightPath.clear();
        g_PathCorridor.clear();
        g_PathPolyCount = 0;
        g_PathStatus    = "(none)";
        g_Timings = PhaseTimings{};
    }

    // 用指定的 navData 创建 dtNavMesh + dtNavMeshQuery (用于 Load 路径)
    bool InitNavMeshFromData(unsigned char* navData, int navDataSize)
    {
        g_Nav.NavMesh = dtAllocNavMesh();
        dtStatus s = g_Nav.NavMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
        if (dtStatusFailed(s)) { g_BuildStatus = "dtNavMesh::init failed"; return false; }

        g_Nav.NavQuery = dtAllocNavMeshQuery();
        s = g_Nav.NavQuery->init(g_Nav.NavMesh, 2048);
        if (dtStatusFailed(s)) { g_BuildStatus = "dtNavMeshQuery::init failed"; return false; }

        g_Nav.bBuilt = true;
        return true;
    }

    bool BuildNavMesh()
    {
        DestroyNavRuntime();
        g_Nav.Ctx.LogLines.clear();
        g_Nav.Ctx.log(RC_LOG_PROGRESS, "Build NavMesh start");

        if (g_Geom.Vertices.empty() || g_Geom.Triangles.empty())
        {
            g_BuildStatus = "input geom is empty";
            return false;
        }

        const auto totalT0 = std::chrono::high_resolution_clock::now();

        rcConfig cfg{};
        cfg.cs                     = g_Config.CellSize;
        cfg.ch                     = g_Config.CellHeight;
        cfg.walkableSlopeAngle     = g_Config.AgentMaxSlope;
        cfg.walkableHeight         = static_cast<int>(std::ceil (g_Config.AgentHeight  / cfg.ch));
        cfg.walkableClimb          = static_cast<int>(std::floor(g_Config.AgentMaxClimb / cfg.ch));
        cfg.walkableRadius         = static_cast<int>(std::ceil (g_Config.AgentRadius  / cfg.cs));
        cfg.maxEdgeLen             = static_cast<int>(g_Config.EdgeMaxLen / cfg.cs);
        cfg.maxSimplificationError = g_Config.EdgeMaxError;
        cfg.minRegionArea          = static_cast<int>(rcSqr(g_Config.RegionMinSize));
        cfg.mergeRegionArea        = static_cast<int>(rcSqr(g_Config.RegionMergeSize));
        cfg.maxVertsPerPoly        = g_Config.VertsPerPoly;
        cfg.detailSampleDist       = g_Config.DetailSampleDist < 0.9f ? 0.0f : g_Config.CellSize  * g_Config.DetailSampleDist;
        cfg.detailSampleMaxError   = g_Config.CellHeight * g_Config.DetailSampleMaxError;

        rcVcopy(cfg.bmin, &g_Geom.Bounds[0]);
        rcVcopy(cfg.bmax, &g_Geom.Bounds[3]);
        rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

        rcHeightfield* solid = rcAllocHeightfield();
        if (!solid || !rcCreateHeightfield(&g_Nav.Ctx, *solid,
                                           cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch))
        {
            rcFreeHeightField(solid);
            g_BuildStatus = "rcCreateHeightfield failed";
            return false;
        }

        const int nverts = static_cast<int>(g_Geom.Vertices.size() / 3);
        const int ntris  = static_cast<int>(g_Geom.Triangles.size() / 3);
        // triareas 起手沿用 g_Geom.AreaTypes:
        //   - Procedural: 障碍下方三角形已被标 RC_NULL_AREA, 其它为 RC_WALKABLE_AREA
        //   - OBJ:        全部 RC_WALKABLE_AREA
        // 然后用 rcClearUnwalkableTriangles 把陡坡(法线超过 walkableSlopeAngle)清成
        // RC_NULL_AREA. 这一步 *不会* 把已标 NULL 的三角形改回 walkable, 因此障碍标记得以保留.
        std::vector<unsigned char> triareas = g_Geom.AreaTypes;
        triareas.resize(ntris, RC_NULL_AREA);
        rcClearUnwalkableTriangles(&g_Nav.Ctx, cfg.walkableSlopeAngle,
                                   g_Geom.Vertices.data(), nverts,
                                   g_Geom.Triangles.data(), ntris,
                                   triareas.data());

        {
            ScopedTimer st(&g_Timings.RasterizeMs);
            if (!rcRasterizeTriangles(&g_Nav.Ctx, g_Geom.Vertices.data(), nverts,
                                      g_Geom.Triangles.data(), triareas.data(),
                                      ntris, *solid, cfg.walkableClimb))
            {
                rcFreeHeightField(solid);
                g_BuildStatus = "rcRasterizeTriangles failed";
                return false;
            }
        }

        {
            ScopedTimer st(&g_Timings.FilterMs);
            rcFilterLowHangingWalkableObstacles(&g_Nav.Ctx, cfg.walkableClimb, *solid);
            rcFilterLedgeSpans(&g_Nav.Ctx, cfg.walkableHeight, cfg.walkableClimb, *solid);
            rcFilterWalkableLowHeightSpans(&g_Nav.Ctx, cfg.walkableHeight, *solid);
        }

        rcCompactHeightfield* chf = rcAllocCompactHeightfield();
        {
            ScopedTimer st(&g_Timings.CompactMs);
            if (!chf || !rcBuildCompactHeightfield(&g_Nav.Ctx, cfg.walkableHeight, cfg.walkableClimb, *solid, *chf))
            {
                rcFreeCompactHeightfield(chf);
                rcFreeHeightField(solid);
                g_BuildStatus = "rcBuildCompactHeightfield failed";
                return false;
            }
        }
        rcFreeHeightField(solid); solid = nullptr;

        {
            ScopedTimer st(&g_Timings.ErodeMs);
            if (!rcErodeWalkableArea(&g_Nav.Ctx, cfg.walkableRadius, *chf))
            {
                rcFreeCompactHeightfield(chf);
                g_BuildStatus = "rcErodeWalkableArea failed";
                return false;
            }
        }
        {
            ScopedTimer st(&g_Timings.DistFieldMs);
            if (!rcBuildDistanceField(&g_Nav.Ctx, *chf))
            {
                rcFreeCompactHeightfield(chf);
                g_BuildStatus = "rcBuildDistanceField failed";
                return false;
            }
        }
        {
            ScopedTimer st(&g_Timings.RegionsMs);
            if (!rcBuildRegions(&g_Nav.Ctx, *chf, 0, cfg.minRegionArea, cfg.mergeRegionArea))
            {
                rcFreeCompactHeightfield(chf);
                g_BuildStatus = "rcBuildRegions failed";
                return false;
            }
        }

        rcContourSet* cset = rcAllocContourSet();
        {
            ScopedTimer st(&g_Timings.ContoursMs);
            if (!cset || !rcBuildContours(&g_Nav.Ctx, *chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *cset))
            {
                rcFreeContourSet(cset);
                rcFreeCompactHeightfield(chf);
                g_BuildStatus = "rcBuildContours failed";
                return false;
            }
        }

        g_Nav.PolyMesh = rcAllocPolyMesh();
        {
            ScopedTimer st(&g_Timings.PolyMeshMs);
            if (!g_Nav.PolyMesh || !rcBuildPolyMesh(&g_Nav.Ctx, *cset, cfg.maxVertsPerPoly, *g_Nav.PolyMesh))
            {
                rcFreeContourSet(cset);
                rcFreeCompactHeightfield(chf);
                g_BuildStatus = "rcBuildPolyMesh failed";
                return false;
            }
        }

        g_Nav.PolyMeshDetail = rcAllocPolyMeshDetail();
        {
            ScopedTimer st(&g_Timings.DetailMeshMs);
            if (!g_Nav.PolyMeshDetail || !rcBuildPolyMeshDetail(&g_Nav.Ctx,
                                                                *g_Nav.PolyMesh, *chf,
                                                                cfg.detailSampleDist,
                                                                cfg.detailSampleMaxError,
                                                                *g_Nav.PolyMeshDetail))
            {
                rcFreeContourSet(cset);
                rcFreeCompactHeightfield(chf);
                g_BuildStatus = "rcBuildPolyMeshDetail failed";
                return false;
            }
        }

        rcFreeContourSet(cset);          cset = nullptr;
        rcFreeCompactHeightfield(chf);   chf  = nullptr;

        for (int i = 0; i < g_Nav.PolyMesh->npolys; ++i)
        {
            if (g_Nav.PolyMesh->areas[i] == RC_WALKABLE_AREA)
                g_Nav.PolyMesh->flags[i] = 0x01;
        }

        dtNavMeshCreateParams params{};
        params.verts            = g_Nav.PolyMesh->verts;
        params.vertCount        = g_Nav.PolyMesh->nverts;
        params.polys            = g_Nav.PolyMesh->polys;
        params.polyAreas        = g_Nav.PolyMesh->areas;
        params.polyFlags        = g_Nav.PolyMesh->flags;
        params.polyCount        = g_Nav.PolyMesh->npolys;
        params.nvp              = g_Nav.PolyMesh->nvp;
        params.detailMeshes     = g_Nav.PolyMeshDetail->meshes;
        params.detailVerts      = g_Nav.PolyMeshDetail->verts;
        params.detailVertsCount = g_Nav.PolyMeshDetail->nverts;
        params.detailTris       = g_Nav.PolyMeshDetail->tris;
        params.detailTriCount   = g_Nav.PolyMeshDetail->ntris;
        params.walkableHeight   = g_Config.AgentHeight;
        params.walkableRadius   = g_Config.AgentRadius;
        params.walkableClimb    = g_Config.AgentMaxClimb;
        rcVcopy(params.bmin, g_Nav.PolyMesh->bmin);
        rcVcopy(params.bmax, g_Nav.PolyMesh->bmax);
        params.cs               = cfg.cs;
        params.ch               = cfg.ch;
        params.buildBvTree      = true;

        unsigned char* navData     = nullptr;
        int            navDataSize = 0;
        {
            ScopedTimer st(&g_Timings.DetourCreateMs);
            if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
            {
                g_BuildStatus = "dtCreateNavMeshData failed";
                return false;
            }
        }

        // 在交给 dtNavMesh 之前先复制一份, 用于 Save NavMesh
        g_Nav.NavMeshData.assign(navData, navData + navDataSize);

        if (!InitNavMeshFromData(navData, navDataSize))
        {
            // InitNavMeshFromData 内部失败时, 当 init 调用前 navData 没被释放, 我们手动 dtFree
            dtFree(navData);
            return false;
        }

        const auto totalT1 = std::chrono::high_resolution_clock::now();
        g_Timings.TotalMs  = std::chrono::duration<double, std::milli>(totalT1 - totalT0).count();
        g_BuildStatus      = "OK";
        g_bGeomDirty       = false;

        // 写入趋势图历史
        BuildStat snap;
        snap.Index        = ++g_BuildCounter;
        snap.T            = g_Timings;
        snap.InputVerts   = static_cast<int>(g_Geom.Vertices.size() / 3);
        snap.InputTris    = static_cast<int>(g_Geom.Triangles.size() / 3);
        snap.PolyCount    = g_Nav.PolyMesh ? g_Nav.PolyMesh->npolys : 0;
        snap.PolyVerts    = g_Nav.PolyMesh ? g_Nav.PolyMesh->nverts : 0;
        snap.DetailTris   = g_Nav.PolyMeshDetail ? g_Nav.PolyMeshDetail->ntris : 0;
        snap.NavDataBytes = static_cast<int>(g_Nav.NavMeshData.size());
        snap.bOk          = true;
        g_BuildHistory.push_back(snap);
        while ((int)g_BuildHistory.size() > kBuildHistoryMax)
            g_BuildHistory.pop_front();

        return true;
    }

    bool SaveNavMesh(const char* path, std::string& errOut)
    {
        if (g_Nav.NavMeshData.empty()) { errOut = "no navmesh data (build first)"; return false; }
        std::ofstream fs(path, std::ios::binary);
        if (!fs.is_open()) { errOut = "open failed"; return false; }
        const std::uint32_t magic   = 0x484E5631u; // 'HNV1'
        const std::uint32_t version = 1;
        const std::uint32_t size    = static_cast<std::uint32_t>(g_Nav.NavMeshData.size());
        fs.write(reinterpret_cast<const char*>(&magic),   sizeof(magic));
        fs.write(reinterpret_cast<const char*>(&version), sizeof(version));
        fs.write(reinterpret_cast<const char*>(&size),    sizeof(size));
        fs.write(reinterpret_cast<const char*>(g_Nav.NavMeshData.data()), size);
        return true;
    }

    bool LoadNavMesh(const char* path, std::string& errOut)
    {
        std::ifstream fs(path, std::ios::binary);
        if (!fs.is_open()) { errOut = "open failed"; return false; }
        std::uint32_t magic = 0, version = 0, size = 0;
        fs.read(reinterpret_cast<char*>(&magic),   sizeof(magic));
        fs.read(reinterpret_cast<char*>(&version), sizeof(version));
        fs.read(reinterpret_cast<char*>(&size),    sizeof(size));
        if (magic != 0x484E5631u || version != 1 || size == 0)
        {
            errOut = "bad header";
            return false;
        }
        std::vector<unsigned char> buf(size);
        fs.read(reinterpret_cast<char*>(buf.data()), size);
        if (!fs) { errOut = "read failed"; return false; }

        DestroyNavRuntime();
        // dtNavMesh 期望持有 dtAlloc 出的 buffer (随后会 dtFree)
        unsigned char* navData = static_cast<unsigned char*>(dtAlloc(size, DT_ALLOC_PERM));
        if (!navData) { errOut = "dtAlloc failed"; return false; }
        std::memcpy(navData, buf.data(), size);
        g_Nav.NavMeshData = std::move(buf);
        if (!InitNavMeshFromData(navData, static_cast<int>(size)))
        {
            errOut = g_BuildStatus;
            dtFree(navData);
            return false;
        }
        g_BuildStatus = "loaded";
        return true;
    }

    // =================================================================================
    // 7. 路径查询
    // =================================================================================
    bool FindPath()
    {
        g_StraightPath.clear();
        g_PathCorridor.clear();
        g_PathPolyCount = 0;

        if (!g_Nav.bBuilt || !g_Nav.NavQuery) { g_PathStatus = "Build NavMesh first"; return false; }

        // 一次寻路的性能快照
        PathStat snap;
        snap.Index = ++g_PathCounter;

        // 在 lambda 末尾把 snap 写进历史
        auto push_history = [&]
        {
            snap.Status = g_PathStatus;
            g_PathHistory.push_back(snap);
            while ((int)g_PathHistory.size() > kPathHistoryMax)
                g_PathHistory.pop_front();
        };

        const auto totalT0 = std::chrono::high_resolution_clock::now();
        auto stamp_total = [&]
        {
            const auto t1 = std::chrono::high_resolution_clock::now();
            snap.T.TotalMs = std::chrono::duration<double, std::milli>(t1 - totalT0).count();
        };

        dtQueryFilter filter;
        filter.setIncludeFlags(0xffff);
        filter.setExcludeFlags(0);
        // Y 方向容差按整个几何高度的一半 (再加一点余量) 自适应, 防止 OBJ 整体在 y!=0
        // 时点击位置离 navmesh 太远导致 findNearestPoly 找不到 polygon.
        const float yExtent = std::max(8.0f, (g_Geom.Bounds[4] - g_Geom.Bounds[1]) * 0.5f + 4.0f);
        const float halfExtents[3] = { 2.0f, yExtent, 2.0f };

        dtPolyRef startRef = 0, endRef = 0;
        float     nearestStart[3], nearestEnd[3];
        {
            ScopedTimer st(&snap.T.NearestStartMs);
            g_Nav.NavQuery->findNearestPoly(g_Start, halfExtents, &filter, &startRef, nearestStart);
        }
        {
            ScopedTimer st(&snap.T.NearestEndMs);
            g_Nav.NavQuery->findNearestPoly(g_End,   halfExtents, &filter, &endRef,   nearestEnd);
        }

        if (!startRef || !endRef)
        {
            g_PathStatus = "Start or end not on navmesh";
            stamp_total();
            push_history();
            return false;
        }

        constexpr int MAX_POLYS = 256;
        dtPolyRef polys[MAX_POLYS];
        int       npolys = 0;
        dtStatus  s = 0;
        {
            ScopedTimer st(&snap.T.FindPathMs);
            s = g_Nav.NavQuery->findPath(startRef, endRef,
                                         nearestStart, nearestEnd,
                                         &filter, polys, &npolys, MAX_POLYS);
        }
        if (dtStatusFailed(s) || npolys == 0)
        {
            g_PathStatus = "findPath failed";
            stamp_total();
            push_history();
            return false;
        }
        g_PathPolyCount = npolys;
        g_PathCorridor.assign(polys, polys + npolys);
        snap.CorridorPolys = npolys;

        constexpr int MAX_STRAIGHT = 256;
        float         straightPath[MAX_STRAIGHT * 3];
        unsigned char straightFlags[MAX_STRAIGHT];
        dtPolyRef     straightRefs[MAX_STRAIGHT];
        int           nstraight = 0;
        {
            ScopedTimer st(&snap.T.StraightPathMs);
            s = g_Nav.NavQuery->findStraightPath(nearestStart, nearestEnd,
                                                 polys, npolys,
                                                 straightPath, straightFlags, straightRefs,
                                                 &nstraight, MAX_STRAIGHT);
        }
        if (dtStatusFailed(s))
        {
            g_PathStatus = "findStraightPath failed";
            stamp_total();
            push_history();
            return false;
        }

        g_StraightPath.assign(straightPath, straightPath + nstraight * 3);
        snap.StraightCorners = nstraight;
        snap.bOk             = true;
        g_PathStatus = "OK (" + std::to_string(nstraight) + " corners, "
                     + std::to_string(npolys) + " polys)";

        stamp_total();
        push_history();
        return true;
    }

    void TryAutoReplan()
    {
        if (g_View.bAutoReplan && g_Nav.bBuilt) FindPath();
    }

    void TryAutoRebuild()
    {
        if (g_View.bAutoRebuild && g_bGeomDirty)
        {
            BuildNavMesh();
            TryAutoReplan();
        }
    }

    // =================================================================================
    // 8. 拣取 (2D / 3D)
    // =================================================================================
    // Möller-Trumbore: 射线与三角形相交, 命中时 outT > 0
    bool RayTriangle(const Vec3& orig, const Vec3& dir,
                     const Vec3& v0,   const Vec3& v1,   const Vec3& v2,
                     float& outT)
    {
        const Vec3  e1   = v1 - v0;
        const Vec3  e2   = v2 - v0;
        const Vec3  pvec = Cross(dir, e2);
        const float det  = Dot(e1, pvec);
        if (std::fabs(det) < 1e-7f) return false;
        const float invDet = 1.0f / det;
        const Vec3  tvec   = orig - v0;
        const float u = Dot(tvec, pvec) * invDet;
        if (u < 0.0f || u > 1.0f) return false;
        const Vec3  qvec = Cross(tvec, e1);
        const float v = Dot(dir, qvec) * invDet;
        if (v < 0.0f || u + v > 1.0f) return false;
        const float t = Dot(e2, qvec) * invDet;
        if (t <= 1e-4f) return false;
        outT = t;
        return true;
    }

    // 射线 vs 全部输入三角形, 返回最近命中点的世界坐标 (用于 OBJ pick)
    bool RaycastInputMesh(const Vec3& orig, const Vec3& dir,
                          float& outX, float& outY, float& outZ)
    {
        const int triCount = static_cast<int>(g_Geom.Triangles.size() / 3);
        if (triCount == 0) return false;
        float bestT = std::numeric_limits<float>::infinity();
        bool  hit   = false;
        for (int t = 0; t < triCount; ++t)
        {
            const int*   tri = &g_Geom.Triangles[t * 3];
            const float* va  = &g_Geom.Vertices[tri[0] * 3];
            const float* vb  = &g_Geom.Vertices[tri[1] * 3];
            const float* vc  = &g_Geom.Vertices[tri[2] * 3];
            float ht;
            if (RayTriangle(orig, dir,
                            V3(va[0], va[1], va[2]),
                            V3(vb[0], vb[1], vb[2]),
                            V3(vc[0], vc[1], vc[2]),
                            ht) && ht < bestT)
            {
                bestT = ht;
                hit   = true;
            }
        }
        if (!hit) return false;
        outX = orig.x + dir.x * bestT;
        outY = orig.y + dir.y * bestT;
        outZ = orig.z + dir.z * bestT;
        return true;
    }

    bool ScreenTo2DWorld(const ImVec2& mp, float& wx, float& wy, float& wz)
    {
        if (!g_LastMap2D.bValid) return false;
        const float* bmin = g_LastMap2D.Bmin;
        const float* bmax = g_LastMap2D.Bmax;
        const float fx = (mp.x - g_LastMap2D.CanvasMin.x) / g_LastMap2D.CanvasSize.x;
        const float fy = (mp.y - g_LastMap2D.CanvasMin.y) / g_LastMap2D.CanvasSize.y;
        wx = bmin[0] + fx * (bmax[0] - bmin[0]);
        wz = bmin[2] + (1.0f - fy) * (bmax[2] - bmin[2]);
        wy = (bmin[1] + bmax[1]) * 0.5f;  // 顶视图无法获得真实 Y, 以包围盒中心 Y 兜底
        return true;
    }

    // 3D: 把屏幕像素 -> 视图射线; 优先与 OBJ 三角形求交, 退回与 y=bmid 平面求交
    bool ScreenTo3DGroundWorld(const ImVec2& mp, float& wx, float& wy, float& wz)
    {
        if (!g_LastMap3D.bValid) return false;
        const float ndcX = ((mp.x - g_LastMap3D.ViewportMin.x) / g_LastMap3D.ViewportSize.x) * 2.0f - 1.0f;
        const float ndcY = 1.0f - ((mp.y - g_LastMap3D.ViewportMin.y) / g_LastMap3D.ViewportSize.y) * 2.0f;

        const Vec3 eye    = g_LastMap3D.EyeWorld;
        const Vec3 target = g_Cam.Target;
        const Vec3 fwd    = Normalize(target - eye);
        const Vec3 right  = Normalize(Cross(fwd, V3(0, 1, 0)));
        const Vec3 up     = Cross(right, fwd);
        const float aspect = g_LastMap3D.ViewportSize.x / g_LastMap3D.ViewportSize.y;
        const float th     = std::tan(g_Cam.Fovy * 0.5f);
        const Vec3  dir    = Normalize(fwd + right * (ndcX * th * aspect) + up * (ndcY * th));

        // OBJ: 真三角形 raycast, 拿到模型表面点 (含 Y)
        if (g_Geom.Source == GeomSource::ObjFile && !g_Geom.Triangles.empty())
        {
            if (RaycastInputMesh(eye, dir, wx, wy, wz)) return true;
        }

        // Procedural / fallback: 射线交 y = bmid 平面
        const float ymid = (g_Geom.Bounds[1] + g_Geom.Bounds[4]) * 0.5f;
        if (std::fabs(dir.y) < 1e-5f) return false;
        const float t = (ymid - eye.y) / dir.y;
        if (t <= 0.0f) return false;
        wx = eye.x + dir.x * t;
        wy = ymid;
        wz = eye.z + dir.z * t;
        return true;
    }

    // 当前画布根据 view mode 选择拣取
    bool PickGroundWorld(const ImVec2& mp, float& wx, float& wy, float& wz)
    {
        if (g_ViewMode == ViewMode::TopDown2D) return ScreenTo2DWorld(mp, wx, wy, wz);
        return ScreenTo3DGroundWorld(mp, wx, wy, wz);
    }

    int FindObstacleAt(float wx, float wz)
    {
        for (int i = static_cast<int>(g_Geom.Obstacles.size()) - 1; i >= 0; --i)
        {
            if (PointInsideObstacle(wx, wz, g_Geom.Obstacles[i])) return i;
        }
        return -1;
    }

    // =================================================================================
    // 9. 2D 绘制
    // =================================================================================
    ImVec2 World2D(float wx, float wz, ImVec2 cmin, ImVec2 csz, const float bmin[3], const float bmax[3])
    {
        const float fx = (wx - bmin[0]) / (bmax[0] - bmin[0]);
        const float fz = (wz - bmin[2]) / (bmax[2] - bmin[2]);
        return ImVec2(cmin.x + fx * csz.x, cmin.y + (1.0f - fz) * csz.y);
    }

    void DrawCanvas2D(const ImVec2& panelMin, const ImVec2& panelSize)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 panelMax(panelMin.x + panelSize.x, panelMin.y + panelSize.y);
        dl->AddRectFilled(panelMin, panelMax, IM_COL32(40, 42, 48, 255));
        dl->AddRect(panelMin, panelMax, IM_COL32(90, 90, 100, 255));

        const float* bmin   = g_Geom.Bounds + 0;
        const float* bmax   = g_Geom.Bounds + 3;
        const float  worldW = bmax[0] - bmin[0];
        const float  worldH = bmax[2] - bmin[2];
        if (worldW <= 0 || worldH <= 0) return;

        constexpr float padding = 8.0f;
        const float maxW  = std::max(40.0f, panelSize.x - padding * 2.0f);
        const float maxH  = std::max(40.0f, panelSize.y - padding * 2.0f);
        const float scale = std::min(maxW / worldW, maxH / worldH);
        const ImVec2 csz(worldW * scale, worldH * scale);
        const ImVec2 cmin(panelMin.x + (panelSize.x - csz.x) * 0.5f,
                          panelMin.y + (panelSize.y - csz.y) * 0.5f);
        const ImVec2 cmax(cmin.x + csz.x, cmin.y + csz.y);

        g_LastMap2D.bValid     = true;
        g_LastMap2D.CanvasMin  = cmin;
        g_LastMap2D.CanvasSize = csz;
        std::memcpy(g_LastMap2D.Bmin, bmin, sizeof(float) * 3);
        std::memcpy(g_LastMap2D.Bmax, bmax, sizeof(float) * 3);

        // 网格
        if (g_View.bShowGrid)
        {
            for (int i = 0; i <= 6; ++i)
            {
                const float t = i / 6.0f;
                const ImVec2 a(cmin.x + t * csz.x, cmin.y);
                const ImVec2 b(cmin.x + t * csz.x, cmax.y);
                const ImVec2 c(cmin.x, cmin.y + t * csz.y);
                const ImVec2 d(cmax.x, cmin.y + t * csz.y);
                dl->AddLine(a, b, IM_COL32(70, 70, 80, 200));
                dl->AddLine(c, d, IM_COL32(70, 70, 80, 200));
            }
        }
        dl->AddRect(cmin, cmax, IM_COL32(120, 120, 130, 255));

        // 像素 / 世界 单位换算 (X 方向)
        const float pxPerWorld = csz.x / (bmax[0] - bmin[0]);

        // 输入几何 XZ 投影线框: 仅对 OBJ 渲染 (procedural 地面 = 网格本身, 避免双 grid)
        if (g_View.bShowInputMesh && g_Geom.Source == GeomSource::ObjFile && !g_Geom.Triangles.empty())
        {
            const ImU32 inputCol = IM_COL32(180, 200, 220, 140);
            const int triCount = static_cast<int>(g_Geom.Triangles.size() / 3);
            const int stride   = (triCount > 40000) ? (triCount / 40000 + 1) : 1;
            for (int t = 0; t < triCount; t += stride)
            {
                const int* tri = &g_Geom.Triangles[t * 3];
                const float* va = &g_Geom.Vertices[tri[0] * 3];
                const float* vb = &g_Geom.Vertices[tri[1] * 3];
                const float* vc = &g_Geom.Vertices[tri[2] * 3];
                const ImVec2 pa = World2D(va[0], va[2], cmin, csz, bmin, bmax);
                const ImVec2 pb = World2D(vb[0], vb[2], cmin, csz, bmin, bmax);
                const ImVec2 pc = World2D(vc[0], vc[2], cmin, csz, bmin, bmax);
                dl->AddLine(pa, pb, inputCol);
                dl->AddLine(pb, pc, inputCol);
                dl->AddLine(pc, pa, inputCol);
            }
        }

        // 障碍 (Box / Cylinder)
        if (g_View.bShowObstacles)
        {
            for (size_t i = 0; i < g_Geom.Obstacles.size(); ++i)
            {
                const Obstacle& o = g_Geom.Obstacles[i];
                const bool selected = (g_SelectedObstacle == (int)i);
                const bool dragging = (g_MoveState.Index   == (int)i);
                const ImU32 fill = IM_COL32(180, 80, 80, dragging ? 240 : (selected ? 220 : 180));
                const ImU32 edge = selected ? IM_COL32(255, 230, 100, 255) : IM_COL32(120, 50, 50, 255);
                const float lw   = selected ? 2.0f : 1.0f;
                if (o.Shape == ObstacleShape::Box)
                {
                    const ImVec2 p0 = World2D(o.CX - o.SX, o.CZ - o.SZ, cmin, csz, bmin, bmax);
                    const ImVec2 p1 = World2D(o.CX + o.SX, o.CZ + o.SZ, cmin, csz, bmin, bmax);
                    const ImVec2 a(std::min(p0.x, p1.x), std::min(p0.y, p1.y));
                    const ImVec2 b(std::max(p0.x, p1.x), std::max(p0.y, p1.y));
                    dl->AddRectFilled(a, b, fill);
                    dl->AddRect(a, b, edge, 0.0f, 0, lw);
                }
                else
                {
                    const ImVec2 c = World2D(o.CX, o.CZ, cmin, csz, bmin, bmax);
                    const float  r = o.Radius * pxPerWorld;
                    dl->AddCircleFilled(c, r, fill, 24);
                    dl->AddCircle(c, r, edge, 24, lw);
                }
            }
        }

        // PolyMesh
        if (g_Nav.bBuilt && g_Nav.PolyMesh && (g_View.bFillPolygons || g_View.bShowPolyEdges))
        {
            const rcPolyMesh* pm  = g_Nav.PolyMesh;
            const int         nvp = pm->nvp;
            const float       cs  = pm->cs;

            for (int i = 0; i < pm->npolys; ++i)
            {
                const unsigned short* p = &pm->polys[i * nvp * 2];
                std::vector<ImVec2> pts;
                pts.reserve(nvp);
                for (int j = 0; j < nvp; ++j)
                {
                    if (p[j] == RC_MESH_NULL_IDX) break;
                    const unsigned short* v  = &pm->verts[p[j] * 3];
                    const float           wx = pm->bmin[0] + v[0] * cs;
                    const float           wz = pm->bmin[2] + v[2] * cs;
                    pts.push_back(World2D(wx, wz, cmin, csz, bmin, bmax));
                }
                if (pts.size() < 3) continue;

                if (g_View.bFillPolygons)
                {
                    const ImU32 col = g_View.bRegionColors
                        ? HashColor(pm->regs ? pm->regs[i] : (unsigned)i, 0.45f)
                        : IM_COL32(80, 200, 100, 90);
                    // fan triangulation
                    for (size_t k = 1; k + 1 < pts.size(); ++k)
                        dl->AddTriangleFilled(pts[0], pts[k], pts[k + 1], col);
                }
                if (g_View.bShowPolyEdges)
                {
                    for (size_t k = 0; k < pts.size(); ++k)
                        dl->AddLine(pts[k], pts[(k + 1) % pts.size()], IM_COL32(80, 200, 100, 230));
                }
            }
        }

        // Detail Mesh
        if (g_View.bShowDetailMesh && g_Nav.bBuilt && g_Nav.PolyMeshDetail)
        {
            const rcPolyMeshDetail* dm = g_Nav.PolyMeshDetail;
            for (int i = 0; i < dm->ntris; ++i)
            {
                const unsigned char* t = &dm->tris[i * 4];
                // detail tris reference detailVerts directly via global indexing per submesh;
                // 这里我们为简化, 用每个 mesh 的 vertBase 加上 t[k]
                // 实际格式: 每个 polyMeshDetail->meshes[i*4 + 0..3] = (vertBase, vertCount, triBase, triCount)
                // tris 索引在 [0, vertCount), 需要加 vertBase 才是 detailVerts 索引
                // 这里 i 是全局三角形索引, 反查 mesh
                // 为简洁, 分块绘制:
                (void)t; // 用下面的分块方案代替
                break;
            }

            for (int m = 0; m < dm->nmeshes; ++m)
            {
                const unsigned int vertBase  = dm->meshes[m * 4 + 0];
                const unsigned int triBase   = dm->meshes[m * 4 + 2];
                const unsigned int triCount  = dm->meshes[m * 4 + 3];
                for (unsigned int i = 0; i < triCount; ++i)
                {
                    const unsigned char* t = &dm->tris[(triBase + i) * 4];
                    ImVec2 verts[3];
                    for (int k = 0; k < 3; ++k)
                    {
                        const float* v  = &dm->verts[(vertBase + t[k]) * 3];
                        verts[k] = World2D(v[0], v[2], cmin, csz, bmin, bmax);
                    }
                    dl->AddLine(verts[0], verts[1], IM_COL32(255, 255, 255, 60));
                    dl->AddLine(verts[1], verts[2], IM_COL32(255, 255, 255, 60));
                    dl->AddLine(verts[2], verts[0], IM_COL32(255, 255, 255, 60));
                }
            }
        }

        // 路径主色 + 走廊高亮色 (独立配置)
        const ImU32 pathColU32     = ColU32(g_PathColor);
        const ImU32 corridorColU32 = ColU32(g_CorridorColor);
        if (g_View.bShowPathCorridor && g_Nav.bBuilt && g_Nav.NavMesh && !g_PathCorridor.empty())
        {
            for (dtPolyRef ref : g_PathCorridor)
            {
                const dtMeshTile* tile = nullptr;
                const dtPoly*     poly = nullptr;
                if (dtStatusFailed(g_Nav.NavMesh->getTileAndPolyByRef(ref, &tile, &poly))) continue;
                if (!tile || !poly) continue;
                std::vector<ImVec2> pts;
                pts.reserve(poly->vertCount);
                for (int j = 0; j < poly->vertCount; ++j)
                {
                    const float* v = &tile->verts[poly->verts[j] * 3];
                    pts.push_back(World2D(v[0], v[2], cmin, csz, bmin, bmax));
                }
                if (pts.size() < 3) continue;
                for (size_t k = 1; k + 1 < pts.size(); ++k)
                    dl->AddTriangleFilled(pts[0], pts[k], pts[k + 1], corridorColU32);
            }
        }

        // 直线路径
        if (g_StraightPath.size() >= 6)
        {
            const size_t n = g_StraightPath.size() / 3;
            for (size_t i = 0; i + 1 < n; ++i)
            {
                const ImVec2 a = World2D(g_StraightPath[i * 3 + 0], g_StraightPath[i * 3 + 2], cmin, csz, bmin, bmax);
                const ImVec2 b = World2D(g_StraightPath[(i + 1) * 3 + 0], g_StraightPath[(i + 1) * 3 + 2], cmin, csz, bmin, bmax);
                dl->AddLine(a, b, pathColU32, 2.5f);
            }
        }

        // Start / End: 胶囊体在 2D 俯视下退化为半径 = AgentRadius 的圆 (内部小圆为中心点)
        const ImVec2 sp = World2D(g_Start[0], g_Start[2], cmin, csz, bmin, bmax);
        const ImVec2 ep = World2D(g_End[0],   g_End[2],   cmin, csz, bmin, bmax);
        const float  capR = std::max(2.0f, g_Config.AgentRadius * pxPerWorld);
        const ImU32  startCol = IM_COL32( 80, 160, 255, 255);
        const ImU32  endCol   = IM_COL32(255,  80,  80, 255);
        const ImU32  startFill = IM_COL32( 80, 160, 255, 90);
        const ImU32  endFill   = IM_COL32(255,  80,  80, 90);
        dl->AddCircleFilled(sp, capR, startFill, 24);
        dl->AddCircle(sp, capR, startCol, 24, 2.0f);
        dl->AddCircleFilled(sp, 3.0f, startCol);
        dl->AddCircleFilled(ep, capR, endFill,   24);
        dl->AddCircle(ep, capR, endCol,   24, 2.0f);
        dl->AddCircleFilled(ep, 3.0f, endCol);

        // 障碍创建草稿: Box 矩形 / Cylinder 圆
        if (g_EditMode == EditMode::CreateBox && g_CreateDraft.bActive)
        {
            const ImVec2 a = World2D(g_CreateDraft.StartX, g_CreateDraft.StartZ, cmin, csz, bmin, bmax);
            const ImVec2 b = World2D(g_CreateDraft.CurX,   g_CreateDraft.CurZ,   cmin, csz, bmin, bmax);
            if (g_DefaultObstacleShape == ObstacleShape::Box)
            {
                const ImVec2 lo(std::min(a.x, b.x), std::min(a.y, b.y));
                const ImVec2 hi(std::max(a.x, b.x), std::max(a.y, b.y));
                dl->AddRectFilled(lo, hi, IM_COL32(255, 230, 100, 80));
                dl->AddRect(lo, hi, IM_COL32(255, 230, 100, 230), 0.0f, 0, 2.0f);
            }
            else
            {
                const float dx = (g_CreateDraft.CurX - g_CreateDraft.StartX);
                const float dz = (g_CreateDraft.CurZ - g_CreateDraft.StartZ);
                const float rw = std::sqrt(dx * dx + dz * dz);
                const float rp = std::max(2.0f, rw * pxPerWorld);
                dl->AddCircleFilled(a, rp, IM_COL32(255, 230, 100, 80), 24);
                dl->AddCircle(a, rp, IM_COL32(255, 230, 100, 230), 24, 2.0f);
            }
        }
    }

    // =================================================================================
    // 10. 3D 绘制 (软件投影)
    // =================================================================================
    struct ProjectedPoint { ImVec2 px; float w; bool ok; };

    inline ProjectedPoint Project(const Mat4& vp, Vec3 wp, ImVec2 vMin, ImVec2 vSize)
    {
        float c[4];
        TransformPoint(vp, wp, c);
        ProjectedPoint r{};
        r.w  = c[3];
        if (c[3] <= 1e-3f) { r.ok = false; r.px = ImVec2(0, 0); return r; }
        const float ndcX = c[0] / c[3];
        const float ndcY = c[1] / c[3];
        r.px.x = vMin.x + (ndcX * 0.5f + 0.5f) * vSize.x;
        r.px.y = vMin.y + (1.0f - (ndcY * 0.5f + 0.5f)) * vSize.y;
        r.ok = true;
        return r;
    }

    inline void Line3D(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
                       Vec3 a, Vec3 b, ImU32 col, float thickness = 1.0f)
    {
        ProjectedPoint pa = Project(vp, a, vMin, vSize);
        ProjectedPoint pb = Project(vp, b, vMin, vSize);
        if (pa.ok && pb.ok) dl->AddLine(pa.px, pb.px, col, thickness);
    }

    inline void TriFilled3D(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
                            Vec3 a, Vec3 b, Vec3 c, ImU32 col)
    {
        ProjectedPoint pa = Project(vp, a, vMin, vSize);
        ProjectedPoint pb = Project(vp, b, vMin, vSize);
        ProjectedPoint pc = Project(vp, c, vMin, vSize);
        if (pa.ok && pb.ok && pc.ok) dl->AddTriangleFilled(pa.px, pb.px, pc.px, col);
    }

    // 在 XZ 平面 + 给定 y 高度上画一个 N 段线圈
    inline void RingXZ(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
                       float cx, float cz, float y, float r, int segs, ImU32 col)
    {
        Vec3 prev = V3(cx + r, y, cz);
        for (int i = 1; i <= segs; ++i)
        {
            const float a   = (i / (float)segs) * 6.28318530718f;
            const Vec3  cur = V3(cx + r * std::cos(a), y, cz + r * std::sin(a));
            Line3D(dl, vp, vMin, vSize, prev, cur, col);
            prev = cur;
        }
    }

    // 顶面 (XZ 圆盘)填充, 圆心 (cx, y, cz)
    inline void DiscXZFilled(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
                             float cx, float cz, float y, float r, int segs, ImU32 col)
    {
        Vec3 prev = V3(cx + r, y, cz);
        for (int i = 1; i <= segs; ++i)
        {
            const float a   = (i / (float)segs) * 6.28318530718f;
            const Vec3  cur = V3(cx + r * std::cos(a), y, cz + r * std::sin(a));
            TriFilled3D(dl, vp, vMin, vSize, V3(cx, y, cz), prev, cur, col);
            prev = cur;
        }
    }

    // 圆柱体障碍 (从 y=0 到 y=h)
    void DrawCylinderObstacle3D(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
                                float cx, float cz, float r, float h,
                                ImU32 fillTop, ImU32 edgeCol)
    {
        constexpr int segs = 24;
        // 顶面填充
        DiscXZFilled(dl, vp, vMin, vSize, cx, cz, h, r, segs, fillTop);
        // 顶/底圆环
        RingXZ(dl, vp, vMin, vSize, cx, cz, 0.0f, r, segs, edgeCol);
        RingXZ(dl, vp, vMin, vSize, cx, cz, h,    r, segs, edgeCol);
        // 8 条立柱
        for (int i = 0; i < 8; ++i)
        {
            const float a = (i / 8.0f) * 6.28318530718f;
            const float x = cx + r * std::cos(a);
            const float z = cz + r * std::sin(a);
            Line3D(dl, vp, vMin, vSize, V3(x, 0, z), V3(x, h, z), edgeCol);
        }
    }

    // 胶囊: 中心轴 (cx, cz), 总高 height (>=2r), 半径 r
    void DrawCapsule3D(ImDrawList* dl, const Mat4& vp, ImVec2 vMin, ImVec2 vSize,
                       float cx, float cz, float r, float height, ImU32 col,
                       float groundY = 0.0f)
    {
        if (r <= 0.0f) return;
        const float h    = std::max(height, 2.0f * r);
        const float yBot = groundY + r;          // 底半球 与 圆柱 的接缝 (相对 ground)
        const float yTop = groundY + h - r;      // 圆柱 与 顶半球 的接缝
        constexpr int  ringSeg = 18;
        constexpr int  hemiSeg = 6;
        constexpr int  meridi  = 4;    // 经线条数 (含 4 条立柱)

        // 三条圆环 (底/中/顶)
        RingXZ(dl, vp, vMin, vSize, cx, cz, yBot,                r, ringSeg, col);
        RingXZ(dl, vp, vMin, vSize, cx, cz, groundY + h * 0.5f,  r, ringSeg, col);
        RingXZ(dl, vp, vMin, vSize, cx, cz, yTop,                r, ringSeg, col);

        // 立柱 (圆柱侧)
        for (int i = 0; i < meridi; ++i)
        {
            const float a = (i / (float)meridi) * 6.28318530718f;
            const float x = cx + r * std::cos(a);
            const float z = cz + r * std::sin(a);
            Line3D(dl, vp, vMin, vSize, V3(x, yBot, z), V3(x, yTop, z), col);
        }
        // 顶半球 经线弧
        for (int i = 0; i < meridi; ++i)
        {
            const float a  = (i / (float)meridi) * 6.28318530718f;
            const float ca = std::cos(a), sa = std::sin(a);
            Vec3 prev = V3(cx + r * ca, yTop, cz + r * sa);
            for (int k = 1; k <= hemiSeg; ++k)
            {
                const float phi = (k / (float)hemiSeg) * 1.57079632679f;
                const float rr  = r * std::cos(phi);
                const float yy  = yTop + r * std::sin(phi);
                Vec3 cur = V3(cx + rr * ca, yy, cz + rr * sa);
                Line3D(dl, vp, vMin, vSize, prev, cur, col);
                prev = cur;
            }
        }
        // 底半球 经线弧
        for (int i = 0; i < meridi; ++i)
        {
            const float a  = (i / (float)meridi) * 6.28318530718f;
            const float ca = std::cos(a), sa = std::sin(a);
            Vec3 prev = V3(cx + r * ca, yBot, cz + r * sa);
            for (int k = 1; k <= hemiSeg; ++k)
            {
                const float phi = (k / (float)hemiSeg) * 1.57079632679f;
                const float rr  = r * std::cos(phi);
                const float yy  = yBot - r * std::sin(phi);
                Vec3 cur = V3(cx + rr * ca, yy, cz + rr * sa);
                Line3D(dl, vp, vMin, vSize, prev, cur, col);
                prev = cur;
            }
        }
    }

    void DrawCanvas3D(const ImVec2& panelMin, const ImVec2& panelSize)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 panelMax(panelMin.x + panelSize.x, panelMin.y + panelSize.y);
        dl->AddRectFilled(panelMin, panelMax, IM_COL32(28, 30, 36, 255));
        dl->AddRect(panelMin, panelMax, IM_COL32(90, 90, 100, 255));
        dl->PushClipRect(panelMin, panelMax, true);

        // 相机: target 由 FrameCameraToBounds()/WASD 显式管理, 这里不再每帧覆写,
        // 否则 WASD 平移会在下一帧被擦掉.
        const float* bmin = g_Geom.Bounds + 0;
        const float* bmax = g_Geom.Bounds + 3;
        const float cy = std::cos(g_Cam.Pitch);
        const float sy = std::sin(g_Cam.Pitch);
        const float cx = std::cos(g_Cam.Yaw);
        const float sx = std::sin(g_Cam.Yaw);
        const Vec3  eye = g_Cam.Target + V3(cx * cy, sy, sx * cy) * g_Cam.Distance;
        const float aspect = panelSize.x / std::max(1.0f, panelSize.y);
        const Mat4  view  = MakeLookAtRH(eye, g_Cam.Target, V3(0, 1, 0));
        const Mat4  proj  = MakePerspectiveRH(g_Cam.Fovy, aspect, 0.1f, 5000.0f);
        const Mat4  vp    = MatMul(proj, view);

        g_LastMap3D.bValid       = true;
        g_LastMap3D.ViewportMin  = panelMin;
        g_LastMap3D.ViewportSize = panelSize;
        g_LastMap3D.EyeWorld     = eye;
        g_LastMap3D.View         = view;
        g_LastMap3D.Proj         = proj;

        // 网格 (在 y=0 平面)
        if (g_View.bShowGrid)
        {
            const int   gridN = 12;
            const float w     = bmax[0] - bmin[0];
            const float h     = bmax[2] - bmin[2];
            for (int i = 0; i <= gridN; ++i)
            {
                const float t = static_cast<float>(i) / gridN;
                Line3D(dl, vp, panelMin, panelSize,
                       V3(bmin[0] + t * w, 0, bmin[2]),
                       V3(bmin[0] + t * w, 0, bmax[2]),
                       IM_COL32(70, 70, 80, 200));
                Line3D(dl, vp, panelMin, panelSize,
                       V3(bmin[0], 0, bmin[2] + t * h),
                       V3(bmax[0], 0, bmin[2] + t * h),
                       IM_COL32(70, 70, 80, 200));
            }
        }

        // 输入几何线框: 仅对 OBJ 渲染 (procedural 地面本身就是一片网格三角形, 与上面的 Grid
        // 叠加会出现"双重 grid"). 大模型时 (>40k tri) 稀疏采样, 防止 ImDrawList 卡顿.
        if (g_View.bShowInputMesh && g_Geom.Source == GeomSource::ObjFile && !g_Geom.Triangles.empty())
        {
            const ImU32 inputCol = IM_COL32(180, 200, 220, 110);
            const int triCount = static_cast<int>(g_Geom.Triangles.size() / 3);
            const int stride   = (triCount > 40000) ? (triCount / 40000 + 1) : 1;
            for (int t = 0; t < triCount; t += stride)
            {
                const int* tri = &g_Geom.Triangles[t * 3];
                const float* va = &g_Geom.Vertices[tri[0] * 3];
                const float* vb = &g_Geom.Vertices[tri[1] * 3];
                const float* vc = &g_Geom.Vertices[tri[2] * 3];
                const Vec3 a = V3(va[0], va[1], va[2]);
                const Vec3 b = V3(vb[0], vb[1], vb[2]);
                const Vec3 c = V3(vc[0], vc[1], vc[2]);
                Line3D(dl, vp, panelMin, panelSize, a, b, inputCol);
                Line3D(dl, vp, panelMin, panelSize, b, c, inputCol);
                Line3D(dl, vp, panelMin, panelSize, c, a, inputCol);
            }
        }

        // 障碍 (Box / Cylinder, 高度独立)
        if (g_View.bShowObstacles)
        {
            for (size_t i = 0; i < g_Geom.Obstacles.size(); ++i)
            {
                const Obstacle& o = g_Geom.Obstacles[i];
                const bool   selected = (g_SelectedObstacle == (int)i);
                const bool   dragging = (g_MoveState.Index   == (int)i);
                const ImU32  fill = IM_COL32(180, 80, 80, dragging ? 220 : (selected ? 200 : 140));
                const ImU32  edge = selected ? IM_COL32(255, 230, 100, 255) : IM_COL32(220, 110, 110, 230);
                const float  H    = o.Height;

                if (o.Shape == ObstacleShape::Box)
                {
                    const float minX = o.CX - o.SX, maxX = o.CX + o.SX;
                    const float minZ = o.CZ - o.SZ, maxZ = o.CZ + o.SZ;
                    Vec3 c000{ minX, 0, minZ }, c100{ maxX, 0, minZ };
                    Vec3 c110{ maxX, 0, maxZ }, c010{ minX, 0, maxZ };
                    Vec3 c001{ minX, H, minZ }, c101{ maxX, H, minZ };
                    Vec3 c111{ maxX, H, maxZ }, c011{ minX, H, maxZ };
                    TriFilled3D(dl, vp, panelMin, panelSize, c001, c101, c111, fill);
                    TriFilled3D(dl, vp, panelMin, panelSize, c001, c111, c011, fill);
                    Line3D(dl, vp, panelMin, panelSize, c000, c100, edge);
                    Line3D(dl, vp, panelMin, panelSize, c100, c110, edge);
                    Line3D(dl, vp, panelMin, panelSize, c110, c010, edge);
                    Line3D(dl, vp, panelMin, panelSize, c010, c000, edge);
                    Line3D(dl, vp, panelMin, panelSize, c001, c101, edge);
                    Line3D(dl, vp, panelMin, panelSize, c101, c111, edge);
                    Line3D(dl, vp, panelMin, panelSize, c111, c011, edge);
                    Line3D(dl, vp, panelMin, panelSize, c011, c001, edge);
                    Line3D(dl, vp, panelMin, panelSize, c000, c001, edge);
                    Line3D(dl, vp, panelMin, panelSize, c100, c101, edge);
                    Line3D(dl, vp, panelMin, panelSize, c110, c111, edge);
                    Line3D(dl, vp, panelMin, panelSize, c010, c011, edge);
                }
                else
                {
                    DrawCylinderObstacle3D(dl, vp, panelMin, panelSize,
                                           o.CX, o.CZ, o.Radius, H, fill, edge);
                }
            }
        }

        // PolyMesh (微抬以避免 z-fighting 视觉)
        const float polyY = 0.05f;
        if (g_Nav.bBuilt && g_Nav.PolyMesh && (g_View.bFillPolygons || g_View.bShowPolyEdges))
        {
            const rcPolyMesh* pm  = g_Nav.PolyMesh;
            const int         nvp = pm->nvp;
            const float       cs  = pm->cs;
            const float       ch  = pm->ch;
            for (int i = 0; i < pm->npolys; ++i)
            {
                const unsigned short* p = &pm->polys[i * nvp * 2];
                std::vector<Vec3> pts;
                pts.reserve(nvp);
                for (int j = 0; j < nvp; ++j)
                {
                    if (p[j] == RC_MESH_NULL_IDX) break;
                    const unsigned short* v = &pm->verts[p[j] * 3];
                    const float wx = pm->bmin[0] + v[0] * cs;
                    const float wy = pm->bmin[1] + v[1] * ch + polyY;
                    const float wz = pm->bmin[2] + v[2] * cs;
                    pts.push_back(V3(wx, wy, wz));
                }
                if (pts.size() < 3) continue;
                if (g_View.bFillPolygons)
                {
                    const ImU32 col = g_View.bRegionColors
                        ? HashColor(pm->regs ? pm->regs[i] : (unsigned)i, 0.55f)
                        : IM_COL32(80, 200, 100, 130);
                    for (size_t k = 1; k + 1 < pts.size(); ++k)
                        TriFilled3D(dl, vp, panelMin, panelSize, pts[0], pts[k], pts[k + 1], col);
                }
                if (g_View.bShowPolyEdges)
                {
                    for (size_t k = 0; k < pts.size(); ++k)
                        Line3D(dl, vp, panelMin, panelSize,
                               pts[k], pts[(k + 1) % pts.size()],
                               IM_COL32(80, 200, 100, 230));
                }
            }
        }

        // Detail mesh
        if (g_View.bShowDetailMesh && g_Nav.bBuilt && g_Nav.PolyMeshDetail)
        {
            const rcPolyMeshDetail* dm = g_Nav.PolyMeshDetail;
            for (int m = 0; m < dm->nmeshes; ++m)
            {
                const unsigned int vertBase = dm->meshes[m * 4 + 0];
                const unsigned int triBase  = dm->meshes[m * 4 + 2];
                const unsigned int triCount = dm->meshes[m * 4 + 3];
                for (unsigned int i = 0; i < triCount; ++i)
                {
                    const unsigned char* t = &dm->tris[(triBase + i) * 4];
                    Vec3 v3[3];
                    for (int k = 0; k < 3; ++k)
                    {
                        const float* v = &dm->verts[(vertBase + t[k]) * 3];
                        v3[k] = V3(v[0], v[1] + polyY * 0.5f, v[2]);
                    }
                    Line3D(dl, vp, panelMin, panelSize, v3[0], v3[1], IM_COL32(255, 255, 255, 70));
                    Line3D(dl, vp, panelMin, panelSize, v3[1], v3[2], IM_COL32(255, 255, 255, 70));
                    Line3D(dl, vp, panelMin, panelSize, v3[2], v3[0], IM_COL32(255, 255, 255, 70));
                }
            }
        }

        // 颜色: 路径主色 / 走廊高亮 (各自独立配置)
        const ImU32 pathColU32     = ColU32(g_PathColor);
        const ImU32 corridorColU32 = ColU32(g_CorridorColor);

        // 路径走廊
        if (g_View.bShowPathCorridor && g_Nav.bBuilt && g_Nav.NavMesh && !g_PathCorridor.empty())
        {
            for (dtPolyRef ref : g_PathCorridor)
            {
                const dtMeshTile* tile = nullptr;
                const dtPoly*     poly = nullptr;
                if (dtStatusFailed(g_Nav.NavMesh->getTileAndPolyByRef(ref, &tile, &poly))) continue;
                if (!tile || !poly) continue;
                std::vector<Vec3> pts;
                pts.reserve(poly->vertCount);
                for (int j = 0; j < poly->vertCount; ++j)
                {
                    const float* v = &tile->verts[poly->verts[j] * 3];
                    pts.push_back(V3(v[0], v[1] + polyY * 0.6f, v[2]));
                }
                if (pts.size() < 3) continue;
                for (size_t k = 1; k + 1 < pts.size(); ++k)
                    TriFilled3D(dl, vp, panelMin, panelSize, pts[0], pts[k], pts[k + 1], corridorColU32);
            }
        }

        // 直线路径 (轻微抬高)
        if (g_StraightPath.size() >= 6)
        {
            const size_t n = g_StraightPath.size() / 3;
            for (size_t i = 0; i + 1 < n; ++i)
            {
                const Vec3 a = V3(g_StraightPath[i * 3 + 0], g_StraightPath[i * 3 + 1] + 0.2f, g_StraightPath[i * 3 + 2]);
                const Vec3 b = V3(g_StraightPath[(i + 1) * 3 + 0], g_StraightPath[(i + 1) * 3 + 1] + 0.2f, g_StraightPath[(i + 1) * 3 + 2]);
                Line3D(dl, vp, panelMin, panelSize, a, b, pathColU32, 2.5f);
            }
        }

        // Start / End 渲染为胶囊体, 尺寸与 AgentRadius/AgentHeight 一致;
        // ground Y 取 g_Start[1]/g_End[1] (OBJ 模式下不为 0)
        DrawCapsule3D(dl, vp, panelMin, panelSize,
                      g_Start[0], g_Start[2], g_Config.AgentRadius, g_Config.AgentHeight,
                      IM_COL32(80, 160, 255, 255), g_Start[1]);
        DrawCapsule3D(dl, vp, panelMin, panelSize,
                      g_End[0],   g_End[2],   g_Config.AgentRadius, g_Config.AgentHeight,
                      IM_COL32(255, 80, 80, 255), g_End[1]);

        // 障碍创建草稿: Box 框线 / Cylinder 圆环
        if (g_EditMode == EditMode::CreateBox && g_CreateDraft.bActive)
        {
            const ImU32 col = IM_COL32(255, 230, 100, 230);
            const float h   = g_DefaultObstacleHeight;
            if (g_DefaultObstacleShape == ObstacleShape::Box)
            {
                const float lo[2] = { std::min(g_CreateDraft.StartX, g_CreateDraft.CurX),
                                      std::min(g_CreateDraft.StartZ, g_CreateDraft.CurZ) };
                const float hi[2] = { std::max(g_CreateDraft.StartX, g_CreateDraft.CurX),
                                      std::max(g_CreateDraft.StartZ, g_CreateDraft.CurZ) };
                Vec3 a{ lo[0], h, lo[1] }, b{ hi[0], h, lo[1] }, c{ hi[0], h, hi[1] }, d{ lo[0], h, hi[1] };
                Vec3 a0{ lo[0], 0, lo[1] }, b0{ hi[0], 0, lo[1] }, c0{ hi[0], 0, hi[1] }, d0{ lo[0], 0, hi[1] };
                Line3D(dl, vp, panelMin, panelSize, a, b, col); Line3D(dl, vp, panelMin, panelSize, b, c, col);
                Line3D(dl, vp, panelMin, panelSize, c, d, col); Line3D(dl, vp, panelMin, panelSize, d, a, col);
                Line3D(dl, vp, panelMin, panelSize, a0, b0, col); Line3D(dl, vp, panelMin, panelSize, b0, c0, col);
                Line3D(dl, vp, panelMin, panelSize, c0, d0, col); Line3D(dl, vp, panelMin, panelSize, d0, a0, col);
                Line3D(dl, vp, panelMin, panelSize, a0, a, col); Line3D(dl, vp, panelMin, panelSize, b0, b, col);
                Line3D(dl, vp, panelMin, panelSize, c0, c, col); Line3D(dl, vp, panelMin, panelSize, d0, d, col);
            }
            else
            {
                const float dx = (g_CreateDraft.CurX - g_CreateDraft.StartX);
                const float dz = (g_CreateDraft.CurZ - g_CreateDraft.StartZ);
                const float r  = std::sqrt(dx * dx + dz * dz);
                if (r > 0.05f)
                {
                    RingXZ(dl, vp, panelMin, panelSize, g_CreateDraft.StartX, g_CreateDraft.StartZ, 0.0f, r, 24, col);
                    RingXZ(dl, vp, panelMin, panelSize, g_CreateDraft.StartX, g_CreateDraft.StartZ, h,    r, 24, col);
                    for (int i = 0; i < 8; ++i)
                    {
                        const float a = (i / 8.0f) * 6.28318530718f;
                        const float x = g_CreateDraft.StartX + r * std::cos(a);
                        const float z = g_CreateDraft.StartZ + r * std::sin(a);
                        Line3D(dl, vp, panelMin, panelSize, V3(x, 0, z), V3(x, h, z), col);
                    }
                }
            }
        }

        dl->PopClipRect();
    }

    // =================================================================================
    // 11. 画布交互 + 绘制总入口
    // =================================================================================
    void HandleCanvasInteraction(const ImVec2& canvasItemMin, const ImVec2& canvasItemSize)
    {
        // ImGui::InvisibleButton 已在外部布好, 这里只读取 hover/active 状态
        const bool       hovered = ImGui::IsItemHovered();
        const bool       active  = ImGui::IsItemActive();
        const ImGuiIO&   io      = ImGui::GetIO();
        const ImVec2     mp      = ImGui::GetMousePos();
        (void)canvasItemMin; (void)canvasItemSize;

        // 3D 相机操作 (右键拖拽旋转, 滚轮缩放) — 优先处理, 不被编辑模式拦截
        if (g_ViewMode == ViewMode::Orbit3D)
        {
            if (hovered && io.MouseWheel != 0.0f)
            {
                g_Cam.Distance *= std::pow(1.1f, -io.MouseWheel);
                g_Cam.Distance = std::max(2.0f, std::min(800.0f, g_Cam.Distance));
            }
            if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f))
            {
                const ImVec2 d = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right, 0.0f);
                g_Cam.Yaw   += d.x * 0.005f;
                g_Cam.Pitch -= d.y * 0.005f;
                g_Cam.Pitch  = std::max(-1.5f, std::min(1.5f, g_Cam.Pitch));
                ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
            }
        }

        if (!hovered && !active) return;

        // 当前世界坐标 (左键操作总是 ground pick)
        float wx = 0.0f, wy = 0.0f, wz = 0.0f;
        const bool havePick = PickGroundWorld(mp, wx, wy, wz);

        // -------- PlaceStart / PlaceEnd --------
        if (g_EditMode == EditMode::PlaceStart || g_EditMode == EditMode::PlaceEnd)
        {
            if (hovered && havePick && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                if (g_EditMode == EditMode::PlaceStart)
                { g_Start[0] = wx; g_Start[1] = wy; g_Start[2] = wz; }
                else
                { g_End[0]   = wx; g_End[1]   = wy; g_End[2]   = wz; }
                TryAutoReplan();
            }
            return;
        }

        // -------- CreateBox --------
        if (g_EditMode == EditMode::CreateBox)
        {
            if (g_Geom.Source != GeomSource::Procedural) return;
            if (hovered && havePick && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                g_CreateDraft.bActive = true;
                g_CreateDraft.StartX  = wx;
                g_CreateDraft.StartZ  = wz;
                g_CreateDraft.CurX    = wx;
                g_CreateDraft.CurZ    = wz;
            }
            else if (g_CreateDraft.bActive && ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                if (havePick) { g_CreateDraft.CurX = wx; g_CreateDraft.CurZ = wz; }
            }
            else if (g_CreateDraft.bActive && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            {
                Obstacle ob;
                ob.Height = g_DefaultObstacleHeight;
                if (g_DefaultObstacleShape == ObstacleShape::Box)
                {
                    const float minX = std::min(g_CreateDraft.StartX, g_CreateDraft.CurX);
                    const float maxX = std::max(g_CreateDraft.StartX, g_CreateDraft.CurX);
                    const float minZ = std::min(g_CreateDraft.StartZ, g_CreateDraft.CurZ);
                    const float maxZ = std::max(g_CreateDraft.StartZ, g_CreateDraft.CurZ);
                    if ((maxX - minX) > 0.2f && (maxZ - minZ) > 0.2f)
                    {
                        ob.Shape = ObstacleShape::Box;
                        ob.CX = (minX + maxX) * 0.5f;
                        ob.CZ = (minZ + maxZ) * 0.5f;
                        ob.SX = (maxX - minX) * 0.5f;
                        ob.SZ = (maxZ - minZ) * 0.5f;
                        g_Geom.Obstacles.push_back(ob);
                        RebuildProceduralInputGeometry();
                        g_bGeomDirty = true;
                        TryAutoRebuild();
                    }
                }
                else
                {
                    const float dx = g_CreateDraft.CurX - g_CreateDraft.StartX;
                    const float dz = g_CreateDraft.CurZ - g_CreateDraft.StartZ;
                    const float r  = std::sqrt(dx * dx + dz * dz);
                    if (r > 0.2f)
                    {
                        ob.Shape  = ObstacleShape::Cylinder;
                        ob.CX     = g_CreateDraft.StartX;
                        ob.CZ     = g_CreateDraft.StartZ;
                        ob.Radius = r;
                        g_Geom.Obstacles.push_back(ob);
                        g_SelectedObstacle = static_cast<int>(g_Geom.Obstacles.size()) - 1;
                        RebuildProceduralInputGeometry();
                        g_bGeomDirty = true;
                        TryAutoRebuild();
                    }
                }
                if (g_DefaultObstacleShape == ObstacleShape::Box && !g_Geom.Obstacles.empty())
                    g_SelectedObstacle = static_cast<int>(g_Geom.Obstacles.size()) - 1;
                g_CreateDraft.bActive = false;
            }
            return;
        }

        // -------- Select & Move --------
        if (g_EditMode == EditMode::MoveBox)
        {
            if (g_Geom.Source != GeomSource::Procedural) return;
            if (hovered && havePick && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                int idx = FindObstacleAt(wx, wz);
                g_SelectedObstacle = idx;
                if (idx >= 0)
                {
                    g_MoveState.Index   = idx;
                    g_MoveState.OffsetX = wx - g_Geom.Obstacles[idx].CX;
                    g_MoveState.OffsetZ = wz - g_Geom.Obstacles[idx].CZ;
                }
            }
            else if (g_MoveState.Index >= 0 && ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                if (havePick && g_MoveState.Index < (int)g_Geom.Obstacles.size())
                {
                    Obstacle& o = g_Geom.Obstacles[g_MoveState.Index];
                    o.CX = wx - g_MoveState.OffsetX;
                    o.CZ = wz - g_MoveState.OffsetZ;
                }
            }
            else if (g_MoveState.Index >= 0 && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            {
                RebuildProceduralInputGeometry();
                g_bGeomDirty = true;
                g_MoveState.Index = -1;
                TryAutoRebuild();
            }
            return;
        }

        // -------- DeleteBox --------
        if (g_EditMode == EditMode::DeleteBox)
        {
            if (g_Geom.Source != GeomSource::Procedural) return;
            if (hovered && havePick && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                int idx = FindObstacleAt(wx, wz);
                if (idx >= 0)
                {
                    g_Geom.Obstacles.erase(g_Geom.Obstacles.begin() + idx);
                    if (g_SelectedObstacle == idx) g_SelectedObstacle = -1;
                    else if (g_SelectedObstacle > idx) --g_SelectedObstacle;
                    RebuildProceduralInputGeometry();
                    g_bGeomDirty = true;
                    TryAutoRebuild();
                }
            }
            return;
        }

        // -------- 任意模式: 左键单击障碍 = 选中, 空白处 = 取消选择 (用于配合快捷键) --------
        if (hovered && havePick && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
            && g_Geom.Source == GeomSource::Procedural)
        {
            g_SelectedObstacle = FindObstacleAt(wx, wz);
        }
    }

    // -------------------------------------------------------------------------------
    // 全局快捷键 (输入框激活时不响应)
    //   --- 场景导航 (按住, 仅 3D Orbit 视角生效) ---
    //   W / S     : 沿相机前/后方向平移 (XZ 平面)
    //   A / D     : 沿相机左/右方向平移
    //   Q / E     : Target.y 下降 / 上升
    //   Shift     : 加速 (×3)
    //   Ctrl      : 慢速 (×0.3)
    //   Home      : 重置 Camera (拉到包围盒)
    //   --- 编辑/操作 (单击) ---
    //   1 / 2     : 进入 PlaceStart / PlaceEnd
    //   T         : Select & Move (原 S, 与 WASD 冲突已迁移)
    //   B / C     : 创建 Box / Cylinder 障碍 (并切换默认形状)
    //   Delete/X  : 删除当前选中障碍
    //   R         : (Re)Build NavMesh
    //   F         : Find Path
    //   V         : 切换 2D / 3D
    //   Esc       : EditMode = None, 清除选择
    // -------------------------------------------------------------------------------
    void HandleHotkeys()
    {
        const ImGuiIO& io = ImGui::GetIO();
        if (io.WantTextInput) return;

        auto Pressed = [](int vk) { return ImGui::IsKeyPressed(vk, false); };

        // ---- WASD/QE 连续平移 (仅 3D 视角; 2D 顶视图 auto-fit 到包围盒, 无平移概念) ----
        if (g_ViewMode == ViewMode::Orbit3D)
        {
            const float dt = std::max(0.0f, io.DeltaTime);
            if (dt > 0.0f)
            {
                // forward = 视线在 XZ 平面的投影 (从 eye 指向 target)
                // eye = target + (cos(yaw)*cy, sy, sin(yaw)*cy) * dist
                // 故 (eye->target) 的 XZ 分量 = -(cos(yaw), sin(yaw))
                const float cyaw = std::cos(g_Cam.Yaw);
                const float syaw = std::sin(g_Cam.Yaw);
                const Vec3  fwd  = V3(-cyaw, 0.0f, -syaw);
                // right = cross(fwd, up=Y), up=(0,1,0): cross((fx,0,fz),(0,1,0))=(-fz,0,fx)
                const Vec3  right = V3(syaw, 0.0f, -cyaw);

                // 速度按 Distance 缩放, 拉得越远移动越快
                float speed = std::max(2.0f, g_Cam.Distance * 1.2f);
                if (io.KeyShift) speed *= 3.0f;
                if (io.KeyCtrl)  speed *= 0.3f;

                Vec3 move{ 0.0f, 0.0f, 0.0f };
                if (ImGui::IsKeyDown('W')) move = move + fwd;
                if (ImGui::IsKeyDown('S')) move = move - fwd;
                if (ImGui::IsKeyDown('D')) move = move + right;
                if (ImGui::IsKeyDown('A')) move = move - right;
                if (ImGui::IsKeyDown('E')) move.y += 1.0f;
                if (ImGui::IsKeyDown('Q')) move.y -= 1.0f;

                g_Cam.Target.x += move.x * speed * dt;
                g_Cam.Target.y += move.y * speed * dt;
                g_Cam.Target.z += move.z * speed * dt;
            }
        }

        if (Pressed(VK_ESCAPE))
        {
            g_EditMode         = EditMode::None;
            g_SelectedObstacle = -1;
        }
        if (Pressed('1')) g_EditMode = EditMode::PlaceStart;
        if (Pressed('2')) g_EditMode = EditMode::PlaceEnd;
        // 'T' = Select & Move (原 'S', 与 WASD 冲突已迁移)
        if (Pressed('T')) g_EditMode = EditMode::MoveBox;
        if (Pressed('B'))
        {
            g_EditMode             = EditMode::CreateBox;
            g_DefaultObstacleShape = ObstacleShape::Box;
        }
        if (Pressed('C'))
        {
            g_EditMode             = EditMode::CreateBox;
            g_DefaultObstacleShape = ObstacleShape::Cylinder;
        }
        if (Pressed(VK_DELETE) || Pressed('X'))
        {
            if (g_Geom.Source == GeomSource::Procedural &&
                g_SelectedObstacle >= 0 &&
                g_SelectedObstacle < static_cast<int>(g_Geom.Obstacles.size()))
            {
                g_Geom.Obstacles.erase(g_Geom.Obstacles.begin() + g_SelectedObstacle);
                g_SelectedObstacle = -1;
                g_MoveState.Index  = -1;
                RebuildProceduralInputGeometry();
                g_bGeomDirty = true;
                TryAutoRebuild();
            }
        }
        if (Pressed('R'))
        {
            BuildNavMesh();
            TryAutoReplan();
        }
        if (Pressed('F'))
        {
            FindPath();
        }
        if (Pressed('V'))
        {
            g_ViewMode = (g_ViewMode == ViewMode::TopDown2D) ? ViewMode::Orbit3D : ViewMode::TopDown2D;
        }
        if (Pressed(VK_HOME))
        {
            FrameCameraToBounds();
        }
    }

    void DrawCanvasPanel()
    {
        const ImVec2 panelMin  = ImGui::GetCursorScreenPos();
        const ImVec2 panelSize = ImGui::GetContentRegionAvail();
        if (panelSize.x < 50.0f || panelSize.y < 50.0f) return;

        // 1) 用 InvisibleButton 占位 + 让 ImGui 知道这块区域用于 hit-test
        ImGui::InvisibleButton("##NavCanvas", panelSize,
                               ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

        // 2) 在该区域上绘制
        if (g_ViewMode == ViewMode::TopDown2D) DrawCanvas2D(panelMin, panelSize);
        else                                   DrawCanvas3D(panelMin, panelSize);

        // 3) 处理交互
        HandleCanvasInteraction(panelMin, panelSize);
    }

    // =================================================================================
    // 12. ImGui 面板
    // =================================================================================
    void DrawViewPanel()
    {
        if (!ImGui::CollapsingHeader("View", ImGuiTreeNodeFlags_DefaultOpen)) return;
        int vm = (g_ViewMode == ViewMode::TopDown2D) ? 0 : 1;
        ImGui::TextUnformatted("View Mode:");
        ImGui::SameLine();
        if (ImGui::RadioButton("2D Top",  &vm, 0)) g_ViewMode = ViewMode::TopDown2D;
        ImGui::SameLine();
        if (ImGui::RadioButton("3D Orbit", &vm, 1)) g_ViewMode = ViewMode::Orbit3D;

        ImGui::Checkbox("Grid",          &g_View.bShowGrid);          ImGui::SameLine();
        ImGui::Checkbox("Input mesh",    &g_View.bShowInputMesh);     ImGui::SameLine();
        ImGui::Checkbox("Obstacles",     &g_View.bShowObstacles);
        ImGui::Checkbox("Fill polygons", &g_View.bFillPolygons);      ImGui::SameLine();
        ImGui::Checkbox("Region colors", &g_View.bRegionColors);
        ImGui::Checkbox("Poly edges",    &g_View.bShowPolyEdges);     ImGui::SameLine();
        ImGui::Checkbox("Detail mesh",   &g_View.bShowDetailMesh);
        ImGui::Checkbox("Path corridor", &g_View.bShowPathCorridor);  ImGui::SameLine();
        ImGui::Checkbox("Auto replan",   &g_View.bAutoReplan);
        ImGui::Checkbox("Auto rebuild",  &g_View.bAutoRebuild);        ImGui::SameLine();
        ImGui::Checkbox("Stats Window",  &g_ShowStatsWindow);

        if (ImGui::Button("Reset Camera (Home, 重置视角到包围盒)", ImVec2(-FLT_MIN, 0)))
            FrameCameraToBounds();

        ImGui::Spacing();
        ImGui::ColorEdit3("##pathcol", &g_PathColor.x,
                          ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_PickerHueWheel);
        ImGui::SameLine();
        ImGui::TextUnformatted("Path Color");

        ImGui::ColorEdit4("##corridorcol", &g_CorridorColor.x,
                          ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel |
                          ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_AlphaBar);
        ImGui::SameLine();
        ImGui::TextUnformatted("Corridor Color (RGBA)");

        if (g_ViewMode == ViewMode::Orbit3D)
        {
            ImGui::Separator();
            ImGui::TextUnformatted("Orbit Camera (Right-drag to rotate, wheel to zoom)");
            ImGui::SliderFloat("Yaw",      &g_Cam.Yaw,      -3.14f, 3.14f);
            ImGui::SliderFloat("Pitch",    &g_Cam.Pitch,    -1.50f, 1.50f);
            ImGui::SliderFloat("Distance", &g_Cam.Distance,  2.00f, 800.0f);
        }
    }

    void DrawScenePanel()
    {
        if (!ImGui::CollapsingHeader("Scene / 场景", ImGuiTreeNodeFlags_DefaultOpen)) return;

        if (g_Geom.Source != GeomSource::Procedural)
        {
            ImGui::TextDisabled(" (Scene size only applies in Procedural mode)");
            ImGui::TextDisabled(" Current source: OBJ file");
            return;
        }

        ImGui::TextDisabled(" 场景以原点为中心, 占地 (2 * HalfSize)^2 (单位: 米)");
        ImGui::PushItemWidth(-FLT_MIN);

        bool changed = false;

        float halfSize = g_Scene.HalfSize;
        if (ImGui::SliderFloat("Half Size", &halfSize, 5.0f, 200.0f, "%.1f m"))
        {
            g_Scene.HalfSize = halfSize;
            changed = true;
        }

        int cells = g_Scene.GridCells;
        if (ImGui::SliderInt("Grid Cells",  &cells, 4, 400))
        {
            g_Scene.GridCells = cells;
            changed = true;
        }

        // 当前几何信息 (只读)
        ImGui::TextDisabled(" 三角形数: %d  (%dx%d 网格 x 2)",
                            cells * cells * 2, cells, cells);
        ImGui::TextDisabled(" 网格步长: %.2f m  (越小精度越高, 构建越慢)",
                            (g_Scene.HalfSize * 2.0f) / std::max(1, cells));

        ImGui::PopItemWidth();

        // 一键预设
        auto Preset = [&](const char* label, float h, int c)
        {
            if (ImGui::SmallButton(label))
            {
                g_Scene.HalfSize = h;
                g_Scene.GridCells = c;
                changed = true;
            }
        };
        Preset("Tiny 10",   5.0f,   20);  ImGui::SameLine();
        Preset("Small 30", 15.0f,   30);  ImGui::SameLine();
        Preset("Medium 60",30.0f,   60);  ImGui::SameLine();
        Preset("Large 120",60.0f,  120);  ImGui::SameLine();
        Preset("Huge 300",150.0f,  300);

        if (changed)
        {
            // 修改场景尺寸: 把已存在障碍位置 clamp 进新边界, 起终点也 clamp
            ClampAllObstaclesToScene(g_Scene.HalfSize);
            const float maxAbs = std::max(0.0f, g_Scene.HalfSize - 1.0f);
            g_Start[0] = std::max(-maxAbs, std::min(maxAbs, g_Start[0]));
            g_Start[2] = std::max(-maxAbs, std::min(maxAbs, g_Start[2]));
            g_End  [0] = std::max(-maxAbs, std::min(maxAbs, g_End  [0]));
            g_End  [2] = std::max(-maxAbs, std::min(maxAbs, g_End  [2]));

            RebuildProceduralInputGeometry();
            FrameCameraToBounds();   // 新尺寸 -> 新包围盒 -> 重新对位 (覆盖 WASD 平移过的 target)
            g_bGeomDirty = true;
            TryAutoRebuild();
        }
    }

    void DrawEditPanel()
    {
        if (!ImGui::CollapsingHeader("Edit Mode", ImGuiTreeNodeFlags_DefaultOpen)) return;
        const char* items[] = { "None", "Place Start", "Place End", "Create Obstacle", "Select & Move", "Delete Obstacle" };
        int cur = static_cast<int>(g_EditMode);
        if (ImGui::Combo("##edit", &cur, items, IM_ARRAYSIZE(items))) g_EditMode = static_cast<EditMode>(cur);

        // 快捷键提示 (折叠头默认折叠)
        if (ImGui::TreeNode("Hotkeys / 快捷键"))
        {
            ImGui::TextDisabled("--- 场景导航 (3D) ---");
            ImGui::BulletText("W/A/S/D : 沿相机方向 前/左/后/右 平移");
            ImGui::BulletText("Q / E   : 下降 / 上升 (Y 轴)");
            ImGui::BulletText("Shift   : 加速 ×3   |   Ctrl: 慢速 ×0.3");
            ImGui::BulletText("右键拖拽: 旋转视角  |   滚轮: 缩放");
            ImGui::BulletText("Home    : 重置 Camera (拉到包围盒)");
            ImGui::TextDisabled("--- 编辑 / 操作 ---");
            ImGui::BulletText("1 / 2   : Place Start / End");
            ImGui::BulletText("T       : Select & Move (原 S, 让位 WASD)");
            ImGui::BulletText("B / C   : New Box / Cylinder");
            ImGui::BulletText("Del / X : 删除当前选中障碍");
            ImGui::BulletText("R       : (Re)Build NavMesh");
            ImGui::BulletText("F       : Find Path");
            ImGui::BulletText("V       : 切换 2D / 3D");
            ImGui::BulletText("Esc     : 取消选择 / 模式归 None");
            ImGui::TreePop();
        }

        if (g_Geom.Source == GeomSource::ObjFile &&
            (g_EditMode == EditMode::CreateBox || g_EditMode == EditMode::MoveBox || g_EditMode == EditMode::DeleteBox))
        {
            ImGui::TextColored(ImVec4(1, 0.6f, 0.4f, 1), "Obstacle editing only works in Procedural mode");
        }

        // 当前选择状态显示
        if (g_SelectedObstacle >= 0 && g_SelectedObstacle < (int)g_Geom.Obstacles.size())
        {
            const Obstacle& o = g_Geom.Obstacles[g_SelectedObstacle];
            ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.4f, 1.0f),
                "Selected: #%d [%s]  center=(%.2f, %.2f)  h=%.2f",
                g_SelectedObstacle,
                o.Shape == ObstacleShape::Box ? "Box" : "Cylinder",
                o.CX, o.CZ, o.Height);
        }
        else
        {
            ImGui::TextDisabled("Selected: (none)");
        }

        // 新建障碍属性
        const char* shapeItems[] = { "Box", "Cylinder" };
        int shapeIdx = static_cast<int>(g_DefaultObstacleShape);
        if (ImGui::Combo("New Shape", &shapeIdx, shapeItems, IM_ARRAYSIZE(shapeItems)))
            g_DefaultObstacleShape = static_cast<ObstacleShape>(shapeIdx);
        ImGui::SliderFloat("New Height", &g_DefaultObstacleHeight, 0.30f, 8.00f);
        ImGui::TextDisabled("Drag on canvas: Box=corner-to-corner, Cylinder=center-to-edge");

        ImGui::TextDisabled("Obstacles: %d", (int)g_Geom.Obstacles.size());
        if (ImGui::Button("Clear All Obstacles", ImVec2(-FLT_MIN, 0)))
        {
            if (g_Geom.Source == GeomSource::Procedural)
            {
                g_Geom.Obstacles.clear();
                RebuildProceduralInputGeometry();
                g_bGeomDirty = true;
                TryAutoRebuild();
            }
        }
        if (ImGui::Button("Reset Geometry", ImVec2(-FLT_MIN, 0)))
        {
            DestroyNavRuntime();
            InitDefaultGeometry();
            TryAutoRebuild();
        }

        // 障碍列表 (可调高度 / 删除)
        if (g_Geom.Source == GeomSource::Procedural && !g_Geom.Obstacles.empty())
        {
            ImGui::Spacing();
            ImGui::TextUnformatted("Obstacle list:");
            if (ImGui::BeginChild("##oblist", ImVec2(0, 160), true,
                                  ImGuiWindowFlags_HorizontalScrollbar))
            {
                int toDelete = -1;
                bool changed = false;
                for (size_t i = 0; i < g_Geom.Obstacles.size(); ++i)
                {
                    Obstacle& o = g_Geom.Obstacles[i];
                    ImGui::PushID(static_cast<int>(i));
                    const char* tag = (o.Shape == ObstacleShape::Box) ? "B" : "C";
                    if (o.Shape == ObstacleShape::Box)
                        ImGui::Text("%02d [%s] (%.1f,%.1f) sx=%.1f sz=%.1f",
                                    (int)i, tag, o.CX, o.CZ, o.SX, o.SZ);
                    else
                        ImGui::Text("%02d [%s] (%.1f,%.1f) r=%.2f",
                                    (int)i, tag, o.CX, o.CZ, o.Radius);
                    ImGui::SetNextItemWidth(140);
                    if (ImGui::SliderFloat("h", &o.Height, 0.20f, 8.00f, "%.2f"))
                        changed = true;
                    ImGui::SameLine();
                    if (ImGui::SmallButton("del")) toDelete = static_cast<int>(i);
                    ImGui::PopID();
                }
                if (toDelete >= 0)
                {
                    g_Geom.Obstacles.erase(g_Geom.Obstacles.begin() + toDelete);
                    changed = true;
                }
                if (changed)
                {
                    RebuildProceduralInputGeometry();
                    g_bGeomDirty = true;
                    TryAutoRebuild();
                }
            }
            ImGui::EndChild();
        }
    }

    void DrawBuildPanel()
    {
        if (!ImGui::CollapsingHeader("Build Config", ImGuiTreeNodeFlags_DefaultOpen)) return;

        // 每项配置前一行带中文描述, 便于理解每个参数的意义
        auto Desc = [](const char* zh) { ImGui::TextDisabled(" %s", zh); };

        ImGui::PushItemWidth(-FLT_MIN);

        Desc("体素水平尺寸 (XZ, 单位: 世界米) - 越小精度越高");
        ImGui::SliderFloat("Cell Size",         &g_Config.CellSize,         0.10f,  1.00f);

        Desc("体素垂直尺寸 (Y, 单位: 世界米)");
        ImGui::SliderFloat("Cell Height",       &g_Config.CellHeight,       0.10f,  1.00f);

        Desc("角色高度 (米) - 用于头顶净空判定");
        ImGui::SliderFloat("Agent Height",      &g_Config.AgentHeight,      0.50f,  5.00f);

        Desc("角色半径 (米) - 用于侵蚀, 路径绕开障碍的距离");
        ImGui::SliderFloat("Agent Radius",      &g_Config.AgentRadius,      0.00f,  3.00f);

        Desc("最大攀爬高度 (米) - 单步可上的台阶高度");
        ImGui::SliderFloat("Agent Max Climb",   &g_Config.AgentMaxClimb,    0.10f,  5.00f);

        Desc("最大可行走坡度 (度) - 大于此角度判定为不可走");
        ImGui::SliderFloat("Agent Max Slope",   &g_Config.AgentMaxSlope,    0.00f, 90.00f);

        Desc("区域最小尺寸 (cell^2) - 小于此值的孤立区域被丢弃");
        ImGui::SliderInt  ("Region Min Size",   &g_Config.RegionMinSize,    0,    150);

        Desc("区域合并阈值 (cell^2) - 小于此值的相邻区域会被合并");
        ImGui::SliderInt  ("Region Merge Size", &g_Config.RegionMergeSize,  0,    150);

        Desc("轮廓最大边长 (世界米) - 长边会被切分");
        ImGui::SliderFloat("Edge Max Len",      &g_Config.EdgeMaxLen,       0.0f,  50.0f);

        Desc("轮廓简化最大偏差 - 越大轮廓越粗糙");
        ImGui::SliderFloat("Edge Max Error",    &g_Config.EdgeMaxError,     0.10f,  3.00f);

        Desc("多边形最大顶点数 - 一般 6, 受 Detour 限制最大 12");
        ImGui::SliderInt  ("Verts Per Poly",    &g_Config.VertsPerPoly,     3,     12);

        ImGui::PopItemWidth();

        ImGui::Spacing();
        if (ImGui::Button("(Re)Build NavMesh", ImVec2(-FLT_MIN, 0)))
        {
            const bool ok = BuildNavMesh();
            LOG_F(INFO, "BuildNavMesh: %s (%s)", ok ? "OK" : "FAILED", g_BuildStatus.c_str());
            TryAutoReplan();
        }
        ImGui::Text("Built: %s | %s", g_Nav.bBuilt ? "Yes" : "No", g_BuildStatus.c_str());
        if (g_Nav.bBuilt && g_Nav.PolyMesh)
        {
            ImGui::Text("PolyMesh:   %d polys, %d verts",
                        g_Nav.PolyMesh->npolys, g_Nav.PolyMesh->nverts);
            if (g_Nav.PolyMeshDetail)
                ImGui::Text("DetailMesh: %d tris, %d verts",
                            g_Nav.PolyMeshDetail->ntris, g_Nav.PolyMeshDetail->nverts);
        }
        if (g_bGeomDirty)
            ImGui::TextColored(ImVec4(1, 0.7f, 0.2f, 1), "Geometry dirty - rebuild needed");
    }

    void DrawPathPanel()
    {
        if (!ImGui::CollapsingHeader("Path Query", ImGuiTreeNodeFlags_DefaultOpen)) return;
        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::DragFloat3("##Start", g_Start, 0.1f); ImGui::SameLine(); ImGui::TextUnformatted("Start");
        ImGui::DragFloat3("##End",   g_End,   0.1f); ImGui::SameLine(); ImGui::TextUnformatted("End");
        ImGui::PopItemWidth();
        if (ImGui::Button("Find Path", ImVec2(-FLT_MIN, 0)))
        {
            FindPath();
            LOG_F(INFO, "FindPath: %s", g_PathStatus.c_str());
        }
        ImGui::TextWrapped("Status: %s", g_PathStatus.c_str());
        if (!g_StraightPath.empty())
        {
            ImGui::Text("Straight corners: %d", static_cast<int>(g_StraightPath.size() / 3));
            ImGui::Text("Corridor polys:   %d", g_PathPolyCount);
        }
    }

    // ---- Native Open/Save 文件对话框 (Windows) ----
#ifdef _WIN32
    static bool ShowOpenFileDialog(const char* title, const char* filter,
                                   char* outPath, size_t outSize)
    {
        OPENFILENAMEA ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner   = GetActiveWindow();
        ofn.lpstrFilter = filter;        // "OBJ Files\0*.obj\0All Files\0*.*\0\0"
        ofn.lpstrFile   = outPath;
        ofn.nMaxFile    = static_cast<DWORD>(outSize);
        ofn.lpstrTitle  = title;
        ofn.Flags       = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
        return GetOpenFileNameA(&ofn) == TRUE;
    }
    static bool ShowSaveFileDialog(const char* title, const char* filter, const char* defExt,
                                   char* outPath, size_t outSize)
    {
        OPENFILENAMEA ofn{};
        ofn.lStructSize  = sizeof(ofn);
        ofn.hwndOwner    = GetActiveWindow();
        ofn.lpstrFilter  = filter;
        ofn.lpstrFile    = outPath;
        ofn.nMaxFile     = static_cast<DWORD>(outSize);
        ofn.lpstrTitle   = title;
        ofn.lpstrDefExt  = defExt;
        ofn.Flags        = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
        return GetSaveFileNameA(&ofn) == TRUE;
    }
#else
    static bool ShowOpenFileDialog(const char*, const char*, char* outPath, size_t outSize)
    { if (outSize) outPath[0] = '\0'; return false; }
    static bool ShowSaveFileDialog(const char*, const char*, const char*, char* outPath, size_t outSize)
    { if (outSize) outPath[0] = '\0'; return false; }
#endif

    // 把过长路径裁短显示, 保留尾部
    static std::string EllipsizePath(const char* p, size_t maxLen = 56)
    {
        if (!p || !*p) return "(none)";
        std::string s(p);
        if (s.size() <= maxLen) return s;
        return std::string("...") + s.substr(s.size() - (maxLen - 3));
    }

    void DrawIOPanel()
    {
        if (!ImGui::CollapsingHeader("Import / Export")) return;

        // ---- OBJ ----
        ImGui::TextUnformatted("OBJ geometry");
        if (ImGui::Button("Open OBJ...", ImVec2(120, 0)))
        {
            char buf[MAX_PATH] = {};
            if (g_ObjPathInput[0])
            {
                std::strncpy(buf, g_ObjPathInput, sizeof(buf) - 1);
                buf[sizeof(buf) - 1] = '\0';
            }
            if (ShowOpenFileDialog("Open OBJ Geometry",
                                   "OBJ Files (*.obj)\0*.obj\0All Files (*.*)\0*.*\0",
                                   buf, sizeof(buf)))
            {
                std::strncpy(g_ObjPathInput, buf, sizeof(g_ObjPathInput) - 1);
                g_ObjPathInput[sizeof(g_ObjPathInput) - 1] = '\0';

                std::string err;
                if (LoadObjGeometry(g_ObjPathInput, err))
                {
                    DestroyNavRuntime();
                    g_Geom.Obstacles.clear();          // OBJ 模式不复用 procedural 障碍
                    g_SelectedObstacle = -1;
                    g_MoveState.Index  = -1;
                    FrameCameraToBounds();             // 把相机拉到模型包围盒, 防止"看不到"
                    // Start/End 也按包围盒重定位, 否则旧的 (-10,0,-10)/(12,0,12) 大概率落在 OBJ 之外
                    const float* bmin = g_Geom.Bounds + 0;
                    const float* bmax = g_Geom.Bounds + 3;
                    const float cx = (bmin[0] + bmax[0]) * 0.5f;
                    const float cy = (bmin[1] + bmax[1]) * 0.5f;
                    const float cz = (bmin[2] + bmax[2]) * 0.5f;
                    const float dx = (bmax[0] - bmin[0]) * 0.25f;
                    const float dz = (bmax[2] - bmin[2]) * 0.25f;
                    g_Start[0] = cx - dx; g_Start[1] = cy; g_Start[2] = cz - dz;
                    g_End  [0] = cx + dx; g_End  [1] = cy; g_End  [2] = cz + dz;
                    LOG_F(INFO, "OBJ loaded: %s (%d verts, %d tris)", g_ObjPathInput,
                          (int)(g_Geom.Vertices.size() / 3), (int)(g_Geom.Triangles.size() / 3));
                    g_BuildStatus = "OBJ loaded: " + std::to_string(g_Geom.Vertices.size() / 3) + " verts, "
                                    + std::to_string(g_Geom.Triangles.size() / 3) + " tris";
                    TryAutoRebuild();
                }
                else
                {
                    LOG_F(ERROR, "OBJ load failed: %s (%s)", g_ObjPathInput, err.c_str());
                    g_BuildStatus = "OBJ load failed: " + err;
                }
            }
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%s", EllipsizePath(g_ObjPathInput).c_str());

        if (ImGui::Button("Use Procedural Geometry", ImVec2(-FLT_MIN, 0)))
        {
            DestroyNavRuntime();
            InitDefaultGeometry();
            FrameCameraToBounds();
            TryAutoRebuild();
        }

        ImGui::Separator();

        // ---- NavMesh binary ----
        ImGui::TextUnformatted("NavMesh binary (HNV1 format)");
        if (ImGui::Button("Save As...", ImVec2(120, 0)))
        {
            char buf[MAX_PATH] = {};
            std::strncpy(buf, g_NavSavePathInput[0] ? g_NavSavePathInput : "navmesh.bin",
                         sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';
            if (ShowSaveFileDialog("Save NavMesh",
                                   "NavMesh (*.bin)\0*.bin\0All Files (*.*)\0*.*\0",
                                   "bin", buf, sizeof(buf)))
            {
                std::strncpy(g_NavSavePathInput, buf, sizeof(g_NavSavePathInput) - 1);
                g_NavSavePathInput[sizeof(g_NavSavePathInput) - 1] = '\0';

                std::string err;
                if (SaveNavMesh(g_NavSavePathInput, err))
                    LOG_F(INFO, "NavMesh saved -> %s (%d bytes)", g_NavSavePathInput,
                          (int)g_Nav.NavMeshData.size());
                else
                {
                    LOG_F(ERROR, "Save failed: %s", err.c_str());
                    g_BuildStatus = "Save failed: " + err;
                }
            }
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%s", EllipsizePath(g_NavSavePathInput).c_str());

        if (ImGui::Button("Open NavMesh...", ImVec2(120, 0)))
        {
            char buf[MAX_PATH] = {};
            if (g_NavLoadPathInput[0])
            {
                std::strncpy(buf, g_NavLoadPathInput, sizeof(buf) - 1);
                buf[sizeof(buf) - 1] = '\0';
            }
            if (ShowOpenFileDialog("Open NavMesh Binary",
                                   "NavMesh (*.bin)\0*.bin\0All Files (*.*)\0*.*\0",
                                   buf, sizeof(buf)))
            {
                std::strncpy(g_NavLoadPathInput, buf, sizeof(g_NavLoadPathInput) - 1);
                g_NavLoadPathInput[sizeof(g_NavLoadPathInput) - 1] = '\0';

                std::string err;
                if (LoadNavMesh(g_NavLoadPathInput, err))
                {
                    LOG_F(INFO, "NavMesh loaded <- %s", g_NavLoadPathInput);
                    TryAutoReplan();
                }
                else
                {
                    LOG_F(ERROR, "Load failed: %s", err.c_str());
                    g_BuildStatus = "Load failed: " + err;
                }
            }
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%s", EllipsizePath(g_NavLoadPathInput).c_str());
    }

    // ---- 鼠标可拖分隔条 (ImGui 主线没内置 splitter, 自己实现) ----
    // 垂直分隔条: 横向拖动改变左右宽度; *width 是左侧宽度
    static bool VSplitter(const char* id, float* width, float minW, float maxW, float thickness = 6.0f)
    {
        ImGui::InvisibleButton(id, ImVec2(thickness, ImGui::GetContentRegionAvail().y));
        const bool active  = ImGui::IsItemActive();
        const bool hovered = ImGui::IsItemHovered();
        if (hovered || active) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        if (active)
        {
            *width += ImGui::GetIO().MouseDelta.x;
            *width = std::max(minW, std::min(maxW, *width));
        }
        ImDrawList* draw = ImGui::GetWindowDrawList();
        const ImVec2 a = ImGui::GetItemRectMin();
        const ImVec2 b = ImGui::GetItemRectMax();
        const ImU32 col = active   ? IM_COL32(120, 160, 220, 255)
                        : hovered  ? IM_COL32(100, 130, 180, 200)
                                   : IM_COL32( 70,  70,  80, 200);
        const float midX = (a.x + b.x) * 0.5f;
        draw->AddLine(ImVec2(midX, a.y + 4), ImVec2(midX, b.y - 4), col, 2.0f);
        return active;
    }

    // 垂直分隔条: 拖动改变 *右侧* 区域宽度 (拖右 -> 右窄左宽)
    static bool VSplitterRight(const char* id, float* rightWidth, float minW, float maxW, float thickness = 6.0f)
    {
        ImGui::InvisibleButton(id, ImVec2(thickness, ImGui::GetContentRegionAvail().y));
        const bool active  = ImGui::IsItemActive();
        const bool hovered = ImGui::IsItemHovered();
        if (hovered || active) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        if (active)
        {
            *rightWidth -= ImGui::GetIO().MouseDelta.x;
            *rightWidth = std::max(minW, std::min(maxW, *rightWidth));
        }
        ImDrawList* draw = ImGui::GetWindowDrawList();
        const ImVec2 a = ImGui::GetItemRectMin();
        const ImVec2 b = ImGui::GetItemRectMax();
        const ImU32 col = active   ? IM_COL32(120, 160, 220, 255)
                        : hovered  ? IM_COL32(100, 130, 180, 200)
                                   : IM_COL32( 70,  70,  80, 200);
        const float midX = (a.x + b.x) * 0.5f;
        draw->AddLine(ImVec2(midX, a.y + 4), ImVec2(midX, b.y - 4), col, 2.0f);
        return active;
    }

    // 水平分隔条: 纵向拖动改变上下高度; *height 是 *底部* 区域高度 (鼠标向上拖, 底部变大)
    static bool HSplitterBottom(const char* id, float* height, float minH, float maxH, float thickness = 6.0f)
    {
        ImGui::InvisibleButton(id, ImVec2(ImGui::GetContentRegionAvail().x, thickness));
        const bool active  = ImGui::IsItemActive();
        const bool hovered = ImGui::IsItemHovered();
        if (hovered || active) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        if (active)
        {
            *height -= ImGui::GetIO().MouseDelta.y;
            *height = std::max(minH, std::min(maxH, *height));
        }
        ImDrawList* draw = ImGui::GetWindowDrawList();
        const ImVec2 a = ImGui::GetItemRectMin();
        const ImVec2 b = ImGui::GetItemRectMax();
        const ImU32 col = active   ? IM_COL32(120, 160, 220, 255)
                        : hovered  ? IM_COL32(100, 130, 180, 200)
                                   : IM_COL32( 70,  70,  80, 200);
        const float midY = (a.y + b.y) * 0.5f;
        draw->AddLine(ImVec2(a.x + 4, midY), ImVec2(b.x - 4, midY), col, 2.0f);
        return active;
    }

    // ---- 多系列趋势图: 自绘 (ImGui 1.86 没 ImPlot, 也没 Docking) ----
    template <typename Stat>
    struct TSeries
    {
        const char* Name;
        ImU32       Color;
        double (*Get)(const Stat&);
    };

    // 构建阶段系列
    static const TSeries<BuildStat>* GetBuildSeriesArray()
    {
        static const TSeries<BuildStat> arr[] = {
            { "Total",           IM_COL32(255, 245, 120, 255), [](const BuildStat& b){ return b.T.TotalMs;        } },
            { "Rasterize",       IM_COL32( 80, 180, 255, 255), [](const BuildStat& b){ return b.T.RasterizeMs;    } },
            { "Filter",          IM_COL32(255, 130,  80, 255), [](const BuildStat& b){ return b.T.FilterMs;       } },
            { "CompactHF",       IM_COL32(120, 230, 130, 255), [](const BuildStat& b){ return b.T.CompactMs;      } },
            { "Erode",           IM_COL32(220, 110, 200, 255), [](const BuildStat& b){ return b.T.ErodeMs;        } },
            { "DistField",       IM_COL32(  0, 200, 200, 255), [](const BuildStat& b){ return b.T.DistFieldMs;    } },
            { "Regions",         IM_COL32(255,  90, 110, 255), [](const BuildStat& b){ return b.T.RegionsMs;      } },
            { "Contours",        IM_COL32(180, 160, 255, 255), [](const BuildStat& b){ return b.T.ContoursMs;     } },
            { "PolyMesh",        IM_COL32(255, 200,  60, 255), [](const BuildStat& b){ return b.T.PolyMeshMs;     } },
            { "DetailMesh",      IM_COL32(140, 220,  90, 255), [](const BuildStat& b){ return b.T.DetailMeshMs;   } },
            { "dtCreateNavMesh", IM_COL32(200, 200, 200, 255), [](const BuildStat& b){ return b.T.DetourCreateMs; } },
        };
        return arr;
    }
    static constexpr int kBuildSeriesCount = 11;

    // 寻路阶段系列
    static const TSeries<PathStat>* GetPathSeriesArray()
    {
        static const TSeries<PathStat> arr[] = {
            { "Total",            IM_COL32(255, 245, 120, 255), [](const PathStat& p){ return p.T.TotalMs;        } },
            { "FindNearestStart", IM_COL32( 80, 180, 255, 255), [](const PathStat& p){ return p.T.NearestStartMs; } },
            { "FindNearestEnd",   IM_COL32(255, 130,  80, 255), [](const PathStat& p){ return p.T.NearestEndMs;   } },
            { "FindPath",         IM_COL32(120, 230, 130, 255), [](const PathStat& p){ return p.T.FindPathMs;     } },
            { "StraightPath",     IM_COL32(220, 110, 200, 255), [](const PathStat& p){ return p.T.StraightPathMs; } },
        };
        return arr;
    }
    static constexpr int kPathSeriesCount = 5;

    // 主图绘制 (单图 + 自绘 + 可切图例) — 模板版本, build/path 共用
    template <typename Stat>
    static void DrawTrendChartT(const ImVec2& chartSize,
                                const std::deque<Stat>& history,
                                const TSeries<Stat>* series,
                                int seriesCount,
                                const bool* visible,
                                const char* emptyMsg,
                                const char* kindLabel /* "Build" or "Path" */,
                                void (*drawTooltipExtra)(const Stat&))
    {
        ImDrawList*   draw = ImGui::GetWindowDrawList();
        const ImVec2  p0   = ImGui::GetCursorScreenPos();
        const ImVec2  p1   = ImVec2(p0.x + chartSize.x, p0.y + chartSize.y);
        ImGui::InvisibleButton("##chartArea", chartSize);
        const bool    hov  = ImGui::IsItemHovered();
        const ImVec2  mp   = ImGui::GetIO().MousePos;

        // 背景 + 边框
        draw->AddRectFilled(p0, p1, IM_COL32(20, 20, 24, 255), 4.0f);
        draw->AddRect      (p0, p1, IM_COL32(80, 80, 90, 255), 4.0f);

        const int n = static_cast<int>(history.size());
        if (n == 0)
        {
            const ImVec2 ts = ImGui::CalcTextSize(emptyMsg);
            draw->AddText(ImVec2(p0.x + (chartSize.x - ts.x) * 0.5f,
                                 p0.y + (chartSize.y - ts.y) * 0.5f),
                          IM_COL32(160, 160, 160, 255), emptyMsg);
            return;
        }

        // 计算可见系列的全局 y 范围
        double yMax = 0.0;
        bool   anyVisible = false;
        for (int s = 0; s < seriesCount; ++s)
        {
            if (!visible[s]) continue;
            anyVisible = true;
            for (int i = 0; i < n; ++i)
                yMax = std::max(yMax, series[s].Get(history[i]));
        }
        if (!anyVisible)
        {
            const char* msg = "(all series hidden - click legend to show)";
            const ImVec2 ts = ImGui::CalcTextSize(msg);
            draw->AddText(ImVec2(p0.x + (chartSize.x - ts.x) * 0.5f,
                                 p0.y + (chartSize.y - ts.y) * 0.5f),
                          IM_COL32(160, 160, 160, 255), msg);
            return;
        }
        if (yMax <= 0.0) yMax = 1.0;
        yMax *= 1.08; // 顶部 8% padding

        constexpr float kAxisPadL = 44.0f;
        constexpr float kAxisPadB = 18.0f;
        constexpr float kAxisPadT =  6.0f;
        const ImVec2 plotMin(p0.x + kAxisPadL, p0.y + kAxisPadT);
        const ImVec2 plotMax(p1.x - 6.0f,      p1.y - kAxisPadB);
        const float  plotW = plotMax.x - plotMin.x;
        const float  plotH = plotMax.y - plotMin.y;

        // 网格 + y 轴标签
        for (int g = 0; g <= 4; ++g)
        {
            const float t = g / 4.0f;
            const float y = plotMax.y - t * plotH;
            draw->AddLine(ImVec2(plotMin.x, y), ImVec2(plotMax.x, y), IM_COL32(60, 60, 70, 255));
            char lbl[32];
            std::snprintf(lbl, sizeof(lbl), "%.2f", yMax * t);
            draw->AddText(ImVec2(p0.x + 4, y - 7), IM_COL32(150, 150, 160, 255), lbl);
        }

        // x 轴标签
        {
            char lbl[32];
            std::snprintf(lbl, sizeof(lbl), "#%d", history.front().Index);
            draw->AddText(ImVec2(plotMin.x, plotMax.y + 2), IM_COL32(150, 150, 160, 255), lbl);
            std::snprintf(lbl, sizeof(lbl), "#%d", history.back().Index);
            const ImVec2 ts = ImGui::CalcTextSize(lbl);
            draw->AddText(ImVec2(plotMax.x - ts.x, plotMax.y + 2), IM_COL32(150, 150, 160, 255), lbl);
        }

        auto mapXY = [&](int idx, double val) -> ImVec2
        {
            const float x = (n == 1)
                ? (plotMin.x + plotW * 0.5f)
                : (plotMin.x + plotW * (idx / float(n - 1)));
            const float y = plotMax.y - static_cast<float>(val / yMax) * plotH;
            return ImVec2(x, std::max(plotMin.y, std::min(plotMax.y, y)));
        };

        int hoverIdx = -1;
        if (hov && n > 1)
        {
            const float relX = (mp.x - plotMin.x) / plotW;
            hoverIdx = static_cast<int>(std::round(relX * (n - 1)));
            hoverIdx = std::max(0, std::min(n - 1, hoverIdx));
            const float hx = plotMin.x + plotW * (hoverIdx / float(n - 1));
            draw->AddLine(ImVec2(hx, plotMin.y), ImVec2(hx, plotMax.y),
                          IM_COL32(100, 100, 120, 180));
        }
        else if (hov && n == 1)
        {
            hoverIdx = 0;
        }

        // 折线
        for (int s = 0; s < seriesCount; ++s)
        {
            if (!visible[s]) continue;
            for (int i = 1; i < n; ++i)
            {
                const ImVec2 pa = mapXY(i - 1, series[s].Get(history[i - 1]));
                const ImVec2 pb = mapXY(i,     series[s].Get(history[i]));
                draw->AddLine(pa, pb, series[s].Color, 2.0f);
            }
            const ImVec2 pLast = mapXY(n - 1, series[s].Get(history[n - 1]));
            draw->AddCircleFilled(pLast, 3.0f, series[s].Color);
        }

        // tooltip
        if (hoverIdx >= 0)
        {
            const Stat& cur = history[hoverIdx];
            ImGui::BeginTooltip();
            ImGui::Text("%s #%d  |  Total %.2f ms", kindLabel, cur.Index, cur.T.TotalMs);
            ImGui::Separator();
            for (int s = 0; s < seriesCount; ++s)
            {
                if (!visible[s]) continue;
                const ImVec4 col = ImGui::ColorConvertU32ToFloat4(series[s].Color);
                ImGui::ColorButton("##sw", col,
                                   ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder,
                                   ImVec2(10, 10));
                ImGui::SameLine();
                ImGui::Text("%-18s %8.3f ms", series[s].Name, series[s].Get(cur));
            }
            if (drawTooltipExtra) drawTooltipExtra(cur);
            ImGui::EndTooltip();
        }
    }

    // 图例 (clickable items, 切换 visible) — 不依赖具体 Stat 类型
    static void DrawLegendItemRaw(int id, bool& visible,
                                  const char* name, ImU32 color, float lastVal,
                                  const char* unit = " ms")
    {
        const float lh = ImGui::GetTextLineHeightWithSpacing();

        char buf[96];
        std::snprintf(buf, sizeof(buf), "%-17s %7.2f%s", name, lastVal, unit);

        const ImVec2 textSize = ImGui::CalcTextSize(buf);
        const ImVec2 itemSize(textSize.x + 22.0f, lh);

        const ImVec2  cur  = ImGui::GetCursorScreenPos();
        ImGui::PushID(id);
        ImGui::InvisibleButton("##leg", itemSize);
        const bool clicked = ImGui::IsItemClicked();
        const bool hovered = ImGui::IsItemHovered();
        ImGui::PopID();

        if (clicked) visible = !visible;

        ImDrawList* draw = ImGui::GetWindowDrawList();
        const ImVec2 boxA(cur.x + 2,  cur.y + (lh - 12) * 0.5f);
        const ImVec2 boxB(cur.x + 14, cur.y + (lh - 12) * 0.5f + 12);
        if (visible)
            draw->AddRectFilled(boxA, boxB, color, 2.0f);
        else
            draw->AddRect(boxA, boxB, (color & 0x00FFFFFF) | 0x80000000, 2.0f, 0, 1.5f);
        const ImU32 tc = visible ? IM_COL32(230, 230, 235, 255)
                                  : IM_COL32(140, 140, 145, 255);
        draw->AddText(ImVec2(cur.x + 18, cur.y + 1), tc, buf);
        if (hovered)
            draw->AddRect(ImVec2(cur.x, cur.y), ImVec2(cur.x + itemSize.x, cur.y + itemSize.y),
                          IM_COL32(120, 120, 140, 255), 2.0f);
    }

    // ---- tooltip extra: build / path ----
    static void BuildTooltipExtra(const BuildStat& bs)
    {
        ImGui::Separator();
        ImGui::Text("Polys=%d  DetailTris=%d  NavData=%.1f KB",
                    bs.PolyCount, bs.DetailTris, bs.NavDataBytes / 1024.0f);
    }
    static void PathTooltipExtra(const PathStat& ps)
    {
        ImGui::Separator();
        ImGui::Text("Status: %s", ps.Status.c_str());
        ImGui::Text("Corridor=%d polys   Straight=%d corners",
                    ps.CorridorPolys, ps.StraightCorners);
    }

    // ---- 单 tab 内部: chart + splitter + legend (公共布局) ----
    template <typename Stat>
    static void DrawStatsTabBody(const std::deque<Stat>& history,
                                 const TSeries<Stat>* series,
                                 int seriesCount,
                                 bool* visible,
                                 const char* emptyMsg,
                                 const char* kindLabel,
                                 void (*drawTooltipExtra)(const Stat&),
                                 const char* sideHeader,
                                 std::function<void(const Stat&)> drawSidePanel,
                                 const char* splitterId)
    {
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const float kSplitterW = 6.0f;
        const float maxLegendW = std::max(120.0f, avail.x - 200.0f - kSplitterW);
        g_StatsLegendWidth = std::max(120.0f, std::min(maxLegendW, g_StatsLegendWidth));
        const ImVec2 chartSize(avail.x - g_StatsLegendWidth - kSplitterW, avail.y);

        DrawTrendChartT<Stat>(chartSize, history, series, seriesCount, visible,
                              emptyMsg, kindLabel, drawTooltipExtra);

        ImGui::SameLine(0.0f, 0.0f);
        VSplitterRight(splitterId, &g_StatsLegendWidth, 120.0f, maxLegendW, kSplitterW);
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::BeginChild("##legend", ImVec2(g_StatsLegendWidth, avail.y), false);
        ImGui::TextDisabled("Legend (click to toggle)");
        ImGui::Separator();
        for (int s = 0; s < seriesCount; ++s)
        {
            const float lastVal = history.empty()
                ? 0.0f
                : static_cast<float>(series[s].Get(history.back()));
            DrawLegendItemRaw(s, visible[s], series[s].Name, series[s].Color, lastVal);
        }
        if (sideHeader && drawSidePanel)
        {
            ImGui::Spacing();
            ImGui::TextDisabled("%s", sideHeader);
            ImGui::Separator();
            if (!history.empty()) drawSidePanel(history.back());
        }
        ImGui::EndChild();
    }

    // ---- 浮动窗口: NavMesh / Path 性能趋势图 ----
    void DrawBuildStatsWindow()
    {
        if (!g_ShowStatsWindow) return;

        // 各 tab 系列可见状态 (持久化)
        static bool sBuildVisible[kBuildSeriesCount] = {
            true, true, true, true, true, true, true, true, true, true, true
        };
        static bool sPathVisible[kPathSeriesCount] = { true, true, true, true, true };

        const ImGuiViewport* vp = ImGui::GetMainViewport();

        // 永久贴主视口底部: 全宽 + 固定高 (高度由顶部水平 splitter 拖动)
        const float h = g_StatsDockHeight;
        ImGui::SetNextWindowPos (ImVec2(vp->WorkPos.x, vp->WorkPos.y + vp->WorkSize.y - h));
        ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x, h));
        constexpr ImGuiWindowFlags wflags =
              ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoBringToFrontOnFocus;

        if (!ImGui::Begin("Performance Stats / 性能趋势", &g_ShowStatsWindow, wflags))
        {
            ImGui::End();
            return;
        }

        // 顶部水平 splitter
        {
            const float maxDockH = std::max(200.0f, vp->WorkSize.y - 200.0f);
            HSplitterBottom("##split_dock", &g_StatsDockHeight, 160.0f, maxDockH, 6.0f);
        }

        if (ImGui::BeginTabBar("##stats_tabs"))
        {
            // ===== Tab 1: NavMesh Build =====
            if (ImGui::BeginTabItem("NavMesh Build / 构建"))
            {
                // 顶栏: 摘要 + 控制
                const int n = static_cast<int>(g_BuildHistory.size());
                if (n > 0)
                {
                    const BuildStat& last = g_BuildHistory.back();
                    ImGui::Text("Builds: %d   Last #%d   Total: %.2f ms   Polys: %d   NavData: %.1f KB",
                                n, last.Index, last.T.TotalMs, last.PolyCount,
                                last.NavDataBytes / 1024.0f);
                }
                else
                {
                    ImGui::TextUnformatted("(no builds yet - press R or click Build)");
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("All##b"))  { for (bool& v : sBuildVisible) v = true; }
                ImGui::SameLine();
                if (ImGui::SmallButton("None##b")) { for (bool& v : sBuildVisible) v = false; }
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60);
                if (ImGui::SmallButton("Clear##b"))
                {
                    g_BuildHistory.clear();
                    g_BuildCounter = 0;
                }
                ImGui::Separator();

                auto buildSidePanel = [](const BuildStat& last)
                {
                    ImGui::Text("Polys     : %d", last.PolyCount);
                    ImGui::Text("PolyVerts : %d", last.PolyVerts);
                    ImGui::Text("DetailTris: %d", last.DetailTris);
                    ImGui::Text("NavData   : %.1f KB", last.NavDataBytes / 1024.0f);
                    ImGui::Text("InputTris : %d", last.InputTris);
                };
                DrawStatsTabBody<BuildStat>(g_BuildHistory,
                                            GetBuildSeriesArray(), kBuildSeriesCount,
                                            sBuildVisible,
                                            "(no builds yet - press R or click Build)",
                                            "Build",
                                            &BuildTooltipExtra,
                                            "Output sizes (last)",
                                            buildSidePanel,
                                            "##split_legend_b");
                ImGui::EndTabItem();
            }

            // ===== Tab 2: Path Query =====
            if (ImGui::BeginTabItem("Path Query / 寻路"))
            {
                const int n = static_cast<int>(g_PathHistory.size());
                if (n > 0)
                {
                    const PathStat& last = g_PathHistory.back();
                    ImGui::Text("Queries: %d   Last #%d   Total: %.3f ms   %s",
                                n, last.Index, last.T.TotalMs,
                                last.bOk ? "OK" : "FAIL");
                }
                else
                {
                    ImGui::TextUnformatted("(no path queries yet - press F or click Find Path)");
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("All##p"))  { for (bool& v : sPathVisible) v = true; }
                ImGui::SameLine();
                if (ImGui::SmallButton("None##p")) { for (bool& v : sPathVisible) v = false; }
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60);
                if (ImGui::SmallButton("Clear##p"))
                {
                    g_PathHistory.clear();
                    g_PathCounter = 0;
                }
                ImGui::Separator();

                auto pathSidePanel = [](const PathStat& last)
                {
                    ImGui::Text("Status     : %s", last.Status.c_str());
                    ImGui::Text("Corridor   : %d polys", last.CorridorPolys);
                    ImGui::Text("Straight   : %d corners", last.StraightCorners);
                    ImGui::Text("Result     : %s", last.bOk ? "OK" : "FAIL");
                };
                DrawStatsTabBody<PathStat>(g_PathHistory,
                                           GetPathSeriesArray(), kPathSeriesCount,
                                           sPathVisible,
                                           "(no path queries yet - press F or click Find Path)",
                                           "Path",
                                           &PathTooltipExtra,
                                           "Last query",
                                           pathSidePanel,
                                           "##split_legend_p");
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    void DrawStatsPanel()
    {
        if (!ImGui::CollapsingHeader("Stats & Log")) return;
        ImGui::Text("Build phases (ms):");
        ImGui::BeginTable("##timings", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH);
        auto Row = [](const char* k, double v)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(k);
            ImGui::TableNextColumn(); ImGui::Text("%.3f", v);
        };
        Row("Rasterize",      g_Timings.RasterizeMs);
        Row("Filter",         g_Timings.FilterMs);
        Row("CompactHF",      g_Timings.CompactMs);
        Row("Erode",          g_Timings.ErodeMs);
        Row("DistanceField",  g_Timings.DistFieldMs);
        Row("Regions",        g_Timings.RegionsMs);
        Row("Contours",       g_Timings.ContoursMs);
        Row("PolyMesh",       g_Timings.PolyMeshMs);
        Row("DetailMesh",     g_Timings.DetailMeshMs);
        Row("dtCreateNavMesh",g_Timings.DetourCreateMs);
        Row("Total",          g_Timings.TotalMs);
        ImGui::EndTable();

        ImGui::Spacing();
        ImGui::Text("rcContext log (%d lines):", (int)g_Nav.Ctx.LogLines.size());
        if (ImGui::Button("Clear", ImVec2(80, 0))) g_Nav.Ctx.LogLines.clear();
        ImGui::BeginChild("##rclog", ImVec2(0, 160), true,
                          ImGuiWindowFlags_HorizontalScrollbar);
        for (const std::string& line : g_Nav.Ctx.LogLines)
            ImGui::TextUnformatted(line.c_str());
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 20.0f)
            ImGui::SetScrollHereY(1.0f);
        ImGui::EndChild();
    }

    // =================================================================================
    // 13. OnGUI
    // =================================================================================
    void OnGUI(float /*DeltaTime*/)
    {
        HandleHotkeys();

        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImVec2 mainPos  = vp->WorkPos;
        ImVec2 mainSize = vp->WorkSize;
        if (g_ShowStatsWindow)
            mainSize.y = std::max(120.0f, mainSize.y - g_StatsDockHeight);
        ImGui::SetNextWindowPos(mainPos);
        ImGui::SetNextWindowSize(mainSize);

        constexpr ImGuiWindowFlags kMainFlags =
            ImGuiWindowFlags_NoTitleBar            |
            ImGuiWindowFlags_NoResize              |
            ImGuiWindowFlags_NoMove                |
            ImGuiWindowFlags_NoCollapse            |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus            |
            ImGuiWindowFlags_NoSavedSettings;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));

        if (ImGui::Begin("##RecastNaviDemoMain", nullptr, kMainFlags))
        {
            // 主窗口可用宽度 (留一点 splitter 余量), 用作拖拽 clamp 上限
            const float kMainAvailW = ImGui::GetContentRegionAvail().x;
            const float kSplitterW  = 6.0f;
            const float kMaxLeft    = std::max(120.0f, kMainAvailW - 200.0f - kSplitterW);
            g_LeftPaneWidth = std::max(180.0f, std::min(kMaxLeft, g_LeftPaneWidth));

            if (ImGui::BeginChild("##LeftPane",
                                  ImVec2(g_LeftPaneWidth, 0.0f),
                                  true,
                                  ImGuiWindowFlags_NoSavedSettings))
            {
                ImGui::TextUnformatted("RecastNavigation Demo");
                ImGui::SameLine();
                ImGui::TextDisabled(g_Geom.Source == GeomSource::Procedural
                                     ? " [procedural]" : " [obj]");
                ImGui::Separator();

                DrawViewPanel();
                ImGui::Separator();
                DrawScenePanel();
                ImGui::Separator();
                DrawEditPanel();
                ImGui::Separator();
                DrawBuildPanel();
                ImGui::Separator();
                DrawPathPanel();
                ImGui::Separator();
                DrawIOPanel();
                ImGui::Separator();
                DrawStatsPanel();
            }
            ImGui::EndChild();

            ImGui::SameLine(0.0f, 0.0f);
            VSplitter("##split_main", &g_LeftPaneWidth, 180.0f, kMaxLeft, kSplitterW);
            ImGui::SameLine(0.0f, 0.0f);

            if (ImGui::BeginChild("##RightPane",
                                  ImVec2(0.0f, 0.0f),
                                  true,
                                  ImGuiWindowFlags_NoSavedSettings))
            {
                // 顶部一行: 状态栏 / 图例
                const char* viewName = (g_ViewMode == ViewMode::TopDown2D) ? "2D Top" : "3D Orbit";
                const char* editName[] = { "None","PlaceStart","PlaceEnd","CreateObstacle","Select&Move","Delete" };
                ImGui::Text("View: %s   Edit: %s", viewName, editName[(int)g_EditMode]);
                ImGui::SameLine(); ImGui::TextDisabled("  |  ");
                ImGui::SameLine(); ImGui::TextColored(ImVec4(0.71f, 0.31f, 0.31f, 1), "Obstacle");
                ImGui::SameLine(); ImGui::TextColored(ImVec4(0.31f, 0.78f, 0.39f, 1), "NavMesh");
                ImGui::SameLine(); ImGui::TextColored(ImVec4(0.31f, 0.63f, 1.00f, 1), "Start");
                ImGui::SameLine(); ImGui::TextColored(ImVec4(1.00f, 0.31f, 0.31f, 1), "End");
                ImGui::SameLine(); ImGui::TextColored(ImVec4(1.00f, 0.86f, 0.24f, 1), "Path");
                ImGui::Separator();

                DrawCanvasPanel();
            }
            ImGui::EndChild();
        }
        ImGui::End();

        ImGui::PopStyleVar(3);

        // 浮动趋势图窗口 (放在主窗口 End 之后, 让它独立可拖拽)
        DrawBuildStatsWindow();
    }

    void OnTick(float)    {}
    void OnPostGUI(float) {}
    void OnInput(float)   {}
}

int main(int argc, char** argv)
{
    loguru::init(argc, argv);
    loguru::add_file(__FILE__ ".log", loguru::Append, loguru::Verbosity_MAX);
    loguru::g_stderr_verbosity = 1;

    LOG_F(INFO, "Entry of RecastNaviDemo");

    InitDefaultGeometry();

    const int rc = DirectX::CreateWindowInstance(
        "RecastNavigation Demo",
        1400, 900,
        0, 0,
        OnTick, OnGUI, OnPostGUI, OnInput);

    DestroyNavRuntime();
    return rc;
}
