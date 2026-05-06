cmake_minimum_required(VERSION 3.0...3.21)

# Include was invoked before project instruction for using scope outside CMake(project context is LearningOpenGL)
include(${SOLUTION_ROOT}/SharedDef.cmake)

# After invoking project, the context will change to 'gl_model'
project(gl_multilights VERSION 1.0)

learning_demo_reset_link_flags()
set(USE_GLFW ON)
set(USE_IMGUI ON)

# 收集用于生成目标的源代码目录(可替代file(GLOB ...)), Should be before add_executable
aux_source_directory(${PROJECT_SOURCE_DIR} SRC)

# Target: 生成可执行文件目标
add_executable(${PROJECT_NAME} ${SRC})

# 生成可执行文件所需的includes路径
include_directories(${PROJECT_NAME})

link_extra_libs(${PROJECT_NAME})

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

deploy_files(${PROJECT_NAME})
