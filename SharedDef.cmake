cmake_minimum_required(VERSION 3.0...3.21)

# 构造Include目录列表
macro(generate_include_directories param_project_name)

    target_include_directories(${param_project_name} 
        PRIVATE 
        ${SOLUTION_ROOT}/Source/${param_project_name}
        ${SOLUTION_ROOT}/Include
        ${SOLUTION_ROOT}/Thirdparty
        ${SOLUTION_ROOT}/Source/LearningFoundation
        ${SOLUTION_ROOT}/Include/PhysX/include
        ${SOLUTION_ROOT}/Include/pxshared/include
        ${SOLUTION_ROOT}/Include/ImGuizmo
        ${SOLUTION_ROOT}/Include/DirectX
        ${SOLUTION_ROOT}/Include/plog
        ${SOLUTION_ROOT}/Include/box2d
        ${SOLUTION_ROOT}/Include/ogre
        ${SOLUTION_ROOT}/Thirdparty/imgui
        ${SOLUTION_ROOT}/Thirdparty/NetImGui
        ${SOLUTION_ROOT}/Thirdparty/implot
        ${SOLUTION_ROOT}/Thirdparty/imnodes
        ${SOLUTION_ROOT}/Thirdparty/node-editor
        ${SOLUTION_ROOT}/Thirdparty/stb_image
        ${SOLUTION_ROOT}/Thirdparty/loguru
    )
endmacro(generate_include_directories)

###########################################


macro(include_directories param_project_name)
    target_include_directories(${param_project_name} 
        PRIVATE 
        ${SOLUTION_ROOT}/Source/${param_project_name}
        ${SOLUTION_ROOT}/Include
        ${SOLUTION_ROOT}/Thirdparty
        ${SOLUTION_ROOT}/Source/LearningFoundation                 
        ${SOLUTION_ROOT}/Include/PhysX/include
        ${SOLUTION_ROOT}/Include/pxshared/include
        ${SOLUTION_ROOT}/Include/ImGuizmo
        ${SOLUTION_ROOT}/Include/DirectX
        ${SOLUTION_ROOT}/Include/plog
        ${SOLUTION_ROOT}/Include/box2d
        ${SOLUTION_ROOT}/Include/ogre
        ${SOLUTION_ROOT}/Thirdparty/imgui
        ${SOLUTION_ROOT}/Thirdparty/NetImGui
        ${SOLUTION_ROOT}/Thirdparty/implot
        ${SOLUTION_ROOT}/Thirdparty/imnodes
        ${SOLUTION_ROOT}/Thirdparty/node-editor
        ${SOLUTION_ROOT}/Thirdparty/stb_image
        ${SOLUTION_ROOT}/Thirdparty/loguru
    )
endmacro(include_directories)

