cmake_minimum_required(VERSION 3.0...3.21)

include(${SOLUTION_ROOT}/SharedDef.cmake)

project(TempTest VERSION 0.1)

aux_source_directory(${PROJECT_SOURCE_DIR} SRC)
add_executable(${PROJECT_NAME} ${SRC})
include_directories(${PROJECT_NAME})
link_extra_libs(${PROJECT_NAME})

deploy_files(${PROJECT_NAME})