#include "GlutWindow.h"

#include <glm/ext/quaternion_common.hpp>

#include "GL/freeglut.h"
#include "PxPhysicsAPI.h"
#include "SceneManager.h"

#include "NetImgui_Api.h"

#include "imgui/imgui_internal.h"

using namespace physx;

static int32_t sClientPort = 8889;
static int32_t sServerPort = 8880;
static std::string sServerHostname = "127.0.0.1";
static bool sbShowDemoWindow = false;

GlutWindow* GlutWindow::Inst = nullptr;
ImVec4 GlutWindow::WindowBackgroundColor = {0.45f, 0.55f, 0.6f, 1.f};
FGlutWindowHandle GlutWindow::WinHandle = 0;
bool GlutWindow::bUseGUI = true;
bool GlutWindow::bUseDarkStyle = true;
int GlutWindow::LastUpdateTimeInMs = 0;
float GlutWindow::DeltaTimeInSeconds = 0.f;
int GlutWindow::FPS = 60;
int GlutWindow::WindowWidth = 1280;
int GlutWindow::WindowHeight = 720;
std::vector<int> GlutWindow::SelectedActorIndices;
int GlutWindow::LastMouseX = 0;
int GlutWindow::LastMouseY = 0;

SceneManager* GlutWindow::SceneManagerPtr = SceneManager::GetInstance(
    GlutWindow::WindowWidth, GlutWindow::WindowHeight);

GlutWindowDrawCallbackFunc GlutWindow::OnDrawCallback = nullptr;
GlutWindowGUICallbackFunc GlutWindow::OnGUICallback = nullptr;
GlutWindowInputCallbackFunc GlutWindow::OnInputCallback = nullptr;
GlutWindowResizeCallbackFunc GlutWindow::OnResizeCallback = nullptr;

static void ClientUtil_ImGuiContent_Common(const char* zAppName)
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3,6) );
	if( ImGui::BeginMainMenuBar() )
	{
		ImGui::AlignTextToFramePadding();
		ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "%s", zAppName);
		ImGui::SameLine(0,32);
		
		//-----------------------------------------------------------------------------------------
		if( NetImgui::IsConnected() )
		//-----------------------------------------------------------------------------------------
		{
			ImGui::TextUnformatted("Status: Connected");
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3, 3));		    
			ImGui::SetCursorPosY(3);
			if( ImGui::Button("Disconnect", ImVec2(120,0)) ) 
			{
				NetImgui::Disconnect();
			}
			ImGui::PopStyleVar();
		}
		
		//-----------------------------------------------------------------------------------------
		else if( NetImgui::IsConnectionPending() )
		//-----------------------------------------------------------------------------------------
		{
			ImGui::TextUnformatted("Status: Waiting Server");
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3, 3));
			ImGui::SetCursorPosY(3);
			if (ImGui::Button("Cancel", ImVec2(120,0)))
			{
				NetImgui::Disconnect();
			}
			ImGui::PopStyleVar();
		}

		//-----------------------------------------------------------------------------------------
		else // No connection
		//-----------------------------------------------------------------------------------------
		{
			//-------------------------------------------------------------------------------------
			if( ImGui::BeginMenu("[ Connect to ]") )
			//-------------------------------------------------------------------------------------
			{
				ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Server Settings");
				ImGui::InputText("Hostname", (char*)(sServerHostname.c_str()), sServerHostname.length());
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Address of PC running the netImgui server application. Can be an IP like 127.0.0.1");
				ImGui::InputInt("Port", &sServerPort);
				ImGui::NewLine();
				ImGui::Separator();
				if (ImGui::Button("Connect", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				{
					NetImgui::ConnectToApp(zAppName, sServerHostname.c_str(), sServerPort);
				}
				ImGui::EndMenu();
			}

			if( ImGui::IsItemHovered() )
				ImGui::SetTooltip("Attempt a connection to a remote netImgui server at the provided address.");

			//-------------------------------------------------------------------------------------
			if (ImGui::BeginMenu("[  Wait For  ]"))
			//-------------------------------------------------------------------------------------
			{
				ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "Client Settings");				
				ImGui::InputInt("Port", &sClientPort);
				ImGui::NewLine();
				ImGui::Separator();
				if (ImGui::Button("Listen", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				{
					NetImgui::ConnectFromApp(zAppName, sClientPort);
				}
				ImGui::EndMenu();
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Start listening for a connection request by a remote netImgui server, on the provided Port.");
		}
		
		ImGui::SameLine(0,40);	
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8,0.8,0.8,0.9) );
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3, 3));				
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, sbShowDemoWindow ? 1.f : 0.f);
		ImGui::SetCursorPosY(3);
	    if( ImGui::Button("Show ImGui Demo", ImVec2(120,0)) )
	    {
	        sbShowDemoWindow = !sbShowDemoWindow;
	    }	
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);
		ImGui::EndMainMenuBar();
	}
	ImGui::PopStyleVar();

    if( sbShowDemoWindow )
    {
        ImGui::ShowDemoWindow(&sbShowDemoWindow);
    }
}

