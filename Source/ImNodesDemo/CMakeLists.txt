cmake_minimum_required(VERSION 3.0...3.21)

# Include was invoked before project instruction for using scope outside CMake(project context is LearningOpenGL)
include(${SOLUTION_ROOT}/SharedDef.cmake)

# After invoking project, the context will change to 'gl_model'
project(ImNodesDemo VERSION 1.0)

option(USE_IMNODES "Use imnodes library" ON)
option(USE_GLFW "Use GLFW library" ON)
option(USE_FREEGLUT "Use freeglut" ON)

# 收集用于生成目标的源代码目录(可替代file(GLOB ...)), Should be before add_executable
aux_source_directory(${PROJECT_SOURCE_DIR} SRC)

# 生成的VS工程中包括Header文件夹
file(GLOB_RECURSE HEADER 
    ${SOLUTION_ROOT}/Include/*.h*
    ${SOLUTION_ROOT}/Thirdparty/*.h*
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h*
)

# Target: 生成可执行文件目标
 add_executable(${PROJECT_NAME} ${SRC})

# 生成可执行文件所需的includes路径
include_directories(${PROJECT_NAME})

link_extra_libs(${PROJECT_NAME})

deploy_files(${PROJECT_NAME})
