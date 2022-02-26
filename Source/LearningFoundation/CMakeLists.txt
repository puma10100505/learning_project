cmake_minimum_required(VERSION 3.0...3.21)

project(learning_foundation)

option(USE_PERFETTO "Use perfetto" ON)

set(ASSETS_PATH_BASE ${SOLUTION_ROOT}/)

message(${SOLUTION_ROOT})

configure_file(CommonDefines.h.in ${SOLUTION_ROOT}/Source/LearningFoundation/CommonDefines.h)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} LEARNING_SRC)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/DirectX LEARNING_SRC)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/OpenGL LEARNING_SRC)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Physics LEARNING_SRC)

add_library(${PROJECT_NAME} STATIC ${LEARNING_SRC})

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