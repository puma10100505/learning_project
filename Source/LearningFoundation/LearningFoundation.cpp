#include "LearningFoundation.h"

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
    // TODO:
}

void FramebufferChangedDefault(GLFWwindow* Window, int Width, int Height) {
    // TODO:
    glViewport(0, 0, Width, Height);
}

void OnKeyEventDefault(GLFWwindow* Window, int Key, int ScanCode, int Action, int Mods) {
    // TODO:
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
    int32_t FrameInterval, GLFWerrorfun GlfwErrCallback, GLFWframebuffersizefun FrameBufferSizeChanged, GLFWkeyfun KeyEventCallback) 
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

    if (bWithGUI)
    {
        if ((iRet = GLInitGUI()) < 0) {
            printf("Init imgui failed, iRet: %d\n", iRet);
        }
    }

    FPS = 1000 / FrameInterval;

    return EXIT_SUCCESS;    
}

int GLCreateWindow(const FCreateWindowParameters& Params)
{
    return GLCreateWindow(Params.InitWidth, Params.InitHeight, Params.Title,
        Params.bHideCursor, Params.bWithGUI, Params.FrameInterval, Params.GlfwErrCallback, 
        Params.FrameBufferSizeChanged, Params.KeyEventCallback);
}

void GLDestroyWindow() {
    glfwTerminate();
}

void GLDestroyGUI() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

int GLWindowTick(std::function<void (float)> OnTick, std::function<void (float)> OnGUI) {
    
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

        // GetCurrentContext == nullptr说明没有初始化GUI
        if (OnGUI && ImGui::GetCurrentContext() != nullptr)
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