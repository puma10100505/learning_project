cmake_minimum_required(VERSION 3.0...3.21)

project(perfetto)

find_package(Threads)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} PERFETTO_SDK_SRC)

add_library(${PROJECT_NAME} STATIC ${PERFETTO_SDK_SRC})

if (WIN32)
  # The perfetto library contains many symbols, so it needs the big object
  # format.
  target_compile_options(perfetto PRIVATE "/bigobj")
  # Disable legacy features in windows.h.
  add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX)
endif (WIN32)