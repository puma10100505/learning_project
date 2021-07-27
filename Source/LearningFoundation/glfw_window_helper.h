#pragma once

#include <chrono>
#include <thread>
#include <cstdio>
#include <iostream>
#include <functional>

// #include "learning/learning.h"

const int32_t WINDOW_WIDTH = 1280;
const int32_t WINDOW_HEIGHT = 720;
const std::string title = "Sample Window";
static int32_t FPS = 60;
static glm::vec4 BackgroundColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

static float WINDOW_RATIO = (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT;

static float LastFrameTime = 0.0f;
static float DeltaTime = 0.0f;

static GLFWwindow* __global_window = nullptr;

static GLFWwindow* get_global_window() {
    return __global_window;
}


static inline void on_glfw_error_default(int code, const char* desc) {
    // TODO:
}

static inline void on_frame_buffer_size_changed_default(GLFWwindow* window, int width, int height) {
    // TODO:
    glViewport(0, 0, width, height);
}

static inline void on_key_event_default(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // TODO:
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(get_global_window(), true);
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
    ImGui_ImplGlfw_InitForOpenGL(get_global_window(), true);
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

    __global_window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
    if (!__global_window) {
        glfwTerminate();
        std::cout << "gl_create_window create window failed\n";
        return -1;
    }

    glfwMakeContextCurrent(__global_window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(1);
    
    // set some callback func
    glfwSetErrorCallback(err_handler);
    glfwSetFramebufferSizeCallback(get_global_window(), fbs_handler);
    glfwSetKeyCallback(get_global_window(), key_handler);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    if (hide_cursor) {
        // Diable the mouse cursor when startup
        glfwSetInputMode(get_global_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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
    std::function<void (void)> on_gui = gl_on_gui_default) {
    
    while (!glfwWindowShouldClose(__global_window)) {
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

        glfwSwapBuffers(__global_window);
        glfwPollEvents();

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / FPS));
    }

    return 0;
}