/*
 * RecastNaviDemo.cpp
 * ------------------
 * 程序入口：仅保留 main()、AppState 单例定义和 FrameCameraToBounds 辅助。
 * 所有 Nav / UI / Render 逻辑已分别提取到对应子模块。
 *
 * P1-P9e 重构结果：
 *   Nav/NavBuilder      — Recast/Detour 构建管线
 *   Nav/NavQuery        — Detour 寻路查询
 *   Nav/NavGeometry     — 程序化几何 / OBJ 导入
 *   Nav/NavSerializer   — NavMesh 二进制保存 / 加载
 *   UI/MainLayout       — 顶级 ImGui 布局
 *   UI/Panels           — 左侧各折叠面板
 *   UI/Interaction      — 键鼠交互
 *   Render/Renderer2D   — 2D 软件光栅化
 *   Render/Renderer3D   — 3D 软件投影
 *   App/AppState        — 全局状态结构体
 */

// ---------------------------------------------------------------------------
// 平台 include
// ---------------------------------------------------------------------------
#ifdef __linux__
#include <unistd.h>
#endif

#if !defined(_WIN32)
#  include <climits>
#  ifndef MAX_PATH
#    if defined(PATH_MAX)
#      define MAX_PATH PATH_MAX
#    else
#      define MAX_PATH 4096
#    endif
#  endif
#endif

#ifdef _WIN32
#  define NOMINMAX
#  include <windows.h>
#  include <commdlg.h>
#  pragma comment(lib, "comdlg32.lib")
#  include "DirectX12Window.h"
#else
#  include "GlfwWindows.h"
#  include <GLFW/glfw3.h>
#endif

// ---------------------------------------------------------------------------
// 公共 include
// ---------------------------------------------------------------------------
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>

#include "loguru.hpp"

#include "App/AppState.h"
#include "Nav/NavBuilder.h"
#include "Nav/NavGeometry.h"
#include "Nav/NavQuery.h"
#include "Nav/NavPathSmooth.h"
#include "Nav/NavLinkGen.h"
#include "Nav/NavSerializer.h"
#include "Phys/PhysWorld.h"
#include "UI/MainLayout.h"
#include "Shared/Profiling.h"   // BuildStat / PathStat
#include "Shared/MathUtils.h"   // V3

// ---------------------------------------------------------------------------
// AppState 单例定义（AppState.h 中只有声明）
// ---------------------------------------------------------------------------
static AppState g_State;
AppState& GetAppState() { return g_State; }

