# learning_project
 
 ## Introduce
 * 将learning_opengl、learning_physx合并到同一个项目中，因为有部分共用的内容，后续还会增加recastnavigation的实验代码。此工程主要以学习实验为目的，通过搭建一个可用的环境了解OpenGL, PhysX, RecastNavigation等相关的开源项目的使用方法，因为要在不同的地方编写代码，所以这个工程被动地实现了CrossPlatform : ). 在Windows和MacOS都可以运行，Linux在后续也可以运行，不过当前还有一些Linux平台的库和CMake代码没有完善。
 

 ---

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


---

## 工程结构介绍
- Assets: 资源文件夹，包括字体(Fonts)、纹理(Textures)、3D模型（Meshes）以及着色器（Shader）
- Bin: 生成的可执行文件都会统一复制到这个目录
- Dependency: 备份的一些依赖项目
- Include: 工程的统一包含目录
- Libraries: 工程的统一库目录，其中按平台分为Windows, MacOS和Linux
- Source: 源代码目录
- Thirdparty: 第三方库的目录，部分以源码形式提供的库放在这里构建产生静态链接库文件

## 编译构建方法
TODO