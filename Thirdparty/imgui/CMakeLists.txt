cmake_minimum_required(VERSION 3.0...3.21)

project(imgui)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} IMGUI_SRC)

add_library(${PROJECT_NAME} STATIC ${IMGUI_SRC})

message(${SOLUTION_ROOT})

target_include_directories(${PROJECT_NAME} PUBLIC 
    ${SOLUTION_ROOT}/include/imgui
    ${SOLUTION_ROOT}/include
)