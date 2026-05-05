cmake_minimum_required(VERSION 3.0...3.21)

project(imgui)

# 核心源文件（跨平台）
set(IMGUI_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_impl_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_impl_glut.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_impl_opengl2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_impl_opengl3.cpp
)

# DirectX / Win32 后端仅在 Windows 上编译
if (WIN32)
    list(APPEND IMGUI_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui_impl_dx11.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui_impl_dx12.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui_impl_win32.cpp
    )
endif()

add_library(${PROJECT_NAME} STATIC ${IMGUI_SRC})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${SOLUTION_ROOT}/Thirdparty/imgui
    ${SOLUTION_ROOT}/include
)