GlutWindow* GlutWindow::GetInstance(int InArgc, char** InArgv, const char* InTitle, 
        int InWidth, int InHeight, bool InShowGUI)
{
    if (Inst == nullptr)
    {
        Inst = new GlutWindow(InArgc, InArgv, InTitle, InWidth, InHeight, InShowGUI);
    }

    return Inst;
} 

GlutWindow* GlutWindow::GetInstance() 
{
    return Inst;
}

GlutWindow::GlutWindow(int InArgc, char** InArgv, const char* InTitle, 
    int InWidth, int InHeight, bool InShowGUI)
    : WindowTitle(InTitle)
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

void GlutWindow::UseGUI(bool InShow)
{
    bUseGUI = InShow;
}

void GlutWindow::InternalIdle()
{
    glutPostRedisplay();
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

    if (SceneManagerPtr != nullptr && SceneManagerPtr->GetCamera())
    {
        SceneManagerPtr->GetCamera()->handleKey(cChar, CurrentMouseX, CurrentMouseY);
    }
}

void GlutWindow::InternalResize(int nWidth, int nHeight)
{
    if (bUseGUI)
    {
        ImGui_ImplGLUT_ReshapeFunc(nWidth, nHeight);
    }

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

float GlutWindow::ElpsedTimeInSeconds()
{
    return glutGet(GLUT_ELAPSED_TIME) / 1000.f;
}

int GlutWindow::Show()
{
    WinHandle = glutCreateWindow(WindowTitle.c_str());
    if (WinHandle < 1) 
    {
        printf("create window failed\n");
        return EXIT_FAILURE;
    }

    glutDisplayFunc(&GlutWindow::InternalUpdate);
    glutIdleFunc(&GlutWindow::InternalIdle);
    glutReshapeFunc(&GlutWindow::InternalResize);
    glutEntryFunc(&GlutWindow::InternalEntry);
    glutKeyboardFunc(&GlutWindow::InternalInput);

    glutMotionFunc(&GlutWindow::InternalMotion);
    glutPassiveMotionFunc(&GlutWindow::InternalPassiveMotion);
    glutMouseFunc(&GlutWindow::InternalMouse);
#ifdef __FREEGLUT_EXT_H__
    glutMouseWheelFunc(&GlutWindow::InternalWheel);
#endif
    glutKeyboardUpFunc(&GlutWindow::InternalKeyboardUp);
    glutSpecialFunc(&GlutWindow::InternalSpecial);
    glutSpecialUpFunc(&GlutWindow::InternalSpecialUp);

    if (SceneManagerPtr && SceneManagerPtr->GetCamera())
    {
        SceneManagerPtr->GetCamera()->handleMotion(0, 0);
    }

    if (bUseGUI)
    {
        IMGUI_CHECKVERSION();
        ImGui::SetCurrentContext( ImGui::CreateContext() );
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        // Setup Dear ImGui style
        if (bUseDarkStyle)
        {
            ImGui::StyleColorsDark();
        }
        else
        {
            ImGui::StyleColorsLight();
        }

        ImGui_ImplGLUT_Init();
        ImGui_ImplOpenGL2_Init();

        if (!NetImgui::Startup())
        {
            printf("create net imgui failed\n");
            return EXIT_FAILURE;
        }
    }

    glutMainLoop();

    Destroy();

    return 0;
}

void GlutWindow::Destroy() 
{
    if (bUseGUI && ImGui::GetCurrentContext())
    {
        ImGui_ImplOpenGL2_Shutdown();
        ImGui_ImplGLUT_Shutdown();
        ImGui::DestroyContext();

        NetImgui::Shutdown(true);
    }
    
    if (SceneManagerPtr)
    {
        delete SceneManagerPtr;
        SceneManagerPtr = nullptr;
    }
}

void GlutWindow::UpdateDeltaTime()
{
    int CurrentTimeMs = glutGet(GLUT_ELAPSED_TIME);
    int DeltaTimeMs = (glutGet(GLUT_ELAPSED_TIME) - LastUpdateTimeInMs);
    if (DeltaTimeMs <= 0)
    {
        DeltaTimeMs = 1;
    }

    DeltaTimeInSeconds = DeltaTimeMs / 1000.0f;
    LastUpdateTimeInMs = CurrentTimeMs;
}

void GlutWindow::InternalUpdate()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(WindowBackgroundColor.x, WindowBackgroundColor.y, WindowBackgroundColor.z, WindowBackgroundColor.w);

    UpdateDeltaTime(); 

    if (SceneManagerPtr) 
    {
        SceneManagerPtr->Update(DeltaTimeInSeconds);
    }

    // 先绘制图形
    if (OnDrawCallback)
    {
        OnDrawCallback(DeltaTimeInSeconds);
    }

    // 再绘制GUI
    if (bUseGUI && OnGUICallback)
    {
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGLUT_NewFrame();

        ClientUtil_ImGuiContent_Common("Learning Project");

        OnGUICallback(DeltaTimeInSeconds);

        ImGui::Render();

        ImDrawData* Client_Draw_Content = (!NetImgui::IsConnected() ? ImGui::GetDrawData() : nullptr);

        if (Client_Draw_Content)
        {
            ImGui_ImplOpenGL2_RenderDrawData(Client_Draw_Content);
        }
    }

    glutSwapBuffers();
    glutPostRedisplay();

    std::this_thread::sleep_for(std::chrono::milliseconds(1000 / FPS));
}

