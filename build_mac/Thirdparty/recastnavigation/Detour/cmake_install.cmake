# Install script for directory: /Users/yinpsoft/GameScience/Github/learning_project/Thirdparty/recastnavigation/Detour

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/Library/Developer/CommandLineTools/usr/bin/objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/Users/yinpsoft/GameScience/Github/learning_project/build_mac/Thirdparty/recastnavigation/Detour/libDetour-d.a")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libDetour-d.a" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libDetour-d.a")
    execute_process(COMMAND "/Library/Developer/CommandLineTools/usr/bin/ranlib" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libDetour-d.a")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/recastnavigation/DetourAlloc.h;/recastnavigation/DetourAssert.h;/recastnavigation/DetourCommon.h;/recastnavigation/DetourMath.h;/recastnavigation/DetourNavMesh.h;/recastnavigation/DetourNavMeshBuilder.h;/recastnavigation/DetourNavMeshQuery.h;/recastnavigation/DetourNode.h;/recastnavigation/DetourStatus.h")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/recastnavigation" TYPE FILE FILES
    "/Users/yinpsoft/GameScience/Github/learning_project/Thirdparty/recastnavigation/Detour/Include/DetourAlloc.h"
    "/Users/yinpsoft/GameScience/Github/learning_project/Thirdparty/recastnavigation/Detour/Include/DetourAssert.h"
    "/Users/yinpsoft/GameScience/Github/learning_project/Thirdparty/recastnavigation/Detour/Include/DetourCommon.h"
    "/Users/yinpsoft/GameScience/Github/learning_project/Thirdparty/recastnavigation/Detour/Include/DetourMath.h"
    "/Users/yinpsoft/GameScience/Github/learning_project/Thirdparty/recastnavigation/Detour/Include/DetourNavMesh.h"
    "/Users/yinpsoft/GameScience/Github/learning_project/Thirdparty/recastnavigation/Detour/Include/DetourNavMeshBuilder.h"
    "/Users/yinpsoft/GameScience/Github/learning_project/Thirdparty/recastnavigation/Detour/Include/DetourNavMeshQuery.h"
    "/Users/yinpsoft/GameScience/Github/learning_project/Thirdparty/recastnavigation/Detour/Include/DetourNode.h"
    "/Users/yinpsoft/GameScience/Github/learning_project/Thirdparty/recastnavigation/Detour/Include/DetourStatus.h"
    )
endif()

