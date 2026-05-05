# RecastNaviDemo 重构方案

> 目标：将 3500+ 行的单文件拆分为职责清晰的模块，便于导航系统的学习和扩展。  
> 原则：**导航核心独立、渲染可替换、UI 纯展示**。

---

## 一、现状问题

| 问题 | 影响 |
|------|------|
| 单文件 3500+ 行，UI/渲染/导航全混杂 | 想学 Recast 构建流程，要在 UI 代码里找 `BuildNavMesh`（第 725 行） |
| 全局变量 30+，散布在文件各处 | 状态流向不清晰，难以追踪 g_Nav / g_Geom 何时被修改 |
| 2D 渲染、3D 渲染、ImGui 面板均写在一起 | 渲染方式的替换（如切换到 OpenGL 着色器）牵一发动全身 |
| 导航参数结构体和 UI 控件硬绑定 | 无法复用 `BuildNavMesh` 于其他项目或测试场景 |

---

## 二、目标目录结构

```
Source/RecastNaviDemo/
│
├── RecastNaviDemo.cpp          ← 只保留 main()，30 行以内
├── CMakeLists.txt
│
├── App/
│   └── AppState.h              ← 全局状态聚合（替代散落的 30+ 全局变量）
│
├── Nav/                        ← ★ 导航核心（最重要的学习区域）
│   ├── NavTypes.h              ← 所有导航数据结构
│   ├── NavGeometry.h/.cpp      ← 输入几何管理（程序化生成 + OBJ 加载）
│   ├── NavBuilder.h/.cpp       ← Recast 构建管线（BuildNavMesh）
│   ├── NavQuery.h/.cpp         ← Detour 路径查询（FindPath）
│   └── NavSerializer.h/.cpp   ← NavMesh 的 Save / Load
│
├── Render/                     ← 渲染层（纯绘制，不含业务逻辑）
│   ├── RenderTypes.h           ← Vec3、Mat4、OrbitCamera、ProjectedPoint
│   ├── Primitives.h/.cpp       ← Line3D、TriFilled3D、RingXZ 等基元
│   ├── Renderer2D.h/.cpp       ← DrawCanvas2D
│   └── Renderer3D.h/.cpp       ← DrawCanvas3D
│
├── UI/                         ← UI 层（纯展示 + 交互，不含导航逻辑）
│   ├── UITypes.h               ← ViewSettings、EditMode、ViewMode
│   ├── Interaction.h/.cpp      ← HandleCanvasInteraction + HandleHotkeys
│   ├── FileDialog.h/.cpp       ← 平台文件对话框（Win/macOS/Linux）
│   ├── Panels.h/.cpp           ← 所有配置面板（View/Scene/Edit/Build/Path/IO）
│   ├── StatsWindow.h/.cpp      ← 性能趋势图浮窗
│   └── MainLayout.h/.cpp       ← OnGUI：主窗口布局 + 面板编排
│
└── Shared/                     ← 跨模块工具
    ├── MathUtils.h             ← Vec3、Mat4 及数学函数
    └── Profiling.h             ← ScopedTimer、PhaseTimings、BuildStat、PathStat
```

---

## 三、各模块详细说明

### 3.1 App/AppState.h

将现有 30+ 全局变量按职责归入 4 个结构体，并在 `AppState` 中聚合：

