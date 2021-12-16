cmake_minimum_required(VERSION 3.0...3.21)

project(glad)

add_library(${PROJECT_NAME} STATIC glad.c)

target_include_directories(${PROJECT_NAME} 
    PUBLIC 
    ${SOLUTION_ROOT}/Thirdparty
    ${SOLUTION_ROOT}/Thirdparty/glad
    ${SOLUTION_ROOT}/include
)