

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#endif

#define USE_GLUT

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <gl/GL.h>

#include "GL/freeglut.h"
#include "CommonDefines.h"
#include "GlutWindow.h"
#include "glm/glm.hpp"
#include "LearningCamera.h"
#include "SceneManager.h"


static void OnCustomGUI(float DeltaTime)
{
    // TODO:    
}

int main(int argc, char** argv)
{
    GlutWindow::bUseGUI = true;
    GlutWindow::bUseDarkStyle = false;
    GlutWindow::FPS = 30;
    GlutWindow::UseNodeEditor(false);

    // GUI Logic callback
    GlutWindow::OnGUICallback = OnCustomGUI;
    
    GlutWindow::GetInstance(argc, argv, "Window Title")->Show();

    return 0;
}