// ---------------------------------------------------------------------------
// FrameCameraToBounds — 将轨道相机对准当前几何包围盒
// ---------------------------------------------------------------------------
static void FrameCameraToBounds()
{
    const float* bmin = g_State.Geom.Bounds + 0;
    const float* bmax = g_State.Geom.Bounds + 3;
    const float dx = bmax[0] - bmin[0];
    const float dy = bmax[1] - bmin[1];
    const float dz = bmax[2] - bmin[2];
    if (dx <= 0.0f || dz <= 0.0f) return;

    g_State.Cam.Target = V3((bmin[0] + bmax[0]) * 0.5f,
                             (bmin[1] + bmax[1]) * 0.5f,
                             (bmin[2] + bmax[2]) * 0.5f);
    const float diag = std::sqrt(dx * dx + dy * dy + dz * dz);
    const float fitH = (diag * 0.5f) / std::tan(g_State.Cam.Fovy * 0.5f);
    const float fitD = std::max(diag * 0.6f, fitH * 1.15f);
    g_State.Cam.Distance = std::max(2.0f, std::min(800.0f, fitD));
    g_State.Cam.Yaw   = 0.7f;
    g_State.Cam.Pitch = 0.9f;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char** argv)
{
    loguru::init(argc, argv);
    loguru::g_stderr_verbosity = 1;
    if (!loguru::add_file(__FILE__ ".log", loguru::Append, loguru::Verbosity_MAX))
    {
        const std::filesystem::path fallback =
            std::filesystem::current_path() / "RecastNaviDemo.runtime.log";
        const std::string fallbackUtf8 = fallback.string();
        if (loguru::add_file(fallbackUtf8.c_str(), loguru::Append, loguru::Verbosity_MAX))
            LOG_F(WARNING, "Primary log file unavailable, fallback to: %s", fallbackUtf8.c_str());
        else
            LOG_F(ERROR, "Both primary and fallback log file setup failed");
    }
    LOG_F(INFO, "Entry of RecastNaviDemo");

    // -----------------------------------------------------------------------
    // 构建所有回调
    // -----------------------------------------------------------------------
    MainLayoutCallbacks cbs;

    // -- BuildNavMesh --
    auto doBuildNavMesh = [&]()
    {
        // 若已有自动生成的 NavLink，将其一并烘焙进 NavMesh
        const std::vector<OffMeshLink>* extraLinks =
            g_State.AutoNavLinkCfg.bEnabled && !g_State.AutoNavLinks.empty()
                ? &g_State.AutoNavLinks : nullptr;

        bool ok = NavBuilder::BuildNavMesh(g_State.Geom, g_State.Config,
                                            g_State.BV, g_State.Nav, g_State.Timings,
                                            extraLinks);
        g_State.BuildStatus = ok ? "OK" : "build failed";
        if (ok)
        {
            g_State.bGeomDirty = false;

            // TileCache 模式：将场景中的障碍列表同步进 TileCache
            if (g_State.Nav.bTileMode)
            {
                NavBuilder::SyncObstaclesToTileCache(g_State.Geom, g_State.Nav,
                                                     g_State.TileCacheObsRefs);
            }

            BuildStat snap;
            snap.Index        = ++g_State.BuildCounter;
            snap.T            = g_State.Timings;
            snap.InputVerts   = static_cast<int>(g_State.Geom.Vertices.size()  / 3);
            snap.InputTris    = static_cast<int>(g_State.Geom.Triangles.size() / 3);
            snap.PolyCount    = g_State.Nav.PolyMesh       ? g_State.Nav.PolyMesh->npolys       : 0;
            snap.PolyVerts    = g_State.Nav.PolyMesh       ? g_State.Nav.PolyMesh->nverts        : 0;
            snap.DetailTris   = g_State.Nav.PolyMeshDetail ? g_State.Nav.PolyMeshDetail->ntris   : 0;
            snap.NavDataBytes = static_cast<int>(g_State.Nav.NavMeshData.size());
            snap.bOk          = true;
            g_State.BuildHistory.push_back(snap);
            while (static_cast<int>(g_State.BuildHistory.size()) > kBuildHistoryMax)
                g_State.BuildHistory.pop_front();
        }
        LOG_F(INFO, "BuildNavMesh %s", ok ? "OK" : "FAILED");
    };

    // -- FindPath --
    auto doFindPath = [&]()
    {
        PathTimings timings;
        bool ok = NavQuery::FindPath(g_State.Nav,
                                      g_State.Start, g_State.End,
                                      g_State.Geom,
                                      g_State.StraightPath,
                                      g_State.PathCorridor,
                                      g_State.PathPolyCount,
                                      g_State.PathStatus,
                                      timings);

        // 路径平滑
        g_State.SmoothedPath.clear();
        if (ok && !g_State.StraightPath.empty() && g_State.View.bSmoothPath)
        {
            if (g_State.View.SmoothMethod == 0)
            {
                dtQueryFilter filter;
                g_State.SmoothedPath = NavPathSmooth::StringPull(
                    g_State.StraightPath,
                    g_State.PathCorridor,
                    g_State.Nav.NavQuery,
                    &filter);
            }
            else
            {
                g_State.SmoothedPath = NavPathSmooth::BezierSubdivide(
                    g_State.StraightPath,
                    g_State.View.SmoothSubdivisions);
            }
        }

        PathStat snap;
        snap.Index          = ++g_State.PathCounter;
        snap.T              = timings;
        snap.CorridorPolys  = g_State.PathPolyCount;
        snap.StraightCorners = static_cast<int>(g_State.StraightPath.size() / 3);
        snap.bOk            = ok;
        snap.Status         = g_State.PathStatus;
        g_State.PathHistory.push_back(snap);
        while (static_cast<int>(g_State.PathHistory.size()) > kPathHistoryMax)
            g_State.PathHistory.pop_front();
        LOG_F(INFO, "FindPath %s", ok ? "OK" : "FAILED");
    };

    // -- RebuildProceduralInputGeometry --
    auto doRebuildProcGeom = [&]()
    {
        NavGeometry::RebuildProceduralGeometry(g_State.Geom, g_State.Scene);
        g_State.bGeomDirty = true;
        PhysWorld::RebuildFromInputGeometry(g_State.Geom);
    };

    // -- GenerateNavLinks --
    // 基于当前 NavMesh 边界生成自动 NavLink，并以新链接为 extraLinks 再 build 一次烘焙进 NavMesh。
    // 定义放在 doTryAutoRebuild 之前，使后者能直接调用（auto lambda 引用必须前向可见）。
    auto doGenerateNavLinks = [&]()
    {
        g_State.AutoNavLinks = NavLinkGen::GenerateEdgeNavLinks(
            g_State.Nav, g_State.AutoNavLinkCfg);
        LOG_F(INFO, "GenerateNavLinks: %d links generated",
              static_cast<int>(g_State.AutoNavLinks.size()));

        if (!g_State.AutoNavLinks.empty())
        {
            bool ok = NavBuilder::BuildNavMesh(g_State.Geom, g_State.Config,
                                               g_State.BV, g_State.Nav, g_State.Timings,
                                               &g_State.AutoNavLinks);
            g_State.BuildStatus = ok ? "OK (with AutoNavLinks)" : "rebuild failed";
            LOG_F(INFO, "Rebuild with AutoNavLinks: %s", ok ? "OK" : "FAILED");
        }
    };

    // -- TryAutoRebuild --
    auto doTryAutoRebuild = [&]()
    {
        if (g_State.View.bAutoRebuild && g_State.bGeomDirty)
        {
            // TileCache 模式：不重跑完整流水线，只将几何同步到 TileCache
            if (g_State.Nav.bTileMode)
            {
                NavBuilder::SyncObstaclesToTileCache(g_State.Geom, g_State.Nav,
                                                     g_State.TileCacheObsRefs);
                g_State.bGeomDirty = false;
            }
            else
            {
                doBuildNavMesh();
                // 几何变化后，旧的 AutoNavLinks 是基于旧 NavMesh 边界生成的，
                // 在新网格上很可能错位/失效。Auto NavLinks 启用时立即按新网格重生成
                // （doGenerateNavLinks 会用新链接再 build 一次，最后烘焙到当前 NavMesh）。
                // TileCache 模式不走完整 build 流水线，链接生成需要触发非 tile 重建，
                // 易和 TileCache 状态冲突，因此仅在非 tile 模式下自动联动。
                if (g_State.AutoNavLinkCfg.bEnabled && g_State.Nav.bBuilt)
                    doGenerateNavLinks();
            }
            if (g_State.View.bAutoReplan && g_State.Nav.bBuilt)
                doFindPath();
        }
    };

    // -- TryAutoReplan --
    auto doTryAutoReplan = [&]()
    {
        if (g_State.View.bAutoReplan && g_State.Nav.bBuilt)
            doFindPath();
    };

    // -- DestroyNavRuntime --
    auto doDestroyNavRuntime = [&]()
    {
        NavBuilder::DestroyNavRuntime(g_State.Nav);
    };

    // -- InitDefaultGeometry --
    auto doInitDefaultGeometry = [&]()
    {
        NavGeometry::InitDefaultGeometry(g_State.Geom, g_State.Scene);
        g_State.bGeomDirty = true;
        PhysWorld::RebuildFromInputGeometry(g_State.Geom);
    };

    // -- FrameCameraToBounds --
    auto doFrameCameraToBounds = [&]()
    {
        FrameCameraToBounds();
    };

    // -- ClampAllObstaclesToScene --
    auto doClampAllObstacles = [&](float halfSize)
    {
        NavGeometry::ClampAllObstaclesToScene(g_State.Geom, halfSize);
    };

    // -- LoadObjGeometry --
    auto doLoadObjGeometry = [&](const char* path, std::string& errOut) -> bool
    {
        if (!NavGeometry::LoadObjGeometry(path, g_State.Geom, errOut)) return false;
        g_State.bGeomDirty = true;
        PhysWorld::RebuildFromInputGeometry(g_State.Geom);
        return true;
    };

    // -- SaveNavMesh --
    auto doSaveNavMesh = [&](const char* path, std::string& errOut) -> bool
    {
        return NavSerializer::SaveNavMesh(g_State.Nav, path, errOut);
    };

    // -- LoadNavMesh --
    auto doLoadNavMesh = [&](const char* path, std::string& errOut) -> bool
    {
        return NavSerializer::LoadNavMesh(g_State.Nav, path, errOut);
    };

    // 装填 Panel 回调
    cbs.Panels.BuildNavMesh                   = doBuildNavMesh;
    cbs.Panels.FindPath                       = doFindPath;
    cbs.Panels.TryAutoRebuild                 = doTryAutoRebuild;
    cbs.Panels.TryAutoReplan                  = doTryAutoReplan;
    cbs.Panels.RebuildProceduralInputGeometry = doRebuildProcGeom;
    cbs.Panels.FrameCameraToBounds            = doFrameCameraToBounds;
    cbs.Panels.DestroyNavRuntime              = doDestroyNavRuntime;
    cbs.Panels.InitDefaultGeometry            = doInitDefaultGeometry;
    cbs.Panels.ClampAllObstaclesToScene       = doClampAllObstacles;
    cbs.Panels.LoadObjGeometry                = doLoadObjGeometry;
    cbs.Panels.SaveNavMesh                    = doSaveNavMesh;
    cbs.Panels.LoadNavMesh                    = doLoadNavMesh;
    cbs.Panels.GenerateNavLinks               = doGenerateNavLinks;

    // 装填 Interaction 回调（与 Panel 共享同一组操作）
    cbs.Interaction.BuildNavMesh                   = doBuildNavMesh;
    cbs.Interaction.FindPath                       = doFindPath;
    cbs.Interaction.TryAutoRebuild                 = doTryAutoRebuild;
    cbs.Interaction.TryAutoReplan                  = doTryAutoReplan;
    cbs.Interaction.RebuildProceduralInputGeometry = doRebuildProcGeom;
    cbs.Interaction.FrameCameraToBounds            = doFrameCameraToBounds;

    // -----------------------------------------------------------------------
    // 初始化默认场景
    // -----------------------------------------------------------------------
    PhysWorld::Init();
    doInitDefaultGeometry();
    FrameCameraToBounds();

    // -----------------------------------------------------------------------
    // 平台窗口循环
    // -----------------------------------------------------------------------
    int rc = 0;

#if defined(_WIN32)
    rc = DirectX::CreateWindowInstance(
        "RecastNavigation Demo",
        1400, 900, 0, 0,
        /*OnTick*/   [](float) {},
        /*OnGUI*/    [&](float) {
            if (g_State.Nav.bTileMode)
                NavBuilder::UpdateTileCache(g_State.Nav);
            MainLayout::OnGUI(g_State, cbs);
        },
        /*OnPostGUI*/[](float) {},
        /*OnInput*/  [](float) {});
#else
    {
        static const std::string s_kWindowTitle = "RecastNavigation Demo";
        const int e = GLCreateWindow(1400, 900, s_kWindowTitle,
                                     false, true, 60,
                                     nullptr, nullptr, nullptr,
                                     nullptr, nullptr, nullptr);
        if (e != EXIT_SUCCESS) return e;
        rc = GLWindowTick(
            [](float) {},
            [&](float) {
                if (g_State.Nav.bTileMode)
                    NavBuilder::UpdateTileCache(g_State.Nav);
                MainLayout::OnGUI(g_State, cbs);
            });
        GLDestroyWindow();
    }
#endif

    // -----------------------------------------------------------------------
    // 清理
    // -----------------------------------------------------------------------
    PhysWorld::Shutdown();
    NavBuilder::DestroyNavRuntime(g_State.Nav);
    return rc;
}
