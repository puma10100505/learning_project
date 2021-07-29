#pragma once

#include <chrono>
#include <thread>
#include <cstdio>
#include <iostream>
#include <functional>


static bool bIsFullScreen = false;

static int FullScreenWidth = 0;
static int FullScreenHeight = 0;

static int ScreenWidth = 1080;
static int ScreenHeight = 720;

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const std::string title = "Sample Window";
static int32_t FPS = 60;
static glm::vec4 BackgroundColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

static float WINDOW_RATIO = (float)ScreenWidth/(float)ScreenHeight;

static float LastFrameTime = 0.0f;
static float DeltaTime = 0.0f;

static GLFWwindow* __GlobalWindow = nullptr;
static GLFWmonitor* __PrimaryMonitor = nullptr;

static GLFWwindow* GetGlobalWindow() {
    return __GlobalWindow;
}

static GLFWmonitor* GetPrimaryMonitor()
{
    return __PrimaryMonitor;
}

// @Old Version
static inline void on_glfw_error_default(int code, const char* desc) 
{
    // TODO:
}

static inline void on_frame_buffer_size_changed_default(GLFWwindow* window, int width, int height) {
    // TODO:
    glViewport(0, 0, width, height);
}

static inline void on_key_event_default(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // TODO:
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(GetGlobalWindow(), true);
    }
}

static inline int gl_init_window() {
    if (!glfwInit()) {
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    return 0;
}

static inline int gl_init_gui() {
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

static inline void gl_on_gui_default() {
    
}

// create glfw window
static inline int gl_create_window(int32_t width = WINDOW_WIDTH, int32_t height = WINDOW_HEIGHT, 
    const std::string& title = "Sample Window", bool hide_cursor = false, int32_t frameInterval = 60,
    GLFWerrorfun err_handler = on_glfw_error_default, 
    GLFWframebuffersizefun fbs_handler = on_frame_buffer_size_changed_default, 
    GLFWkeyfun key_handler = on_key_event_default) {

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

static inline void gl_destroy_window() {
    glfwTerminate();
}

static inline void gl_destroy_gui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

static inline int gl_window_loop(
    std::function<void (void)> on_update,
    std::function<void (void)> on_gui) {
    
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

static inline void OnGlfwErrorDefault(int Code, const char* Desc)
{
    // TODO:
}

static inline void FramebufferChangedDefault(GLFWwindow* Window, int Width, int Height) {
    // TODO:
    glViewport(0, 0, Width, Height);
}

static inline void OnKeyEventDefault(GLFWwindow* Window, int Key, int ScanCode, int Action, int Mods) {
    // TODO:
    if (Key == GLFW_KEY_ESCAPE && Action == GLFW_PRESS) {
        glfwSetWindowShouldClose(GetGlobalWindow(), true);
    }
}

static void WindowCloseCallback(GLFWwindow* InWindow)
{
    printf("Window will be closed...\n");
}

static void WindowResizedCallback(GLFWwindow* window, int width, int height)
{
    printf("Window has been resized, width: %d, height: %d\n", width, height);
}

static inline int InitGlfwWindow() {
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

static inline int GLInitGUI() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(GetGlobalWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 330");

    return 0;
}

typedef struct CreateWindowParameters
{
public:
    int InitWidth = ScreenWidth;
    int InitHeight = ScreenHeight;
    const std::string& Title = "Sample Window";
    bool bHideCursor = false;
    int FrameInterval = 60;
    GLFWkeyfun KeyEventCallback = OnKeyEventDefault;
    GLFWerrorfun GlfwErrCallback = OnGlfwErrorDefault;
    GLFWframebuffersizefun FrameBufferSizeChanged = FramebufferChangedDefault; 

    CreateWindowParameters(
        int W, 
        int H, 
        const std::string& T, 
        bool bHideC, 
        int FI, 
        GLFWkeyfun KeyFunc)
            :InitWidth(W), 
            InitHeight(H), 
            Title(T), 
            bHideCursor(bHideC), 
            FrameInterval(FI), 
            KeyEventCallback(KeyFunc)
        {

        } 
} FCreateWindowParameters;

// create glfw window
static inline int GLCreateWindow(int InitWidth = ScreenWidth, int InitHeight = ScreenHeight, 
    const std::string& Title = "Sample Window", bool bHideCursor = false, int32_t FrameInterval = 60,
    GLFWerrorfun GlfwErrCallback = OnGlfwErrorDefault, 
    GLFWframebuffersizefun FrameBufferSizeChanged = FramebufferChangedDefault, 
    GLFWkeyfun KeyEventCallback = OnKeyEventDefault) {

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
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(1);
    
    // set some callback func
    glfwSetErrorCallback(GlfwErrCallback);
    glfwSetFramebufferSizeCallback(__GlobalWindow, FrameBufferSizeChanged);
    glfwSetKeyCallback(__GlobalWindow, KeyEventCallback);
    glfwSetWindowCloseCallback(__GlobalWindow, WindowCloseCallback);
    glfwSetWindowSizeCallback(__GlobalWindow, WindowResizedCallback);

    glfwFocusWindow(__GlobalWindow);

    // 创建窗口之后，使用OPENGL之前要初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to init GLAD" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    if (bHideCursor) {
        // Diable the mouse cursor when startup
        glfwSetInputMode(GetGlobalWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    FPS = 1000 / FrameInterval;

    return EXIT_SUCCESS;    
}

static inline int GLCreateWindow(const FCreateWindowParameters& Params)
{
    return GLCreateWindow(Params.InitWidth, Params.InitHeight, Params.Title,
        Params.bHideCursor, Params.FrameInterval, Params.GlfwErrCallback, 
        Params.FrameBufferSizeChanged, Params.KeyEventCallback);
}

static inline void GLDestroyWindow() {
    glfwTerminate();
}

static inline void GLDestroyGUI() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

static inline int GLWindowTick(
    std::function<void (float)> OnTick,
    std::function<void (float)> OnGUI) {
    
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

        if (OnGUI)
        {
            OnGUI(DeltaTime);
        }

        OnTick(DeltaTime);

        // Rendering
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(__GlobalWindow);
        glfwPollEvents();

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / FPS));
    }

    return 0;
}