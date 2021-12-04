#pragma once

#include <chrono>
#include <thread>
#include <cstdio>
#include <iostream>
#include <functional>

#include "glm/glm.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

extern glm::vec4 BackgroundColor;
extern float DeltaTime;

static bool bIsFullScreen = false;
static int FullScreenWidth = 0;
static int FullScreenHeight = 0;
static int ScreenWidth = 1280;
static int ScreenHeight = 900;
static const int WINDOW_WIDTH = 1280;
static const int WINDOW_HEIGHT = 900;
const std::string title = "Sample Window";
static int32_t FPS = 60;
static float WINDOW_RATIO = (float)ScreenWidth/(float)ScreenHeight;
static float LastFrameTime = 0.0f;

static GLFWwindow* __GlobalWindow = nullptr;
static GLFWmonitor* __PrimaryMonitor = nullptr;

void OnKeyboardEventDefault(GLFWwindow* InWindow, int Key, int ScanCode, int Action, int Mods);

typedef struct CreateWindowParameters
{
public:
    int InitWidth = ScreenWidth;
    int InitHeight = ScreenHeight;
    const std::string& Title = "Sample Window";
    bool bHideCursor = false;
    bool bWithGUI = true;
    int FrameInterval = 60;
    GLFWkeyfun KeyEventCallback;
    GLFWerrorfun GlfwErrCallback;
    GLFWframebuffersizefun FrameBufferSizeChanged;
    GLFWcursorposfun MouseMoveCallback;
    GLFWscrollfun MouseScrollCallback;
    GLFWmousebuttonfun MouseButtonCallback;

    CreateWindowParameters(
        int W, 
        int H, 
        const std::string& T, 
        bool bHideC, 
        bool InWithGUI,
        int FI)
            :InitWidth(W), 
            InitHeight(H), 
            Title(T), 
            bHideCursor(bHideC), 
            bWithGUI(InWithGUI),
            FrameInterval(FI)
        {
            KeyEventCallback = nullptr;
            GlfwErrCallback = nullptr;
            FrameBufferSizeChanged = nullptr;
            MouseMoveCallback = nullptr;
            MouseScrollCallback = nullptr;
            MouseButtonCallback = nullptr;
        } 

    static struct CreateWindowParameters DefaultWindowParameters();
    static struct CreateWindowParameters WindowBySizeParameters(int W, int H);


} FCreateWindowParameters;

static void OnGlfwErrorDefault(int Code, const char* Desc);
static void FramebufferChangedDefault(GLFWwindow* Window, int Width, int Height);
static void OnKeyEventDefault(GLFWwindow* Window, int Key, int ScanCode, int Action, int Mods);
static void WindowCloseCallback(GLFWwindow* InWindow);
static void WindowResizedCallback(GLFWwindow* window, int width, int height);

int InitGlfwWindow();
int GLInitGUI();
GLFWwindow* GetGlobalWindow();
GLFWmonitor* GetPrimaryMonitor();
void GLDestroyWindow();
void GLDestroyGUI();
int GLWindowTick(std::function<void (float)> OnTick, std::function<void (float)> OnGUI);

int GLCreateWindow(const FCreateWindowParameters& Params);
int GLCreateWindow(int InitWidth = ScreenWidth, int InitHeight = ScreenHeight, 
    const std::string& Title = "Sample Window", bool bHideCursor = false, bool bWithGUI = true, int32_t FrameInterval = 60,
    GLFWerrorfun GlfwErrCallback = nullptr, 
    GLFWframebuffersizefun FrameBufferSizeChanged = nullptr, 
    GLFWkeyfun KeyEventCallback = nullptr, 
    GLFWcursorposfun MouseMoveCallback = nullptr,
    GLFWscrollfun MouseScrollCallback = nullptr, 
    GLFWmousebuttonfun MouseButtonCallback = nullptr);