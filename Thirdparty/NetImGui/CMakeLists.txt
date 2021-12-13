cmake_minimum_required(VERSION 3.0...3.21)

project(NetImGui)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Client NETIMGUI_SRC)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Client/Private NETIMGUI_SRC)

add_library(${PROJECT_NAME} STATIC ${NETIMGUI_SRC})

message(${SOLUTION_ROOT})

target_link_libraries(NetImGui PUBLIC imgui)

target_include_directories(${PROJECT_NAME} PUBLIC 
    ${SOLUTION_ROOT}/include/imgui
    ${SOLUTION_ROOT}/include
    ${SOLUTION_ROOT}/include/NetImGui
)

