cmake_minimum_required(VERSION 3.0...3.21)

project(perfetto_sdk)

find_package(Threads)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} PERFETTO_SDK_SRC)

add_library(${PROJECT_NAME} STATIC ${PERFETTO_SDK_SRC})

# if (WIN32)
#     set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY COMPILE_DEFINITIONS "NOMINMAX")
# endif()