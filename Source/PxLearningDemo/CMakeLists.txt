cmake_minimum_required(VERSION 3.0...3.21)

include(${SOLUTION_ROOT}/SharedDef.cmake)

project(PxLearningDemo VERSION 0.1)

option(USE_PHYSX "Use physx library" ON)
option(USE_D3D "Use windows d3d library" ON)
option(USE_FREEGLUT "Use Freeglut library" ON)
option(USE_IMGUI "Use imgui library" OFF)
option(USE_NETIMGUI "Use netimgui library" ON)
option(USE_IMPLOT "Use implot library" ON)
option(USE_IMNODES "Use imnodes library" ON)
option(USE_IMGUIZMO "Use imguizmo library" OFF)
option(USE_LOGURU "Use loguru library" OFF)
option(USE_NODEEDITOR "Use node-editor library" ON)
option(USE_GLFW "Use GLFW library" OFF)

aux_source_directory(${PROJECT_SOURCE_DIR} SRC)
add_executable(${PROJECT_NAME} ${SRC})
include_directories(${PROJECT_NAME})
link_extra_libs(${PROJECT_NAME})

deploy_files(${PROJECT_NAME})