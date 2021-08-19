cmake_minimum_required(VERSION 3.0...3.21)

project(SOIL2)

aux_source_directory(${PROJECT_SOURCE_DIR} SRC)

add_library(${PROJECT_NAME} STATIC ${SRC})

target_include_directories(${PROJECT_NAME} PUBLIC 
    ${PROJECT_SOURCE_DIR}
)