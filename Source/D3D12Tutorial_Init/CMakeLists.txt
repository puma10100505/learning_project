cmake_minimum_required(VERSION 3.0...3.21)

# Include was invoked before project instruction for using scope outside CMake(project context is LearningOpenGL)
include(${SOLUTION_ROOT}/SharedDef.cmake)

# After invoking project, the context will change to 'gl_model'
project(D3D12Tutorial_Init VERSION 1.0)

option(USE_D3D "Use windows d3d library" ON)

# 收集用于生成目标的源代码目录(可替代file(GLOB ...)), Should be before add_executable
aux_source_directory(${PROJECT_SOURCE_DIR} SRC)

# Target: 生成可执行文件目标
 add_executable(${PROJECT_NAME} ${SRC})

 if (WIN32)
    # 入口函数使用WinMain
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# 生成可执行文件所需的includes路径
include_directories(${PROJECT_NAME})

# 链接库文件
link_extra_libs(${PROJECT_NAME})

# 部署
deploy_files(${PROJECT_NAME})
