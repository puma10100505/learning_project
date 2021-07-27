cmake_minimum_required(VERSION 3.0...3.21)

# 构造Include目录列表
macro(include_directories param_project_name)
    target_include_directories(${param_project_name} 
        PRIVATE 
        ${SOLUTION_ROOT}/Source/LearningFoundation 
        ${SOLUTION_ROOT}/Include/
        ${SOLUTION_ROOT}/Include/imgui
        ${SOLUTION_ROOT}/Thirdparty/stb_image
        ${SOLUTION_ROOT}/Thirdparty/SOIL
        ${SOLUTION_ROOT}/Include/PhysX/include
        ${SOLUTION_ROOT}/Include/pxshared/include
        ${SOLUTION_ROOT}/Include
    )
endmacro(include_directories)

# 构造库引用信息
macro(link_extra_libs param_project_name)    
    list(APPEND EXTRA_LIBS imgui glad learning_foundation stb_image SOIL)
    if (APPLE)
        target_compile_definitions(${PROJECT_NAME} PUBLIC _DEBUG)

        list(APPEND EXTRA_LIBS 
            -lglfw3
            -lassimp            
            -lz
            -lIrrXML
            -lPhysX_static_64
            -lPhysXCommon_static_64
            -lPhysXCharacterKinematic_static_64
            -lPhysXCooking_static_64
            -lPhysXExtensions_static_64
            -lPhysXFoundation_static_64
            -lPhysXPvdSDK_static_64
            -lPhysXVehicle_static_64
            libpthread.a
            -lm
            -lstdc++
            -ldl
        )

        target_link_directories(${param_project_name}
            PRIVATE 
            ${SOLUTION_ROOT}/Libraries/MacOS
            ${SOLUTION_ROOT}/Libraries/MacOS/PhysX/debug
        )

        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo")

    elseif(UNIX)
        list(APPEND EXTRA_LIBS
            libassimp.a
            libglfw3.a
            -lz 
            -lXi 
            -lXrandr 
            -lXxf86vm 
            -lXinerama 
            -lXcursor 
            -lIrrXML
            -lrt 
            -lm  
            -lX11 
            -lGLU 
            -lGL 
            -lpthread
            -lstdc++
            -ldl
            # TODO: PhysX Lib
            )

            target_link_directories(${param_project_name}
                PRIVATE 
                ${SOLUTION_ROOT}/Libraries/Linux
            )
            
    elseif (WIN32)
        list(APPEND EXTRA_LIBS 
            IrrXMLd.lib 
            zlibstaticd.lib
            assimp-vc142-mtd.lib
            glfw3.lib
            LowLevel_static_64.lib
            LowLevelAABB_static_64.lib
            LowLevelDynamics_static_64.lib
            PhysXCharacterKinematic_static_64.lib
            PhysXPvdSDK_static_64.lib
            PhysXTask_static_64.lib
            SceneQuery_static_64.lib
            SimulationController_static_64.lib
            PhysX_64.lib
            PhysXCommon_64.lib
            PhysXFoundation_64.lib
            PhysXExtensions_static_64.lib
        )

        target_link_directories(${param_project_name}
            PRIVATE 
            ${SOLUTION_ROOT}/Libraries/Windows
            ${SOLUTION_ROOT}/Libraries/Windows/PhysX
        )
    endif()

    target_compile_options(${param_project_name} 
        PRIVATE 
        $<$<PLATFORM_ID: Linux, Darwin>: -O2 -g>
    )  
    
    # Extra libraries
    target_link_libraries(${param_project_name} PUBLIC ${EXTRA_LIBS})

endmacro(link_extra_libs )

macro(deploy_bin param_project_name)    
    add_custom_command(TARGET ${param_project_name} 
        COMMAND ${CMAKE_COMMAND} -E make_directory 
        ${SOLUTION_ROOT}/Bin
    )

    add_custom_command(TARGET ${param_project_name} 
        COMMAND ${CMAKE_COMMAND} -E copy_if_different 
        ${PROJECT_BINARY_DIR}/${param_project_name}
        ${SOLUTION_ROOT}/Bin/
    )
endmacro(deploy_bin)