void GlutWindow::InternalMotion(int x, int y)
{
    bool IsInAnyWindowArea = false;

    if (bUseGUI)
    {
        if (GImGui)
        {
            // 在窗口范围内拖动事件传递到窗口的处理(可使用NetImGui避免)
            // ImGuiContext& g = *GImGui;
            //
            // for (int i = 0; i < g.Windows.size(); i++)
            // {
            //     if (const ImGuiWindow* WinPtr = g.Windows[i]) 
            //     {
            //         IsInAnyWindowArea = (x > WinPtr->Pos.x && x < WinPtr->Pos.x + WinPtr->Size.x) && 
            //             (y > WinPtr->Pos.y && y < WinPtr->Pos.y + WinPtr->Size.y);
            //     }
            //
            //     if (IsInAnyWindowArea) break;
            // }

            ImGui_ImplGLUT_MotionFunc(x, y);
        }
    }

    // if (!IsInAnyWindowArea)
    // {
    //     if (SceneManagerPtr && SceneManagerPtr->GetCamera())
    //     {
    //         SceneManagerPtr->GetCamera()->handleMotion(x, y);
    //     }
    // }

    SceneManagerPtr->GetCamera()->handleMotion(CurrentMouseX, CurrentMouseY);
}

void GlutWindow::InternalPassiveMotion(int x, int y)
{
    if (bUseGUI)
    {
        ImGui_ImplGLUT_MotionFunc(x, y);
    }
}

