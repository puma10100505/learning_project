cmake_minimum_required(VERSION 3.0...3.21)

project(node-editor)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} NODEEDITOR_SRC)

add_library(${PROJECT_NAME} STATIC ${NODEEDITOR_SRC})

message(${SOLUTION_ROOT})

target_include_directories(${PROJECT_NAME} PUBLIC 
    ${SOLUTION_ROOT}/Thirdparty/imgui
    ${SOLUTION_ROOT}/Thirdparty/node-editor
    ${SOLUTION_ROOT}/include
)