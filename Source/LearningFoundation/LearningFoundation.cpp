#include "LearningFoundation.h"

glm::vec4 BackgroundColor = glm::vec4(0.1f, 0.6f, 0.7f, 1.0f);
float DeltaTime = 0.0f;

GLFWwindow* GetGlobalWindow() {
    return __GlobalWindow;
}

GLFWmonitor* GetPrimaryMonitor()
{
    return __PrimaryMonitor;
}

// @Old Version
void on_glfw_error_default(int code, const char* desc) 
{
    // TODO:
}

void on_frame_buffer_size_changed_default(GLFWwindow* window, int width, int height) {
    // TODO:
    glViewport(0, 0, width, height);
}

void on_key_event_default(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // TODO:
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(GetGlobalWindow(), true);
    }
}

int gl_init_window() {
    if (!glfwInit()) {
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    return 0;
}

int gl_init_gui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();
    // ImGui::StyleColorsClassic();
    // io.Fonts->AddFontFromFileTTF("../resources/fonts/Roboto-Medium.ttf", 18);

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(GetGlobalWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 330");

    return 0;
}

void gl_on_gui_default() {
    
}

// create glfw window
int gl_create_window(int32_t width, int32_t height, 
    const std::string& title, bool hide_cursor, int32_t frameInterval,
    GLFWerrorfun err_handler, GLFWframebuffersizefun fbs_handler, GLFWkeyfun key_handler) {

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    __GlobalWindow = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
    if (!__GlobalWindow) {
        glfwTerminate();
        std::cout << "gl_create_window create window failed\n";
        return -1;
    }

    glfwMakeContextCurrent(__GlobalWindow);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(1);
    
    // set some callback func
    glfwSetErrorCallback(err_handler);
    glfwSetFramebufferSizeCallback(GetGlobalWindow(), fbs_handler);
    glfwSetKeyCallback(GetGlobalWindow(), key_handler);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    if (hide_cursor) {
        // Diable the mouse cursor when startup
        glfwSetInputMode(GetGlobalWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    FPS = 1000 / frameInterval;

    return 0;    
}

void gl_destroy_window() {
    glfwTerminate();
}

void gl_destroy_gui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

int gl_window_loop(std::function<void (void)> on_update, std::function<void (void)> on_gui) {
    
    while (!glfwWindowShouldClose(__GlobalWindow)) {
        glClearColor(BackgroundColor.x, BackgroundColor.y, BackgroundColor.z, BackgroundColor.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        float currentFrameTime = glfwGetTime();
        DeltaTime = currentFrameTime - LastFrameTime;
        LastFrameTime = currentFrameTime;

            // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        on_gui();

        on_update();

        // Rendering
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(__GlobalWindow);
        glfwPollEvents();

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / FPS));
    }

    return 0;
}


// @@@@@@@@@@@@@@@@@ New Version @@@@@@@@@@@@@@@@@@@

void OnGlfwErrorDefault(int Code, const char* Desc)
{
    
}

void FramebufferChangedDefault(GLFWwindow* Window, int Width, int Height) {
    glViewport(0, 0, Width, Height);
    // printf("Current viewport size, width: %d, height: %d\n", Width, Height);
}

void OnKeyEventDefault(GLFWwindow* Window, int Key, int ScanCode, int Action, int Mods) {
    if (Key == GLFW_KEY_ESCAPE && Action == GLFW_PRESS) {
        glfwSetWindowShouldClose(GetGlobalWindow(), true);
    }
}

void WindowCloseCallback(GLFWwindow* InWindow)
{
    printf("Window will be closed...\n");
}

void WindowResizedCallback(GLFWwindow* window, int width, int height)
{
    printf("Window has been resized, width: %d, height: %d\n", width, height);
}

int InitGlfwWindow() {
    if (!glfwInit()) {
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
#endif

    __PrimaryMonitor = glfwGetPrimaryMonitor();
    if (!__PrimaryMonitor)
    {
        std::cout << "Not found primary monitor" << std::endl;        
        return EXIT_FAILURE;
    }

    const GLFWvidmode* mode = glfwGetVideoMode(__PrimaryMonitor);
    if (!mode)
    {
        std::cout << "Not found video mode object" << std::endl;
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    if (bIsFullScreen)
    {
        ScreenWidth = mode->width;
        ScreenHeight = mode->height;
    }

    return 0;
}

int GLInitGUI() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(GetGlobalWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 330");

    return 0;
}

// create glfw window
int GLCreateWindow(int InitWidth, int InitHeight, const std::string& Title, bool bHideCursor, bool bWithGUI, 
    int32_t FrameInterval, GLFWerrorfun GlfwErrCallback, GLFWframebuffersizefun FrameBufferSizeChanged, 
    GLFWkeyfun KeyEventCallback, GLFWcursorposfun MouseMoveCallback, 
    GLFWscrollfun MouseScrollCallback, GLFWmousebuttonfun MouseButtonCallback) 
{
    int iRet = InitGlfwWindow();
    if (iRet < 0)
    {
        printf("Init glfw window failed, ret: %d\n", iRet);
        return EXIT_FAILURE;
    }

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    __GlobalWindow = glfwCreateWindow(InitWidth, InitHeight, Title.c_str(), 
        bIsFullScreen ? GetPrimaryMonitor() : nullptr, nullptr);
    if (!__GlobalWindow) {
        glfwTerminate();
        std::cout << "gl_create_window create window failed\n";
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(__GlobalWindow);
    glfwSwapInterval(1);
    
    // set some callback func
    // if (GlfwErrCallback)
    // {
    //     glfwSetErrorCallback(GlfwErrCallback);
    // }

    if (FrameBufferSizeChanged)
    {
        glfwSetFramebufferSizeCallback(__GlobalWindow, FrameBufferSizeChanged);
    }

    if (KeyEventCallback)
    {
        glfwSetKeyCallback(__GlobalWindow, KeyEventCallback);
        printf("Finished register KeyEventCallback\n");
    }

    if (WindowCloseCallback)
    {
        glfwSetWindowCloseCallback(__GlobalWindow, WindowCloseCallback);
        printf("Finished register WindowCloseCallback\n");
    }

    if (MouseMoveCallback)
    {
        glfwSetCursorPosCallback(__GlobalWindow, MouseMoveCallback);
        printf("Finished register MouseMoveCallback\n");
    }

    if (MouseScrollCallback)
    {
        glfwSetScrollCallback(__GlobalWindow, MouseScrollCallback);
        printf("Finished register MouseScrollCallback\n");
    }

    if (MouseButtonCallback)
    {
        glfwSetMouseButtonCallback(__GlobalWindow, MouseButtonCallback);
        printf("Finished register MouseButtonCallback\n");
    }

    // if (WindowResizedCallback)
    // {
    //     printf("hit here set the resize callback\n");
    //     glfwSetWindowSizeCallback(__GlobalWindow, WindowResizedCallback);
    // }

    glfwFocusWindow(__GlobalWindow);

    // 创建窗口之后，使用OPENGL之前要初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to init GLAD" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    if (bHideCursor) {
        // Diable the mouse cursor when startup
        glfwSetInputMode(GetGlobalWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    if (bWithGUI)
    {
        if ((iRet = GLInitGUI()) < 0) {
            printf("Init imgui failed, iRet: %d\n", iRet);
        }
    }

    glViewport(0, 0, InitWidth, InitHeight);

    FPS = 1000 / FrameInterval;

    return EXIT_SUCCESS;    
}

int GLCreateWindow(const FCreateWindowParameters& Params)
{
    return GLCreateWindow(Params.InitWidth, Params.InitHeight, Params.Title,
        Params.bHideCursor, Params.bWithGUI, Params.FrameInterval, 
        Params.GlfwErrCallback, 
        Params.FrameBufferSizeChanged == nullptr ? FramebufferChangedDefault : Params.FrameBufferSizeChanged, 
        Params.KeyEventCallback, 
        Params.MouseMoveCallback, 
        Params.MouseScrollCallback, 
        Params.MouseButtonCallback);
}

void GLDestroyWindow() {
    if (ImGui::GetCurrentContext())
    {
        GLDestroyGUI();
    }

    glfwTerminate();
}

void GLDestroyGUI() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

int GLWindowTick(std::function<void (float)> OnTick, std::function<void (float)> OnGUI) 
{
    while (!glfwWindowShouldClose(__GlobalWindow)) {
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glClearColor(BackgroundColor.x, BackgroundColor.y, BackgroundColor.z, BackgroundColor.w);
        
        float currentFrameTime = glfwGetTime();
        DeltaTime = currentFrameTime - LastFrameTime;
        LastFrameTime = currentFrameTime;

        OnTick(DeltaTime);

        // GetCurrentContext == nullptr说明没有初始化GUI
        if (OnGUI && ImGui::GetCurrentContext() != nullptr)
        {
            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            OnGUI(DeltaTime);

            // Rendering
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        glfwSwapBuffers(__GlobalWindow);
        glfwPollEvents();

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / FPS));
    }

    return EXIT_SUCCESS;
}

void OnKeyboardEventDefault(GLFWwindow* InWindow, int Key, int ScanCode, int Action, int Mods)
{
    if (glfwGetKey(InWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(InWindow, GLFW_TRUE);
    }
}