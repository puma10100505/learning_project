#pragma once
#pragma warning (disable:4819)
// 下面这行可以隐藏FreeGlut或Glfw的Console窗口
#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )

#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <thread>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glut.h"
#include "imgui/imgui_impl_opengl2.h"

typedef int FGlutWindowHandle;
typedef void(*GlutWindowDrawCallbackFunc)(float);
typedef void(*GlutWindowGUICallbackFunc)(float);
typedef void(*GlutWindowInputCallbackFunc)(unsigned char cChar, int nMouseX, int nMouseY);
typedef void(*GlutWindowResizeCallbackFunc)(int Width, int Height);

class GlutWindow
{
private:
    GlutWindow(int InArgc, char** InArgv, const char* InTitle, 
        int InWidth, int InHeight, bool InShowGUI);

public:
    static GlutWindow* GetInstance(int InArgc, char** InArgv, const char* InTitle, 
        int InWidth, int InHeight, bool InShowGUI);

    static GlutWindow* GetInstance();

    int Show();

    float ElpsedTimeInSeconds();
    
    static void UseGUI(bool InShow);

protected:
    void Initialize(int InArgc, char** InArgv);
    
    static void InternalUpdate();
    static void InternalIdle();
    static void InternalEntry(int State);
    static void InternalResize(int nWidth, int nHeight);
    static void InternalInput(unsigned char cChar, int nMouseX, int nMouseY);
    static void InternalMotion(int x, int y);
    static void InternalPassiveMotion(int x, int y);
    static void InternalMouse(int glut_button, int state, int x, int y);
    static void InternalWheel(int button, int dir, int x, int y);
    static void InternalKeyboardUp(unsigned char c, int x, int y);
    static void InternalSpecial(int key, int x, int y);
    static void InternalSpecialUp(int key, int x, int y);

    static void UpdateDeltaTime();

    
private:
    static FGlutWindowHandle WinHandle;
    int WindowWidth;
    int WindowHeight;
    std::string WindowTitle;    
    static GlutWindow* Inst;
    static int LastUpdateTimeInMs;
    static float DeltaTimeInSeconds;

public:
    static GlutWindowDrawCallbackFunc OnDrawCallback;
    static GlutWindowGUICallbackFunc OnGUICallback;
    static GlutWindowInputCallbackFunc OnInputCallback;
    static GlutWindowResizeCallbackFunc OnResizeCallback;

    static bool bUseDarkStyle;
    static bool bUseGUI;
    static ImVec4 WindowBackgroundColor;
    static int FPS;
};
