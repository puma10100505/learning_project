cmake_minimum_required(VERSION 3.0...3.21)

# Include was invoked before project instruction for using scope outside CMake(project context is LearningOpenGL)
include(${PROJECT_SOURCE_DIR}/SharedDef.cmake)

add_subdirectory(GLDepthTesting)
add_subdirectory(GLCubemapSkybox)
add_subdirectory(GLStencilTesting)
add_subdirectory(GLFrameBuffer)