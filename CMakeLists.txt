cmake_minimum_required(VERSION 3.0...3.21)

project(
    LearningProject
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(SOLUTION_ROOT ${PROJECT_SOURCE_DIR})


 
# include directories
add_subdirectory(Include)
add_subdirectory(Source)
add_subdirectory(Thirdparty)
add_subdirectory(Tools)