```cpp
// App/AppState.h
#pragma once
#include "Nav/NavTypes.h"
#include "Render/RenderTypes.h"
#include "UI/UITypes.h"
#include "Shared/Profiling.h"

struct AppState {
    // --- 导航数据 ---
    InputGeometry   Geom;
    SceneConfig     Scene;
    NavBuildConfig  Config;
    NavRuntime      Nav;

    // --- 路径结果 ---
    float                       Start[3]    = {-10, 0, -10};
    float                       End[3]      = { 12, 0,  12};
    std::vector<float>          StraightPath;
    std::vector<dtPolyRef>      PathCorridor;
    dtStatus                    PathStatus  = 0;
    std::string                 BuildStatus;
    bool                        bGeomDirty  = false;

    // --- 性能历史 ---
    std::deque<BuildStat>  BuildHistory;  // 最多 128
    std::deque<PathStat>   PathHistory;   // 最多 128
    int BuildCounter = 0;
    int PathCounter  = 0;

    // --- 视图/编辑状态 ---
    ViewSettings  View;
    OrbitCamera   Cam;
    ViewMode      CurrentViewMode = ViewMode::TopDown2D;
    EditMode      CurrentEditMode = EditMode::None;
    int           SelectedObstacle = -1;
    CreateDraftBox CreateDraft;
    MoveBoxState   MoveState;

    // --- UI 状态 ---
    bool  bShowStatsWindow  = true;
    float LeftPaneWidth     = 320.f;
    float StatsDockHeight   = 240.f;
    float StatsLegendWidth  = 180.f;
    ImVec4 PathColor        = {1, 0.6f, 0, 1};
    ImVec4 CorridorColor    = {0, 0.5f, 1, 0.25f};
    char  ObjPathInput[MAX_PATH]     = {};
    char  NavSavePathInput[MAX_PATH] = {};
    char  NavLoadPathInput[MAX_PATH] = {};

    // --- Map 缓存（Interaction 需要）---
    Map2D LastMap2D;
    Map3D LastMap3D;
};

// 单例访问（仅 main 拥有，其他模块通过引用传参）
AppState& GetAppState();
```

> **关键设计**：AppState 以 **引用** 方式传递给各模块，不再有隐式全局依赖，利于单元测试和多场景扩展。

---

### 3.2 Nav/ — 导航核心（★ 最重要）

这是重构后最值得学习的区域，每个文件对应一个 Recast/Detour 概念：

#### Nav/NavTypes.h
```
保留现有的：
  ObstacleShape、Obstacle、GeomSource、InputGeometry
  SceneConfig、NavBuildConfig、NavRuntime
  CapturedRcContext（Recast 日志捕获）
新增：
  NavBuildResult（构建结果，与 NavRuntime 解耦）
```

#### Nav/NavGeometry.h/.cpp
```
职责：管理"喂给 Recast 的几何数据"

函数：
  void  RebuildProceduralGeometry(InputGeometry& geom, const SceneConfig& scene)
  bool  LoadObjGeometry(const char* path, InputGeometry& geom)
  void  ClampObstacleToScene(Obstacle& o, float halfSize)
  bool  TriCenterInsideAnyObstacle(...)
  Obstacle MakeBox(...)
  Obstacle MakeCylinder(...)
  void  InitDefaultGeometry(InputGeometry& geom, const SceneConfig& scene)
```

#### Nav/NavBuilder.h/.cpp（★ 核心学习文件）
```
职责：封装完整的 Recast 构建管线

// 接口清晰，入参出参一目了然
bool BuildNavMesh(
    const InputGeometry&  geom,     // 输入：几何数据
    const NavBuildConfig& config,   // 输入：构建参数
    NavRuntime&           runtime,  // 输出：NavMesh 运行时
    PhaseTimings&         timings   // 输出：各阶段耗时
);

void DestroyNavRuntime(NavRuntime& runtime);
bool InitNavMeshFromData(NavRuntime& runtime);

// 构建管线内部按阶段拆分为私有函数，便于逐步学习：
//   Step1_CreateHeightfield(...)
//   Step2_RasterizeTriangles(...)
//   Step3_FilterWalkable(...)
//   Step4_BuildCompactHeightfield(...)
//   Step5_ErodeWalkableArea(...)
//   Step6_BuildRegions(...)
//   Step7_BuildContours(...)
//   Step8_BuildPolyMesh(...)
//   Step9_BuildDetailMesh(...)
//   Step10_CreateDetourNavMesh(...)
```

> 拆成私有 Step 函数后，注释可以对应 Recast 文档，是学习导航网格构建原理的最佳入口。

#### Nav/NavQuery.h/.cpp
```
职责：Detour 路径查询

bool FindPath(
    const NavRuntime&       nav,
    const float             start[3],
    const float             end[3],
    std::vector<float>&     outStraightPath,
    std::vector<dtPolyRef>& outCorridor,
    dtStatus&               outStatus,
    PathTimings&            outTimings
);
```

