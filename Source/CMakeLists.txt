cmake_minimum_required(VERSION 3.0...3.21)

### Required Foundation:
add_subdirectory(LearningFoundation)

### Optional Projects Below:
# add_subdirectory(GLModels)
# add_subdirectory(GLCameraSystem)
# add_subdirectory(GLPrimitives)
# add_subdirectory(GLTransform)
# add_subdirectory(GLShaders)
# add_subdirectory(GLFreeglutDemo)
 add_subdirectory(PxLearningDemo_CreateBox)
# add_subdirectory(ImGuiWithFreeglutDemo)

# 使用了老的glfw窗口接口（老接口已废弃）
# add_subdirectory(GLBasic)
# add_subdirectory(GLMultilights)
# add_subdirectory(GLAdvanced)

# 无法打开窗口
# add_subdirectory(PxSimpleDemo)    
# add_subdirectory(GLUtilDemo)

# 编译不过
# add_subdirectory(GLLight) 