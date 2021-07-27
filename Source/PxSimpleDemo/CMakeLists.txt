cmake_minimum_required(VERSION 3.0...3.21)

include(${SOLUTION_ROOT}/SharedDef.cmake)

project(PxSimpleDemo VERSION 0.1)

aux_source_directory(${PROJECT_SOURCE_DIR} SRC)
add_executable(${PROJECT_NAME} ${SRC})
include_directories(${PROJECT_NAME})
link_extra_libs(${PROJECT_NAME})

if (WIN32)
    set(DYNAMIC_LIBS 
        ${SOLUTION_ROOT}/lib/win64/PhysX/PhysX_64.dll
        ${SOLUTION_ROOT}/lib/win64/PhysX/PhysXCommon_64.dll
        ${SOLUTION_ROOT}/lib/win64/PhysX/PhysXFoundation_64.dll
    )

    # 部署动态库文件
    add_custom_command(TARGET ${PROJECT_NAME} 
        COMMAND ${CMAKE_COMMAND} -E copy 
        ${DYNAMIC_LIBS}
        ${PROJECT_BINARY_DIR}/Debug/
    )
endif()

deploy_bin(${PROJECT_NAME})