#### Nav/NavSerializer.h/.cpp
```
职责：NavMesh 二进制序列化

bool SaveNavMesh(const NavRuntime& nav, const char* path);
bool LoadNavMesh(NavRuntime& nav, const char* path);
```

---

### 3.3 Render/ — 渲染层

渲染层**只知道怎么画，不知道为什么画**，所有判断逻辑由调用方决定。

#### Render/RenderTypes.h
```
Vec3、Mat4（及相关数学函数）
OrbitCamera
ProjectedPoint
Map2D、Map3D（屏幕↔世界坐标映射缓存）
```

#### Render/Primitives.h/.cpp
```
// 所有函数接收 ImDrawList* 和 ViewProjection 矩阵，无全局依赖
void Line3D(ImDrawList*, const Mat4& vp, Vec3 a, Vec3 b, ImU32 col, float thickness=1)
void TriFilled3D(ImDrawList*, const Mat4& vp, Vec3 a, Vec3 b, Vec3 c, ImU32 col)
void RingXZ(ImDrawList*, const Mat4& vp, Vec3 center, float r, int segs, ImU32 col)
void DiscXZFilled(ImDrawList*, const Mat4& vp, Vec3 center, float r, int segs, ImU32 col)
void DrawCylinderObstacle3D(...)
void DrawCapsule3D(...)
```

#### Render/Renderer2D.h/.cpp
```
// 绘制参数结构体，避免参数列表爆炸
struct Draw2DParams {
    const InputGeometry& Geom;
    const NavRuntime&    Nav;
    const float*         Start;
    const float*         End;
    const ViewSettings&  View;
    // ... 路径、走廊、草稿等
};

Map2D DrawCanvas2D(ImDrawList*, ImVec2 canvasPos, ImVec2 canvasSize, const Draw2DParams&);
```

#### Render/Renderer3D.h/.cpp
```
struct Draw3DParams { /* 同上 */ };
Map3D DrawCanvas3D(ImDrawList*, ImVec2 canvasPos, ImVec2 canvasSize, OrbitCamera&, const Draw3DParams&);

// 射线拣取（从 3D 视图中提取，属于渲染层辅助）
bool ScreenTo3DGroundWorld(ImVec2 screen, const Map3D&, const InputGeometry&, float out[3]);
```

---

### 3.4 UI/ — UI 层

UI 层调用 Nav/ 和 Render/，但 **Nav/ 和 Render/ 不反向依赖 UI/**。

#### UI/Interaction.h/.cpp
```
// 画布交互和快捷键，修改 AppState
void HandleCanvasInteraction(AppState& state, ImVec2 canvasPos, ImVec2 canvasSize);
void HandleHotkeys(AppState& state);
```

#### UI/FileDialog.h/.cpp
```
// 平台文件对话框（已有良好封装，几乎不变）
bool ShowOpenFileDialog(const char* filter, char* outPath, size_t size);
bool ShowSaveFileDialog(const char* filter, char* outPath, size_t size);
```

#### UI/Panels.h/.cpp
```
// 每个面板接收 AppState 引用，读写状态，触发导航操作
void DrawViewPanel   (AppState& state);
void DrawScenePanel  (AppState& state);
void DrawEditPanel   (AppState& state);
void DrawBuildPanel  (AppState& state);   // ← 调用 NavBuilder::BuildNavMesh
void DrawPathPanel   (AppState& state);   // ← 调用 NavQuery::FindPath
void DrawIOPanel     (AppState& state);   // ← 调用 NavSerializer + FileDialog
void DrawStatsPanel  (AppState& state);
```

#### UI/StatsWindow.h/.cpp
```
// 性能趋势图浮窗（独立性强，改动最少）
void DrawBuildStatsWindow(const AppState& state, bool* pOpen);
```

#### UI/MainLayout.h/.cpp
```
// 主 GUI 回调：布局 + 面板调度
void OnGUI(AppState& state, float deltaTime);
```

---

### 3.5 RecastNaviDemo.cpp（重构后）

```cpp
// ← 只剩 main，约 40 行
#include "App/AppState.h"
#include "Nav/NavGeometry.h"
#include "UI/MainLayout.h"

// 平台窗口头文件（保持现有跨平台逻辑）
#ifdef _WIN32
  #include "DirectX12Window.h"
#else
  #include "GlfwWindows.h"
#endif

static AppState g_State;
AppState& GetAppState() { return g_State; }

int main(int argc, char** argv) {
    // 初始化 loguru
    // 初始化默认几何
    NavGeometry::InitDefaultGeometry(g_State.Geom, g_State.Scene);
    // 创建窗口 + 主循环（现有逻辑不变）
    ...
    GLWindowTick(
        [](float dt) { /* OnTick：空 */ },
        [](float dt) { OnGUI(g_State, dt); }
    );
    // 清理
    NavBuilder::DestroyNavRuntime(g_State.Nav);
}
```

---

## 四、依赖关系图

```
                    ┌─────────────┐
                    │  main.cpp   │
                    └──────┬──────┘
                           │ 拥有 AppState，调用
              ┌────────────┼────────────┐
              ▼            ▼            ▼
         ┌────────┐  ┌──────────┐  ┌────────┐
         │  Nav/  │  │ Render/  │  │  UI/   │
         └────────┘  └──────────┘  └────────┘
              ▲            ▲            │
              │            │       调用 ▼
              │       ┌────────────────┤
              └───────│   Shared/      │
                      │   App/State    │
                      └────────────────┘

