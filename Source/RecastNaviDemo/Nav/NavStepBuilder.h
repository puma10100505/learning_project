#pragma once
/*
 * Nav/NavStepBuilder.h
 * --------------------
 * NavMesh 分步骤构建（10 步流水线），用于教学/演示：
 *   - StepForward / StepBack / RunAll / Reset 控制构建进度
 *   - 各步骤完成后保留中间数据 (rcHeightfield / rcCompactHeightfield / rcContourSet ...)
 *     供 Render/NavStepDebug 进行可视化叠绘
 *   - "回退" 通过释放当前所有中间数据并重放前 N-1 步实现（语义最简单）
 *
 * 构建流程与 NavBuilder::BuildNavMesh 完全一致（仅结构上拆分），
 * 因此最终结果 (NavRuntime.NavMesh / PolyMesh / DetailMesh) 与一键构建相同。
 *
 * 依赖：Nav/NavTypes.h、Shared/Profiling.h
 */

#include "NavTypes.h"
#include "../Shared/Profiling.h"

#include <string>
#include <vector>

namespace NavStepBuilder
{

// =============================================================================
// 步骤枚举（1..10 与 Recast 经典 10 步流水线一一对应；0 = 未开始）
// =============================================================================
enum class Step : int
{
    None       = 0,   ///< 尚未开始
    Config     = 1,   ///< Step 1: 配置 rcConfig（不分配资源）
    Rasterize  = 2,   ///< Step 2: 创建 rcHeightfield 并光栅化三角形
    Filter     = 3,   ///< Step 3: 过滤可走性（低悬挂 / 台阶 / 低空间）
    CompactHF  = 4,   ///< Step 4: 紧凑高度场（释放 solid）
    Erode      = 5,   ///< Step 5: 腐蚀 + 距离场
    Regions    = 6,   ///< Step 6: 区域分割
    Contours   = 7,   ///< Step 7: 轮廓集
    PolyMesh   = 8,   ///< Step 8: 多边形网格
    DetailMesh = 9,   ///< Step 9: 细节网格（之后释放 chf）
    DetourNav  = 10,  ///< Step 10: dtNavMesh + dtNavMeshQuery
};

constexpr int kStepCount = 10;

/// 步骤短名（"Config" / "Rasterize" / ...）
const char* GetStepName(Step s);

/// 步骤中文说明（一句话，用于面板提示）
const char* GetStepDescription(Step s);

// =============================================================================
// StepBuilder — 分步骤构建器（拥有所有中间数据）
// =============================================================================
struct StepBuilder
{
    // 输入快照（在 Reset 时拷贝，整个 Step 会话内只读，避免边界外修改）
    InputGeometry  GeomSnap;
    NavBuildConfig ConfigSnap;
    BuildVolume    BVSnap;

    // Recast 配置（Step1 后填充）
    rcConfig Cfg{};

    // 中间数据（生命周期由 StepBuilder 管理）
    rcHeightfield*        Solid = nullptr;  ///< Step2 创建，Step4 完成后释放
    rcCompactHeightfield* Chf   = nullptr;  ///< Step4 创建，Step9 完成后释放
    rcContourSet*         Cset  = nullptr;  ///< Step7 创建，Step8 完成后释放
    rcPolyMesh*           Pmesh = nullptr;  ///< Step8 创建，Step10 完成时移交 NavRuntime
    rcPolyMeshDetail*     Dmesh = nullptr;  ///< Step9 创建，Step10 完成时移交 NavRuntime

    // 进度
    Step LastCompleted = Step::None;        ///< 最近一次完成的步骤
    bool bFailed       = false;             ///< 上次操作是否失败
    std::string FailMsg;                    ///< 失败原因（用于 UI 显示）

    // 各步耗时（mirror 到 NavRuntime 上的 PhaseTimings）
    PhaseTimings Timings;

    // Recast log 抓取
    CapturedRcContext Ctx;
};

// =============================================================================
// 公开 API
// =============================================================================

/// 是否处于活动会话（已开始或仍持有中间数据）
bool IsActive(const StepBuilder& sb);

/// 当前可前进的下一步；若已完成全部则返回 Step::None（注意 None 既可表示"未开始"）
Step PeekNextStep(const StepBuilder& sb);

/**
 * @brief 开始一个新的分步骤构建会话。
 *
 * 行为：
 *   - 先调用 NavBuilder::DestroyNavRuntime 销毁旧 NavMesh
 *   - 释放所有中间数据
 *   - 拷贝输入到 GeomSnap/ConfigSnap/BVSnap
 *   - LastCompleted = None
 *
 * 注意：TileCache 模式（ConfigSnap.bUseTileCache==true）暂不支持分步骤；
 *       Reset 仍会清理旧数据但 Forward 会立即报错。
 */
void Reset(StepBuilder&            sb,
           NavRuntime&             runtime,
           const InputGeometry&    geom,
           const NavBuildConfig&   cfg,
           const BuildVolume&      bv);

/// 释放 StepBuilder 自身持有的所有中间数据（不影响 NavRuntime）
void Clear(StepBuilder& sb);

/**
 * @brief 前进一步。若没有可前进步骤或上一步已失败则返回 false。
 * @param extraLinks 仅在执行 Step10 时使用
 */
bool Forward(StepBuilder&                       sb,
             NavRuntime&                        runtime,
             const std::vector<OffMeshLink>*    extraLinks = nullptr);

/**
 * @brief 回退到上一个步骤（即 LastCompleted - 1）。通过 Clear + 重放前 N-1 步实现。
 *        若当前已是 None 则返回 false。
 */
bool Back(StepBuilder&                          sb,
          NavRuntime&                           runtime,
          const std::vector<OffMeshLink>*       extraLinks = nullptr);

/// 一直前进到 Step10（中途失败立即停下）
bool RunAll(StepBuilder&                        sb,
            NavRuntime&                         runtime,
            const std::vector<OffMeshLink>*     extraLinks = nullptr);

} // namespace NavStepBuilder
