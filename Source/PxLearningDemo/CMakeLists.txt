cmake_minimum_required(VERSION 3.0...3.21)

include(${SOLUTION_ROOT}/SharedDef.cmake)

project(PxLearningDemo VERSION 0.1)

learning_demo_reset_link_flags()
set(USE_PHYSX ON)
set(USE_D3D ON)
set(USE_FREEGLUT ON)
set(USE_IMGUI OFF)
set(USE_NETIMGUI ON)
set(USE_IMPLOT ON)
set(USE_IMNODES ON)
set(USE_IMGUIZMO OFF)
set(USE_LOGURU OFF)
set(USE_NODEEDITOR ON)
set(USE_GLFW OFF)

aux_source_directory(${PROJECT_SOURCE_DIR} SRC)
add_executable(${PROJECT_NAME} ${SRC})
include_directories(${PROJECT_NAME})
link_extra_libs(${PROJECT_NAME})

deploy_files(${PROJECT_NAME})