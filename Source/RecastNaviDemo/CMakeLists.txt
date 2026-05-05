cmake_minimum_required(VERSION 3.0...3.21)

# 引入工程共享 CMake 宏（必须在 project() 之前 include, 因为里边的 if/else 依赖 SOLUTION_ROOT）
include(${SOLUTION_ROOT}/SharedDef.cmake)

project(RecastNaviDemo VERSION 1.0)

# 本 demo 依赖：
#   - Windows : D3D12 + ImGui (DirectX12Window)
#   - macOS/Linux: GLFW + OpenGL + ImGui (GlfwWindows, learning_foundation/OpenGL)
#   - IMGUI   : 窗口集成 ImGui
#   - LOGURU  : 日志
if (NOT WIN32)
    set(USE_D3D OFF)
endif()
# 注意：本 demo 不开启 USE_NAV, 因为 SharedDef.cmake 里 USE_NAV 会 link 一个并不存在的
# `recastnavigation` 目标。Recast/Detour 的子库（Recast / Detour / DetourCrowd / ...) 直接
# 在下面用 target_link_libraries 显式链接。
option(USE_D3D    "Use windows d3d library" ON)
option(USE_IMGUI  "Use imgui library"        ON)
option(USE_LOGURU "Use loguru library"       ON)

# 收集源文件
aux_source_directory(${PROJECT_SOURCE_DIR}         SRC)
aux_source_directory(${PROJECT_SOURCE_DIR}/Nav     NAV_SRC)
aux_source_directory(${PROJECT_SOURCE_DIR}/Render  RENDER_SRC)
aux_source_directory(${PROJECT_SOURCE_DIR}/UI      UI_SRC)
aux_source_directory(${PROJECT_SOURCE_DIR}/App     APP_SRC)
aux_source_directory(${PROJECT_SOURCE_DIR}/Phys    PHYS_SRC)

# 可执行目标
add_executable(${PROJECT_NAME} ${SRC} ${NAV_SRC} ${RENDER_SRC} ${UI_SRC} ${APP_SRC} ${PHYS_SRC})

# PhysX：macOS 在 SharedDef 中已链接；Windows 需打开 USE_PHYSX
if(APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE RECAST_NAVI_USE_PHYSX=1)
endif()
if(WIN32)
    set(USE_PHYSX ON)
    target_compile_definitions(${PROJECT_NAME} PRIVATE RECAST_NAVI_USE_PHYSX=1)
endif()

# 源码内嵌中文字符串字面量, 让 MSVC 按 UTF-8 解析源文件 + 执行字符集
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

# 工程通用 include 路径（含 LearningFoundation, imgui, loguru, glm 等）
include_directories(${PROJECT_NAME})

# Recast / Detour 头文件
target_include_directories(${PROJECT_NAME} PRIVATE
    ${SOLUTION_ROOT}/Thirdparty/recastnavigation/Recast/Include
    ${SOLUTION_ROOT}/Thirdparty/recastnavigation/Detour/Include
    ${SOLUTION_ROOT}/Thirdparty/recastnavigation/DetourCrowd/Include
    ${SOLUTION_ROOT}/Thirdparty/recastnavigation/DetourTileCache/Include
    ${SOLUTION_ROOT}/Thirdparty/recastnavigation/DebugUtils/Include
)

# 平台库 + LearningFoundation + ImGui 等（来自 SharedDef.cmake）
link_extra_libs(${PROJECT_NAME})

# 显式链接 Recast / Detour 各子库
# 这些目标是由 Thirdparty/recastnavigation/<子目录>/CMakeLists.txt 中的 add_library() 生成的
target_link_libraries(${PROJECT_NAME} PRIVATE
    Recast
    Detour
    DetourCrowd
    DetourTileCache
)

# 部署 dll/exe 到 Bin/
deploy_files(${PROJECT_NAME})