规则：
  Nav/    → 只依赖 NavTypes.h、Shared/、Recast/Detour SDK
  Render/ → 只依赖 RenderTypes.h、Shared/、ImGui
  UI/     → 依赖 Nav/、Render/、App/AppState
  Nav/    ✗ 不依赖 UI/ 或 Render/
  Render/ ✗ 不依赖 UI/ 或 Nav/
```

---

## 五、重构步骤（优先级顺序）

| 步骤 | 内容 | 风险 | 预估工作量 |
|------|------|------|-----------|
| **P1** | 创建 `Shared/MathUtils.h` + `Shared/Profiling.h`，把 Vec3/Mat4/ScopedTimer 等移过去 | 低（纯搬移，无逻辑变化） | 0.5h |
| **P2** | 创建 `Nav/NavTypes.h`，搬所有导航数据结构 | 低 | 0.5h |
| **P3** | 创建 `Nav/NavGeometry.cpp`，搬输入几何相关函数 | 低 | 1h |
| **P4** | 创建 `Nav/NavBuilder.cpp`（★），搬 `BuildNavMesh` 并拆分 Step 函数 | 中（需按阶段整理逻辑） | 2h |
| **P5** | 创建 `Nav/NavQuery.cpp`，搬 `FindPath` 等 | 低 | 0.5h |
| **P6** | 创建 `Nav/NavSerializer.cpp`，搬 Save/Load | 低 | 0.5h |
| **P7** | 创建 `App/AppState.h`，把 30+ 全局变量整合，修改所有引用处 | 高（引用点多） | 3h |
| **P8** | 创建 `Render/Primitives.cpp` + `Renderer2D/3D.cpp` | 中（接口需重新设计参数传递方式） | 2h |
| **P9** | 创建 `UI/` 各面板文件，搬 ImGui 面板 | 中 | 2h |
| **P10** | `main.cpp` 瘦身，验证编译和功能 | 低 | 1h |

> **建议先做 P1→P4**：仅把导航核心独立出来就能覆盖 80% 的学习需求，P5 之后可按需推进。

---

## 六、扩展性收益

完成重构后，新增功能的改动范围将大幅缩小：

| 新功能 | 改动文件 |
|--------|---------|
| 新增 TileCache 动态障碍支持 | 只需在 `Nav/NavBuilder.cpp` 增加构建分支 |
| 替换 3D 渲染为真正的 OpenGL 着色器 | 只改 `Render/Renderer3D.cpp` |
| 添加"寻路调试步进"功能 | 只改 `Nav/NavQuery.cpp` + `UI/Panels.cpp` |
| 添加多 Agent 仿真 | 新建 `Nav/NavCrowd.h/.cpp`，不影响其他模块 |
| 命令行批量构建测试 | 只依赖 `Nav/`，完全不需要 UI/ 和 Render/ |

---

*方案版本 v1.0 — 2026/04/30*
