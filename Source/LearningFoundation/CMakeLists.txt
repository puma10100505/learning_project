cmake_minimum_required(VERSION 3.0...3.21)

project(learning_foundation)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} LEARNING_SRC)

add_library(${PROJECT_NAME} STATIC ${LEARNING_SRC})

target_include_directories(${PROJECT_NAME} PUBLIC 
    ${SOLUTION_ROOT}/Source/LearningFoundation
    ${SOLUTION_ROOT}/Include
    ${SOLUTION_ROOT}/Thirdparty/SOIL2
    ${SOLUTION_ROOT}/Include/pxshared/include
)