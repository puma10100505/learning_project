# `learning_project` 项目结构说明

> 本文档由项目结构分析整理而成，作为 `README.md` 的补充说明，集中描述工程目录、构建系统、源码组织、第三方依赖与设计约定。

这是一个 **跨平台的图形与游戏底层技术学习实验工程**，由作者把之前的 `learning_opengl`、`learning_physx` 合并而来，并预计扩展到 RecastNavigation、Box2D、Ogre 等领域。整体定位是「通过搭建一个统一可用的环境，集中学习/试验各种游戏开发底层开源库」。

下面分四个层面来梳理。

---

## 1. 顶层目录结构

| 目录 | 作用 |
| --- | --- |
| `Assets/` | 运行时资源：`Fonts`、`Textures`、`Meshes`、`Shaders`、`Data` |
| `Bin/` | 构建后统一输出可执行文件（被 `deploy_files` 宏复制到这里） |
| `Dependency/` | 备份的依赖项（脚本/源码包） |
| `Include/` | 工程统一头文件目录（`PhysX`、`DirectX`、`box2d`、`ogre`、`glm`、`GLFW`、`assimp`、`plog`、`p7`、`rapidjson`、`nlohmann_json` 等以及 `csv.h`、`inicpp.h`） |
| `Libraries/` | 平台预编译库，按 `Windows / MacOS / Linux` 分目录存放 |
| `Source/` | **本项目自己的代码**，每个子目录是一个 demo 或基础库 |
| `Thirdparty/` | 以源码方式集成、需要本工程自行编译为静态库的第三方（`glad`、`imgui`、`implot`、`imnodes`、`node-editor`、`ImGuizmo`、`stb_image`、`SOIL2`、`loguru`、`NetImGui`、`recastnavigation`、`perfetto_sdk`、`nvBlast` 等） |
| `Tools/` | 本地工具（`NetImguiServer`、`perfetto`、`BaicalServer`、`RecastnaviSample`、`box2d-testbed.exe` 等） |
| `.vscode/` | 已配置好的 `launch.json`、`tasks.json`、`c_cpp_properties.json`、`settings.json`，README 里说明了 VSCode 调试约定 |
| 根目录脚本 | `Build.bat`、`Rebuild.bat`、`ReloadCMakeCache.bat`、`UpdateProject.bat` 简化构建流程 |

---

## 2. 构建系统（Modern CMake）

构建结构是「三层 CMakeLists」：

- 根 `CMakeLists.txt`：定义工程、C++20、`SOLUTION_ROOT`，并 `add_subdirectory(Source)`、`add_subdirectory(Thirdparty)`。
- `Source/CMakeLists.txt`：通过注释行手动开关每个 demo 是否参与构建（当前启用的是 `LearningFoundation`、`DxImGuiWindowDemo`、`D3DSimpleApp`，其他全部注释掉）。
- 每个 demo 自己的 `CMakeLists.txt`：通常只调用几个共享宏。

核心是 `SharedDef.cmake`，里面定义了 3 个工程共用宏：

- `generate_include_directories(target)` / `include_directories(target)`：一次性把工程内所有公用 include 目录加到 target 上。
- `link_extra_libs(target)`：根据 `USE_PHYSX / USE_D3D / USE_GLFW / USE_FREEGLUT / USE_IMGUI / USE_IMPLOT / USE_IMNODES / USE_IMGUIZMO / USE_NETIMGUI / USE_NODEEDITOR / USE_LOGURU / USE_NAV / USE_BOX2D / USE_OGRE / USE_PERFETTO / USE_BOOST_FILESYSTEM` 这一组开关，分别 `list(APPEND EXTRA_LIBS …)` 并 `add_compile_definitions(...)`，再按平台（`APPLE / UNIX / WIN32`）追加各自的链接库与 `target_link_directories`。
- `deploy_files(target)`：构建完成后把 `PhysX_64.dll`、`assimp-vc142-mtd.dll`、`freeglutd.dll` 等运行时 DLL 以及生成的 `.exe` 复制到 `Bin/`。

也就是说：**任何新建 demo，只需要写很小的 CMakeLists 调用这三个宏，就能继承全套依赖。**

---

## 3. 源码组织（`Source/`）

`Source/` 下有 **3 类内容**：

### (1) 共享基础库 `LearningFoundation`

`LearningFoundation` 是一个静态库，被所有 demo 链接。它从 `CommonDefines.h.in` 生成带 `ASSETS_PATH_BASE` 的头文件，并把根目录、`DirectX/`、`OpenGL/`、`Physics/` 三个子目录 `aux_source_directory` 一起编进去。其内部三大模块是：

- **OpenGL 模块**：`GlfwWindows`、`GlutWindow`、`GLMesh / GLModel / GLObject / GLScene`、`GLCubic / GLQuad`、`GLTexture`、灯光体系（`GLLight / GLLightBase / GLDirectionLight / GLPointLight / GLSpotLight`）、`GLCamera`。
- **DirectX 模块**：DX12 学习样例风格，`d3dApp`、`d3dUtil`、`DXSample / DXSampleHelper`、`Win32Application`、`DirectX12Window`、`GameTimer`、`MathHelper`、`DDSTextureLoader`，基本是微软 D3D12 官方示例那一套封装。
- **Physics 模块**：`PhysicsManager`、`SceneManager`，作为 PhysX 的轻量管理器。

