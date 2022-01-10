cmake_minimum_required(VERSION 3.0...3.21)

project(imnodes)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} IMNODES_SRC)

add_library(${PROJECT_NAME} STATIC ${IMNODES_SRC})

target_include_directories(${PROJECT_NAME} PUBLIC 
    ${SOLUTION_ROOT}/Thirdparty/imgui
    ${SOLUTION_ROOT}/Thirdparty/imnodes
    ${SOLUTION_ROOT}/include
)