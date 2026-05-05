#pragma once
/*
 * Shared/Profiling.h
 * ------------------
 * 性能采样工具：ScopedTimer、各阶段耗时结构体、构建/寻路快照。
 *
 * 设计原则：
 *   - 仅头文件，无平台依赖
 *   - PhaseTimings / BuildStat / PathTimings / PathStat 同时供 Nav/ 和 UI/ 使用
 */

#include <chrono>
#include <string>

// =============================================================================
// 范围耗时辅助 — RAII 计时器
// =============================================================================
struct ScopedTimer
{
    std::chrono::high_resolution_clock::time_point t0;
    double* out;

    explicit ScopedTimer(double* p)
        : t0(std::chrono::high_resolution_clock::now()), out(p) {}

    ~ScopedTimer()
    {
        const auto t1 = std::chrono::high_resolution_clock::now();
        *out = std::chrono::duration<double, std::milli>(t1 - t0).count();
    }
};

// =============================================================================
// NavMesh 构建阶段耗时
// =============================================================================
struct PhaseTimings
{
    double TotalMs        = 0.0;   ///< 整体构建耗时
    double RasterizeMs    = 0.0;   ///< Step2: 光栅化三角形
    double FilterMs       = 0.0;   ///< Step3: 过滤可行走区域
    double CompactMs      = 0.0;   ///< Step4: 构建紧凑高度场
    double ErodeMs        = 0.0;   ///< Step5: 腐蚀可行走区域
    double DistFieldMs    = 0.0;   ///< Step5b: 距离场
    double RegionsMs      = 0.0;   ///< Step6: 构建区域
    double ContoursMs     = 0.0;   ///< Step7: 构建轮廓集
    double PolyMeshMs     = 0.0;   ///< Step8: 构建多边形网格
    double DetailMeshMs   = 0.0;   ///< Step9: 构建细节网格
    double DetourCreateMs = 0.0;   ///< Step10: 创建 Detour NavMesh
};

// =============================================================================
// 单次构建快照（用于趋势图）
// =============================================================================
struct BuildStat
{
    int          Index        = 0;     ///< 第几次构建（1-based）
    PhaseTimings T            {};
    int          InputVerts   = 0;
    int          InputTris    = 0;
    int          PolyCount    = 0;
    int          PolyVerts    = 0;
    int          DetailTris   = 0;
    int          NavDataBytes = 0;
    bool         bOk          = false;
};

// =============================================================================
// 寻路阶段耗时
// =============================================================================
struct PathTimings
{
    double TotalMs        = 0.0;
    double NearestStartMs = 0.0;   ///< findNearestPoly (start)
    double NearestEndMs   = 0.0;   ///< findNearestPoly (end)
    double FindPathMs     = 0.0;   ///< findPath (corridor)
    double StraightPathMs = 0.0;   ///< findStraightPath (waypoints)
};

// =============================================================================
// 单次寻路快照（用于趋势图）
// =============================================================================
struct PathStat
{
    int         Index           = 0;
    PathTimings T               {};
    int         CorridorPolys   = 0;
    int         StraightCorners = 0;
    bool        bOk             = false;
    std::string Status;
};
