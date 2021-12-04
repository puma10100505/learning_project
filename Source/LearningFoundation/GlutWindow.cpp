#include "GlutWindow.h"
#include "GL/freeglut.h"

GlutWindow* GlutWindow::Inst = nullptr;
ImVec4 GlutWindow::WindowBackgroundColor = {0.45f, 0.55f, 0.6f, 1.f};
FGlutWindowHandle GlutWindow::WinHandle = 0;

GlutWindowCallbackFunc GlutWindow::OnDrawCallback = nullptr;
GlutWindowCallbackFunc GlutWindow::OnGUICallback = nullptr;
GlutWindowInputCallbackFunc GlutWindow::OnInputCallback = nullptr;
GlutWindowResizeCallbackFunc GlutWindow::OnResizeCallback = nullptr;


GlutWindow::GlutWindow(int InArgc, char** InArgv, const char* InTitle, 
    int InWidth, int InHeight, bool InShowGUI)
    : WindowTitle(InTitle), WindowWidth(InWidth), WindowHeight(InHeight), bShowGUI(InShowGUI)
{
    WindowBackgroundColor = {0.45f, 0.55f, 0.6f, 1.f};
    Initialize(InArgc, InArgv);
}

void GlutWindow::Initialize(int InArgc, char** InArgv) 
{
    glutInit(&InArgc, InArgv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_MULTISAMPLE);
    glutInitWindowSize(WindowWidth, WindowHeight);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE,GLUT_ACTION_GLUTMAINLOOP_RETURNS);
}

void GlutWindow::InternalIdle()
{
    glutPostWindowRedisplay(WinHandle);
}

void GlutWindow::InternalEntry(int State)
{
    int window = glutGetWindow () ;
    printf ( "Window %d Entry Callback: %d\n", window, State ) ;
}

void GlutWindow::InternalInput(unsigned char cChar, int nMouseX, int nMouseY)
{
    if (OnInputCallback)
    {
        OnInputCallback(cChar, nMouseX, nMouseY);
    }
}

void GlutWindow::InternalResize(int nWidth, int nHeight)
{
    GLfloat fAspect = (GLfloat) nHeight / (GLfloat) nWidth;
    GLfloat fPos[ 4 ] = { 0.0f, 0.0f, 10.0f, 0.0f };
    GLfloat fCol[ 4 ] = { 0.5f, 1.0f,  0.0f, 1.0f };

    /*
     * Update the viewport first
     */
    glViewport( 0, 0, nWidth, nHeight );

    /*
     * Then the projection matrix
     */
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glFrustum( -1.0, 1.0, -fAspect, fAspect, 1.0, 80.0 );

    /*
     * Move back the camera a bit
     */
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity( );
    glTranslatef( 0.0, 0.0, -40.0f );

    /*
     * Enable some features...
     */
    glEnable( GL_CULL_FACE );
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_NORMALIZE );

    /*
     * Set up some lighting
     */
    glLightfv( GL_LIGHT0, GL_POSITION, fPos );
    glEnable( GL_LIGHTING );
    glEnable( GL_LIGHT0 );

    /*
     * Set up a sample material
     */
    glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, fCol );
}

int GlutWindow::Show()
{
    WinHandle = glutCreateWindow(WindowTitle.c_str());
    if (WinHandle < 1) 
    {
        printf("create window failed");
        return EXIT_FAILURE;
    }

    glutDisplayFunc(&GlutWindow::InternalUpdate);
    glutIdleFunc(&GlutWindow::InternalIdle);
    glutReshapeFunc(&GlutWindow::InternalResize);
    glutEntryFunc(&GlutWindow::InternalEntry);
    glutKeyboardFunc(&GlutWindow::InternalInput);

    if (bShowGUI)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        ImGui_ImplGLUT_Init();
        ImGui_ImplGLUT_InstallFuncs();
        ImGui_ImplOpenGL2_Init();
    }

    glutMainLoop();

    if (ImGui::GetCurrentContext())
    {
        ImGui_ImplOpenGL2_Shutdown();
        ImGui_ImplGLUT_Shutdown();
        ImGui::DestroyContext();
    }

    return 0;
}

void GlutWindow::InternalUpdate()
{
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGLUT_NewFrame();

    if (OnDrawCallback)
    {
        OnDrawCallback();
    }

    if (OnGUICallback)
    {
        OnGUICallback();
    }

    ImGui::Render();
    ImGuiIO& io = ImGui::GetIO();
    glViewport(0, 0, (GLsizei)io.DisplaySize.x, (GLsizei)io.DisplaySize.y);
    glClearColor(WindowBackgroundColor.x, WindowBackgroundColor.y, WindowBackgroundColor.z, WindowBackgroundColor.w);
    glClear(GL_COLOR_BUFFER_BIT);
    
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

    glutSwapBuffers();
    glutPostRedisplay();
    //glutPostWindowRedisplay(WinHandle);
}