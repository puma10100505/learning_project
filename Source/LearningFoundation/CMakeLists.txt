cmake_minimum_required(VERSION 3.0...3.21)

project(learning_foundation)

option(USE_PERFETTO "Use perfetto" ON)
option(USE_D3D "Use D3D" ON)

message(${SOLUTION_ROOT})

set(LearningOpenGL_VERSION_MAJOR ${CMAKE_PROJECT_VERSION_MAJOR})
set(LearningOpenGL_VERSION_MINOR ${CMAKE_PROJECT_VERSION_MINOR})

configure_file(CommonDefines.h.in ${SOLUTION_ROOT}/Source/LearningFoundation/CommonDefines.h)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} LEARNING_SRC)
# DirectX 与 Win32 绑定，仅在 Windows 上编译进静态库
if (WIN32)
    aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/DirectX LEARNING_SRC)
endif()
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/OpenGL LEARNING_SRC)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Physics LEARNING_SRC)

add_library(${PROJECT_NAME} STATIC ${LEARNING_SRC})

# PhysX 头文件（PxPreprocessor.h）要求 _DEBUG 与 NDEBUG 必须恰好定义其一。
# MSVC Debug 构建会自动定义 _DEBUG；Clang/GCC 不会，需要手动补充。
if (NOT WIN32)
    target_compile_definitions(${PROJECT_NAME} PUBLIC _DEBUG)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC 
    ${SOLUTION_ROOT}/Source/LearningFoundation
    ${SOLUTION_ROOT}/Source/LearningFoundation/DirectX
    ${SOLUTION_ROOT}/Source/LearningFoundation/OpenGL
    ${SOLUTION_ROOT}/Source/LearningFoundation/Physics
    ${SOLUTION_ROOT}/Include
    ${SOLUTION_ROOT}/Include/DirectX
    ${SOLUTION_ROOT}/Thirdparty
    ${SOLUTION_ROOT}/Thirdparty/loguru
    ${SOLUTION_ROOT}/Thirdparty/imgui
    ${SOLUTION_ROOT}/Thirdparty/implot
    ${SOLUTION_ROOT}/Thirdparty/imnodes
    ${SOLUTION_ROOT}/Thirdparty/node-editor
    ${SOLUTION_ROOT}/Thirdparty/SOIL2
    ${SOLUTION_ROOT}/Include/pxshared/include
    ${SOLUTION_ROOT}/Include/PhysX/include
    ${SOLUTION_ROOT}/Thirdparty/NetImGui/Client/Private
)