# 构造库引用信息
macro(link_extra_libs param_project_name)   
    message("USE_IMPLOT: "          ${USE_IMPLOT})
    message("USE_NETIMGUI: "        ${USE_NETIMGUI})
    message("USE_FREEGLUT: "        ${USE_FREEGLUT})
    message("USE_IMNODES: "         ${USE_IMNODES})
    message("USE_NODEEDITOR: "      ${USE_NODEEDITOR})
    message("USE_LOGURU: "          ${USE_LOGURU})
    message("USE_IMGUIZMO: "        ${USE_IMGUIZMO})
    message("USE_IMGUI: "           ${USE_IMGUI})
    message("USE_PHYSX: "           ${USE_PHYSX})
    message("USE_GLFW: "            ${USE_GLFW})
    message("USE_D3D: "             ${USE_D3D})
    message("USE_NAV: "             ${USE_NAV})
    message("USE_BOX2D: "           ${USE_BOX2D})
    message("USE_OGRE: "            ${USE_OGRE})


    # 基础库
    list(APPEND EXTRA_LIBS learning_foundation stb_image glad)


    if (USE_NAV)
        list(APPEND EXTRA_LIBS recastnavigation)
    endif()

    if (USE_IMPLOT)
        list(APPEND EXTRA_LIBS imgui)
        list(APPEND EXTRA_LIBS implot)
    endif()

    if (USE_NETIMGUI)
        list(APPEND EXTRA_LIBS imgui)
        list(APPEND EXTRA_LIBS NetImGui)
    endif()

    if (USE_IMNODES)
        list(APPEND EXTRA_LIBS imgui)
        list(APPEND EXTRA_LIBS imnodes)
    endif()

    if (USE_NODEEDITOR)
        list(APPEND EXTRA_LIBS imgui)
        list(APPEND EXTRA_LIBS node-editor)
    endif()

    if (USE_LOGURU)
        list(APPEND EXTRA_LIBS loguru)
    endif()

    if (USE_IMGUIZMO)
        list(APPEND EXTRA_LIBS imgui)
        list(APPEND EXTRA_LIBS ImGuizmo)
    endif()

    if (USE_IMGUI)
        list(APPEND EXTRA_LIBS imgui)
    endif()    

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
            ${SOLUTION_ROOT}/Libraries/MacOS/PhysX/Debug
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
        )

        if (USE_GLFW)
            list(APPEND EXTRA_LIBS glfw3.lib)
        endif()

        if (USE_BOX2D)
            list(APPEND EXTRA_LIBS box2d.lib)
        endif()

        if (USE_OGRE)
            list(APPEND EXTRA_LIBS 
                Codec_Assimp_d.lib
                Codec_STBI_d.lib 
                DefaultSamples_d.lib 
                OgreBites_d.lib 
                OgreGLSupport_d.lib 
                OgreMain_d.lib 
                OgreMeshLodGenerator_d.lib 
                OgreOverlay_d.lib 
                OgrePaging_d.lib 
                OgreProperty_d.lib 
                OgreRTShaderSystem_d.lib 
                OgreTerrain_d.lib 
                OgreVolume_d.lib 
                Plugin_BSPSceneManager_d.lib 
                Plugin_DotScene_d.lib 
                Plugin_OctreeSceneManager_d.lib 
                Plugin_OctreeZone_d.lib 
                Plugin_ParticleFX_d.lib 
                Plugin_PCZSceneManager_d.lib 
                RenderSystem_Direct3D11_d.lib 
                RenderSystem_GL_d.lib 
                RenderSystem_GL3Plus_d.lib 
                RenderSystem_GLES2_d.lib
            )
        endif()

        if (USE_PHYSX)        
            # PhysX
            list(APPEND EXTRA_LIBS 
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
                PhysXCooking_64.lib
            )

            target_link_directories(${param_project_name} PRIVATE ${SOLUTION_ROOT}/Libraries/Windows/PhysX/Debug)
        endif()

        if (USE_FREEGLUT)        
            # Freeglut
            list(APPEND EXTRA_LIBS 
                freeglut_staticd.lib
                freeglutd.lib
            )

            target_link_directories(${param_project_name} PRIVATE ${SOLUTION_ROOT}/Libraries/Windows/freeglut)
        endif()

        if (USE_D3D)        
            # For DirectX12
            list(APPEND EXTRA_LIBS 
                d3d12.lib 
                d3dcompiler.lib 
                dxgi.lib
            )
        endif()
        

        target_link_directories(${param_project_name}
            PRIVATE 
            ${SOLUTION_ROOT}/Libraries/Windows
            ${SOLUTION_ROOT}/Libraries/Windows/PhysX/Debug            
            ${SOLUTION_ROOT}/Libraries/Windows/glew            
            ${SOLUTION_ROOT}/Libraries/Windows/glut
        )
    endif()

    target_compile_options(${param_project_name} 
        PRIVATE 
        $<$<PLATFORM_ID: Linux, Darwin>: -O2 -g>
    )
    
    # Extra libraries
    target_link_libraries(${param_project_name} PUBLIC ${EXTRA_LIBS})
endmacro(link_extra_libs )

# 部署输出文件
# param_project_name: PROJECT_NAME
macro(deploy_files param_project_name)
    add_custom_command(TARGET ${param_project_name} 
        COMMAND ${CMAKE_COMMAND} -E make_directory 
        ${SOLUTION_ROOT}/Bin
    )

    if (WIN32)
        list(APPEND DYNAMIC_LIBS 
            ${SOLUTION_ROOT}/Libraries/Windows/PhysX/Debug/PhysX_64.dll 
            ${SOLUTION_ROOT}/Libraries/Windows/PhysX/Debug/PhysXCommon_64.dll 
            ${SOLUTION_ROOT}/Libraries/Windows/PhysX/Debug/PhysXFoundation_64.dll 
            ${SOLUTION_ROOT}/Libraries/Windows/PhysX/Debug/PhysXCooking_64.dll 
            ${SOLUTION_ROOT}/Libraries/Windows/assimp-vc142-mtd.dll
            ${SOLUTION_ROOT}/Libraries/Windows/freeglut/freeglutd.dll
            #${SOLUTION_ROOT}/Libraries/Windows/glew/glew32.dll
        )

        add_custom_command(TARGET ${param_project_name} 
            COMMAND ${CMAKE_COMMAND} -E copy 
            ${DYNAMIC_LIBS}
            ${PROJECT_BINARY_DIR}/Debug/
        )

        add_custom_command(TARGET ${param_project_name} 
            COMMAND ${CMAKE_COMMAND} -E copy 
            ${DYNAMIC_LIBS}
            ${SOLUTION_ROOT}/Bin
        )

        add_custom_command(TARGET ${param_project_name} 
            COMMAND ${CMAKE_COMMAND} -E copy 
            ${PROJECT_BINARY_DIR}/Debug/${param_project_name}.exe 
            ${SOLUTION_ROOT}/Bin
        )
    elseif (APPLE)
        add_custom_command(TARGET ${param_project_name} 
            COMMAND ${CMAKE_COMMAND} -E copy 
            ${PROJECT_BINARY_DIR}/${param_project_name}
            ${SOLUTION_ROOT}/Bin
        )
    elseif (UNIX)
        add_custom_command(TARGET ${param_project_name} 
            COMMAND ${CMAKE_COMMAND} -E copy 
            ${PROJECT_BINARY_DIR}/${param_project_name}
            ${SOLUTION_ROOT}/Bin
        )
    endif()
endmacro(deploy_files)