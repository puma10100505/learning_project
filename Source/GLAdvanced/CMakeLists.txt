cmake_minimum_required(VERSION 3.0...3.21)

# Include was invoked before project instruction for using scope outside CMake(project context is LearningOpenGL)
include(${SOLUTION_ROOT}/SharedDef.cmake)

if(MSVC)
    add_compile_options(/utf-8)
endif()

add_subdirectory(GLDepthTesting)
add_subdirectory(GLCubemapSkybox)
add_subdirectory(GLStencilTesting)
add_subdirectory(GLFrameBuffer)