glm::vec3 GlutWindow::ScreenToWorldLocation(int x, int y)
{
    GLint Viewport[4];
    GLdouble ModelView[16];
    GLdouble Projection[16];
    GLfloat WinX, WinY, WinZ;
    GLdouble PosX, PosY, PosZ;

    glGetDoublev(GL_MODELVIEW_MATRIX, ModelView);
    glGetDoublev(GL_PROJECTION_MATRIX, Projection);
    glGetIntegerv(GL_VIEWPORT, Viewport);

    WinX = (float)x;
    WinY = (float)Viewport[3] - (float)y;
    glReadPixels(WinX, WinY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &WinZ);
    gluUnProject(WinX, WinY, WinZ, ModelView, Projection, Viewport, &PosX, &PosY, &PosZ);

    return { (float)PosX, (float)PosY, (float)PosZ };
}

void GlutWindow::InternalMouse(int glut_button, int state, int x, int y)
{
    if (bUseGUI)
    {
        ImGui_ImplGLUT_MouseFunc(glut_button, state, x, y);
    }

    if (SceneManagerPtr && SceneManagerPtr->GetCamera())
    {
        SceneManagerPtr->GetCamera()->handleMouse(glut_button, state, x, y);
    }

    if (state == 0)
    {
        // Press
    }
    else if (state == 1)
    {
        // Release
        glm::vec3 WorldPos = ScreenToWorldLocation(x, y);
        printf("mouse clicked, button: %d, x: %d, y: %d -> (%f, %f, %f)\n", 
            glut_button, x, y, WorldPos.x, WorldPos.y, WorldPos.z);

        PxVec3 EndPt = { WorldPos.x, WorldPos.y, WorldPos.z};
        PxVec3 BeginPt = GetScene()->GetCamera()->getEye();

        physx::PxHitBuffer<PxRaycastHit> HitBuff;
        bool bIsBlocked = GetScene()->GetPhysics()->GetPhysicsScene()->raycast(BeginPt, (EndPt - BeginPt).getNormalized(), 10000.f, HitBuff);
        printf("bIsBlocked: %d, nbTouches: %u, hasBlock: %d\n", 
            bIsBlocked, HitBuff.nbTouches, HitBuff.hasBlock);

        if (bIsBlocked)
        {
            if (HitBuff.block.actor)
            {
                GameObject* Info = static_cast<GameObject*>(HitBuff.block.actor->userData);
                if (Info)
                {
                    printf("Block something named: %s, index: %d\n", Info->Name.c_str(), Info->ActorIndex);
                    //GlutWindow::SelectedActorIndices.push_back(Info->ActorIndex);
                }
            }

            for (int i = 0; i < HitBuff.nbTouches; i++)
            {
                const PxRaycastHit& HitItem = HitBuff.getTouch(i);
                printf("hit item, dist: %f, pos: (%f, %f, %f)\n", HitItem.distance, HitItem.position.x, 
                    HitItem.position.y, HitItem.position.z);
                GameObject* Info = static_cast<GameObject*>(HitItem.actor->userData);
                if (Info)
                {
                    printf("Hit something named: %s\n", Info->Name.c_str());
                }
                else
                {
                    printf("can not convert to GameObject\n");
                }
            }
        }
    }
}

void GlutWindow::InternalWheel(int button, int dir, int x, int y)
{
    if (bUseGUI)
    {
        ImGui_ImplGLUT_MouseWheelFunc(button, dir, x, y);
    }
}

void GlutWindow::InternalKeyboardUp(unsigned char c, int x, int y)
{
    if (bUseGUI)
    {
        ImGui_ImplGLUT_KeyboardUpFunc(c, x, y);
    }
}

void GlutWindow::InternalSpecial(int key, int x, int y)
{
    if (bUseGUI)
    {
        ImGui_ImplGLUT_SpecialFunc(key, x, y);
    }
}

void GlutWindow::InternalSpecialUp(int key, int x, int y)
{
    if (bUseGUI)
    {
        ImGui_ImplGLUT_SpecialUpFunc(key, x, y);
    }
}