另外还有顶层的 `LearningCamera`、`LearningShader`、`LearningStatics`、`BlueprintNodeManager`、`learning.h / utility.h / defines.h / thirdparty_header.h`。`learning.h` 把整个引擎的常用头（`glm`、`stb_image`、`Camera`、`Shader`、`GLMesh/GLModel/GLLight…`）整合成一个总入口。

`Source/Shared/PerfettoTracingHelper.h` 是 perfetto tracing 的共享辅助头。

### (2) 模板（`Source/Templates/`）

提供三种常见 demo 起手模板：

- `DxAppTemplate`（DirectX）
- `GlfwAppTemplate`（GLFW + OpenGL）
- `GlutAppTemplate`（FreeGLUT）

### (3) 各类 Demo / 实验工程

按主题大致可以分为 5 簇：

- **OpenGL 基础（LearnOpenGL 路线）**：`GLBasic`、`GLPrimitives`、`GLShaders`、`GLTextures`、`GLTransform`、`GLCameraSystem`、`GLLight`、`GLMultilights`、`GLModels`、`GLAdvanced`、`GLUtilDemo`。
- **FreeGLUT / GLUT 系**：`GLFreeglutDemo`、`ImGuiWithFreeglutDemo`。
- **DirectX 系（HLSL / DX12）**：`D3DHelloTriangle`、`D3DSimpleApp`、`D3DMathVector`、`D3DMathMatrix`、`D3D12Tutorial_Init`、`DxImGuiWindowDemo`。
- **ImGui 生态扩展**：`ImPlotDemo`（图表）、`ImNodesDemo`、`NodeEditorDemo`、`NodeApp`（节点编辑器）、`GizmoDemo` / `GizmoExample`（ImGuizmo）。
- **物理 / 工具 / Tracing**：`PxLearningDemo`、`PxSimpleDemo`（PhysX）、`GooglePerfettoExample`（Perfetto Tracing SDK）。

`Source/CMakeLists.txt` 里能看到作者还给每个 demo 标注了状态注释，如「使用了老的 GLFW 接口（已废弃）」、「无法打开窗口」、「编译不过」，相当于一份隐式的 demo 健康清单。

---

## 4. 第三方依赖（两条引入路径）

工程刻意区分：

- **`Include/` + `Libraries/<平台>/`**：以「头文件 + 预编译库」方式接入的库——`PhysX`、`assimp`、`GLFW`、`box2d`、`ogre`、`boost`、`DirectX`、`glm`、`plog`、`p7`、`glog`、`rapidjson`、`nlohmann_json`，以及单文件的 `csv.h`、`inicpp.h`。
- **`Thirdparty/`（源码）**：自行编译为静态库——`glad`、`imgui`、`implot`、`imnodes`、`node-editor`、`ImGuizmo`、`stb_image`、`SOIL2`、`loguru`、`NetImGui`、`recastnavigation`、`perfetto_sdk`、`nvBlast`。

`generate_include_directories` / `include_directories` 宏把这两边的所有路径都默认加到 demo 的 include 路径里，所以 demo 内部直接 `#include "imgui/imgui.h"` 之类即可。

---

## 5. 设计上的几个亮点 / 约定

1. **统一开关**：`USE_XXX` 系列开关 + `add_compile_definitions(USE_XXX)`，让代码侧可以用 `#ifdef USE_PHYSX` 等控制是否启用某模块，与 CMake 完全联动。
2. **跨平台分支**：`SharedDef.cmake` 里 `if (APPLE) elseif(UNIX) elseif(WIN32)` 三套链接库列表，保证 macOS 已经能跑、Linux 留有占位、Windows 是主战场。
3. **VSCode 调试约定**：每个 demo 主入口源文件名必须等于其所在目录名，如 `DxImGuiWindowDemo/DxImGuiWindowDemo.cpp`，便于 `launch.json` 通用化。
4. **运行时部署**：`deploy_files` 宏一并搞定 DLL 复制 + 可执行文件汇总到 `Bin/`，避免缺 DLL 的常见痛点。
5. **总入口头**：`Source/LearningFoundation/learning.h` 把 GLM、stb、Camera、Shader、各种 GL 几何体/灯光头都聚合进来，demo 里通常只需要 `#include "learning.h"` 即可起步。

---

## 6. 路线图（README 的 TODO）

作者在 `README.md` 里列出来的下一步：

- 重构场景对象、统一渲染对象管理。
- 重构窗口对象，做 `WindowManager` / `Window` 类。
- 基于 `glfw + PhysX + LearningFoundation` 的物理场景渲染。
- Linux 平台 CMake 完整支持。
- 渲染与逻辑分离 + 多线程化。
- 物理引擎序列化/反序列化研究。

---

## 7. 一句话总结

这是一个 **「以 CMake 为构建核心 + `LearningFoundation` 静态库为公共底座 + N 个独立 demo 为试验场」** 的跨平台 C++20 学习沙盒，覆盖 OpenGL（LearnOpenGL 体系）、DirectX12、PhysX、ImGui 系生态（`implot` / `imnodes` / `node-editor` / `ImGuizmo` / `NetImGui`）、RecastNavigation、Box2D、Ogre 以及 Perfetto / loguru 等基础设施，借助 `USE_*` 开关可以按需裁剪、按 demo 试验。
