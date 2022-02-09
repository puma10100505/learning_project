# learning_project
 
 ## Introduce
 * 将learning_opengl、learning_physx合并到同一个项目中，因为有部分共用的内容，后续还会增加recastnavigation的实验代码。此工程主要以学习实验为目的，通过搭建一个可用的环境了解OpenGL, PhysX, RecastNavigation等相关的开源项目的使用方法，因为要在不同的地方编写代码，所以这个工程被动地实现了CrossPlatform : ). 在Windows和MacOS都可以运行，Linux在后续也可以运行，不过当前还有一些Linux平台的库和CMake代码没有完善。
 


 ## LearningProject涉及的开源库、工具和接口：
 - OpenGL: 从learning_opengl工程引入的部分代码，是在学习LearnOpenGL的时候产生的。https://learnopengl-cn.github.io/, https://learnopengl.com/(英文版)
 - CMake: 使用CMake生成构建脚本，尽可能地使用了Modern CMake(3.0+)的方法实现
 - PhysX: Nvidia开源物理库，无需解释
 - glfw3.3.4: 开源的GL库，用于创建窗口系统，支持OpenGL, OpenGLES和Vulkan, https://www.glfw.org/docs/latest/index.html
 - assimp: 支持导入多种3D模型的开源库， https://github.com/assimp/assimp，LearningProject使用的是3.3.1版本
 - imgui: 强大的即时GUI库，总体来看上手较简单也不用过多担心布局问题，在LearningProject中用于支持运行时的实时数据修改（未来应该也会用它实现所有的交互功能）。https://github.com/ocornut/imgui
 - glm: 用于GL的数学库，这是一个开源的头文件库，非常容易集成到工程中。https://github.com/Groovounet/glm
 - glad: gl接口加载助手，可在https://glad.dav1d.de/ 生成针对不同GL版本的加载程序
 - SOIL: 全称是Simple OpenGL Image Library，是用于加载纹理到OpenGL程序的开源库，它基于stb_image. https://github.com/SpartanJ/SOIL2
 - stb_image: 支持多种主流图像格式的加载与解码，可以从内存或文件加载图像数据。https://github.com/nothings/stb, 除了stb_image之外，stb这个开源库还有很多其它有意思的东西（音频、3D、工具等相关内容）
 - glut: OpenGL的工具函数库，用于快速创建基本形状
 - freeglut: glut太老，使用freeglut代替，如果要快速使用物理库实验可以使用freeglut作为Windows以及Input库，因为它可以快速创建Geometry物件。SDL, GLFW都没有类似功能接口
 - recastnavigation: 开源导航信息生成以及寻路库，可用于AI移动
 - implot: 基于imgui的图表库，可用于数据分析应用(https://github.com/epezent/implot)
 - imnodes: 基于imgui的节点编辑库，可用于类蓝图或状态机应用(https://github.com/Nelarius/imnodes)
 - node-editor: 基于imgui的节点编辑器库，类似imnodes，但是比它更加全面和复杂(可以做出Unreal Engine蓝图的效果)(https://github.com/puma10100505/imgui-node-editor)
 - csv.h: CSV文件解析库
 - rapidjson: JSON处理库
 - loguru: 日志库
 - glog: Google的日志库，loguru在Win32应用中无法使用，loguru初始化的时候需要argc, argv参数，这在WinMain中不提供
 - p7: 日志库同时实现了基于网络的日志监控服务的开源项目
 - perfetto: Google Android中的性能数据采集以及跟踪库，功能强大，具体可参考：https://perfetto.dev/docs/，C++ SDK可参考：https://perfetto.dev/docs/instrumentation/tracing-sdk


## 工程结构介绍
- Assets: 资源文件夹，包括字体(Fonts)、纹理(Textures)、3D模型（Meshes）以及着色器（Shader）
- Bin: 生成的可执行文件都会统一复制到这个目录
- Dependency: 备份的一些依赖项目
- Include: 工程的统一包含目录
- Libraries: 工程的统一库目录，其中按平台分为Windows, MacOS和Linux
- Source: 源代码目录
- Thirdparty: 第三方库的目录，部分以源码形式提供的库放在这里构建产生静态链接库文件

## 编译构建方法
1. 如果没有特别说明，修改的CMakeLists.txt文件都是位于Source/XXX/CMakeLists.txt这个地方的文件，即程序最内层的CMakeLists.txt文件
2. 确保系统中已经安装cmake最新版本
3. 下载源代码后在工程根目录创建一个用于构建的目录，可使用命令：cmake -E make_directory cmake-build-debug, cmake-build-debug可以换成其它任何名称
4. 在工程根目录执行命令：
    - cmake -B cmake-build-debug
    - cmake --build cmake-build-debug
5. 等待构建完成后在工程根目录可以看到Bin文件夹，里边就是构建的输出（可执行文件）
6. 对于一些非必要的库，可以针对性地在CMakeLists.txt文件中开启或关闭(如果不显式指定，开关默认为OFF)，构建开关列表：
    - USE_PHYSX
    - USE_D3D
    - USE_FREEGLUT
    - USE_IMGUI
    - USE_NETIMGUI
    - USE_IMPLOT
    - USE_IMNODES
    - USE_IMGUIZMO
    - USE_LOGURU
    - USE_NODEEDITOR
    - USE_GLFW
    - USE_NAV
    - USE_BOX2D
    - USE_OGRE
    - USE_PERFETTO
7. 如果要创建Windows窗口程序，需要在CMakeLists.txt中加上如下的代码：
    `
    # 如果是WIN32窗口程序，入口函数使用WinMain
    if (WIN32)        
        set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
    `

## 调试代码
1. 工程支持在VSCode环境下调试代码，工程已经有配置好的launch.json文件
2. 要进行代码调试，在Source下的每个APP目录，其中的主入口代码文件名要与其所在的目录同名，如DxImGuiWindowDemo/DxImGuiWindowDemo.cpp就是DxImGuiWindowDemo这个目录下的主入口代码文件，main函数位于其中。这是方便在VSCode中调试的一个命名约定。

## DevLog:
- 根据https://learnopengl-cn.github.io/01%20Getting%20started/09%20Camera/#_3的方法实现简单的摄像机控制。摄像机控制完成后可以更方便地实现基于物理的场景渲染，更容易跟踪场景物体。
- 渲染的时候如果设置了uniform但是没有在程序中设置它们，可能会导致渲染结果出现奇怪的错误，如果通过uniform设置MVP矩阵变换，但是在程序中没有设置MVP矩阵的值，可能会导致图形不能正常显示。
- OPENGL中VBO, VAO, EBO的概念以及关系，学习使用GL的基础API指令绘制图形
- 集成Freeglut以及基于Glut的ImGUI, GlutWindowWithImGui目录下的代码可作为其DEMO, 此DEMO可用于物理引擎试验，因为Freeglut有方便的接口用于绘制基础几何形状。下一步可在此基础上引入物理计算。
- 将implot, imnodes以及node-editor集成到工程中，后续创建窗口的时候默认支持implot, imnodes和node-editor, 可以在应用中直接调用它们的API，不用考虑它们的初始化和销毁

## TODO
* 重构场景相关对象集中管理渲染对象
* 重构窗口对象，实现WindowManager以及Window类，集中管理窗口相关上下文
* 实现基于glfw+PhysX+LearningFoundation的物理场景物体渲染。
* Linux环境下的工程配置以及构建脚本
* 渲染与逻辑分离，多线程实现*
* 物理引擎的序列化和反序列化功能研究
