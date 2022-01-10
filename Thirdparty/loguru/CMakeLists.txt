cmake_minimum_required(VERSION 3.0...3.21)

project(loguru)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} LOGURU_SRC)

add_library(${PROJECT_NAME} STATIC ${LOGURU_SRC})


target_include_directories(${PROJECT_NAME} PUBLIC 
    ${SOLUTION_ROOT}/include
    ${SOLUTION_ROOT}/Thirdparty/loguru
)

