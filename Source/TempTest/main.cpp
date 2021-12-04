#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#endif

#define USE_GLUT


#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

#include "learning.h"
#include "LearningStatics.h"
#include "CommonDefines.h"
#include "GlutWindow.h"

static void OnCustomGUI()
{
    ImGui::Begin("Hello glut gui");
    if (ImGui::Button("Button")) {}
    ImGui::End();
}

static void OnCustomTick()
{
    printf("entry of CustomTick\n");
    //const GLfloat time = glutGet(GLUT_ELAPSED_TIME) / 1000.f * 40;

    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();

    //glRotatef( time, 0, 0, 1 );
    //glRotatef( time, 0, 1, 0 );
    //glRotatef( time, 1, 0, 0 );

    /*
        * And then drawn...
        */
    glColor3f( 1, 0, 0 );
    /* glutWireCube( 20.0 ); */
    //glutWireTeapot( 20.0 );
    glutSolidCube(5.f);
    /* glutWireSphere( 15.0, 15, 15 ); */
    /* glColor3f( 0, 1, 0 ); */
    /* glutWireCube( 30.0 ); */
    /* glutSolidCone( 10, 20, 10, 2 ); */

    /*
        * Don't forget about the model-view matrix
        */
    glPopMatrix( );

}

int main(int argc, char** argv)
{
    //FGGlobalWindowContext.OnGUI = OnCustomGUI;
    //FGGlobalWindowContext.OnTick = OnCustomTick;

    // printf("FGGlobalWindowContext.OnTick...: %d\n", FGGlobalWindowContext.OnTick == nullptr);

    // FGInitWindow(argc, argv, ScreenWidth, ScreenHeight);
    // FGCreateWindow("test freeglut window", true);
    // FGDestroyWindow();

    //std::unique_ptr<GlutWindow> glutWindow = std::make_unique<GlutWindow>(argc, argv, "Window Title", 1280, 720, true);
    // GlutWindow Win(argc, argv, "Window Title", 1280, 720, true);

    
    GlutWindow::OnDrawCallback = OnCustomTick;
    GlutWindow::OnGUICallback = OnCustomGUI;
    GlutWindow::GetInstance(argc, argv, "Window Title", 1280, 720, true)->Show();
    
    return 0;
}