cmake_minimum_required(VERSION 3.0...3.21)

# Include was invoked before project instruction for using scope outside CMake(project context is LearningOpenGL)
include(${PROJECT_SOURCE_DIR}/SharedDef.cmake)

add_subdirectory(gl_depth_testing)
add_subdirectory(gl_cubemap_skybox)
add_subdirectory(gl_stencil_testing)
add_subdirectory(gl_frame_buffer)