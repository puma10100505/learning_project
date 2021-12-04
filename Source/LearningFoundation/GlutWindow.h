#pragma once

#include <iostream>
#include <memory>
#include <string>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glut.h"
#include "imgui/imgui_impl_opengl2.h"

typedef int FGlutWindowHandle;
typedef void(*GlutWindowCallbackFunc)();
typedef void(*GlutWindowInputCallbackFunc)(unsigned char cChar, int nMouseX, int nMouseY);
typedef void(*GlutWindowResizeCallbackFunc)(int Width, int Height);

class GlutWindow
{
private:
    GlutWindow(int InArgc, char** InArgv, const char* InTitle, 
        int InWidth, int InHeight, bool InShowGUI);

public:
    static GlutWindow* GetInstance(int InArgc, char** InArgv, const char* InTitle, 
        int InWidth, int InHeight, bool InShowGUI)
    {
        if (Inst == nullptr)
        {
            Inst = new GlutWindow(InArgc, InArgv, InTitle, InWidth, InHeight, InShowGUI);
        }

        return Inst;
    } 

    int Show();

protected:
    void Initialize(int InArgc, char** InArgv);
    
    static void InternalUpdate();
    static void InternalIdle();
    static void InternalEntry(int State);
    static void InternalResize(int nWidth, int nHeight);
    static void InternalInput(unsigned char cChar, int nMouseX, int nMouseY);
    
private:
    static FGlutWindowHandle WinHandle;
    int WindowWidth;
    int WindowHeight;
    std::string WindowTitle;
    bool bShowGUI;
    static ImVec4 WindowBackgroundColor;
    static GlutWindow* Inst; 

public:
    static GlutWindowCallbackFunc OnDrawCallback;
    static GlutWindowCallbackFunc OnGUICallback;
    static GlutWindowInputCallbackFunc OnInputCallback;
    static GlutWindowResizeCallbackFunc OnResizeCallback;
};
