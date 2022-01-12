cmake_minimum_required(VERSION 3.0...3.21)

project(
    LearningProject 
    VERSION 1.0 
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(SOLUTION_ROOT ${PROJECT_SOURCE_DIR})

# 以下为所有的外部库开关列表，可在源代码子目录中选择使用
# option(USE_PHYSX "Use physx library" ON)
# option(USE_D3D "Use windows d3d library" ON)
# option(USE_FREEGLUT "Use Freeglut library" ON)
# option(USE_IMGUI "Use imgui library" ON)
# option(USE_NETIMGUI "Use netimgui library" ON)
# option(USE_IMPLOT "Use implot library" ON)
# option(USE_IMNODES "Use imnodes library" ON)
# option(USE_IMGUIZMO "Use imguizmo library" ON)
# option(USE_LOGURU "Use loguru library" ON)
# option(USE_NODEEDITOR "Use node-editor library" ON)
# option(USE_GLFW "Use GLFW library" ON)
# option(USE_NAV "Use navigation library" ON)


if (WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
    add_definitions(/wd"4819" /wd"4996" /wd"4800" /wd"4005")
endif()
 
# include directories
add_subdirectory(Source)
add_subdirectory(Thirdparty)

#add_subdirectory(Tools)
