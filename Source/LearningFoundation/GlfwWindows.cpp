#include "GlfwWindows.h"
#include "implot.h"
#include "imnodes.h"
#include <imgui_internal.h>
#include <imgui_node_editor.h>
#include "glad/glad.h"

namespace ed = ax::NodeEditor;


glm::vec4 BackgroundColor = glm::vec4(0.1f, 0.6f, 0.7f, 1.0f);
float DeltaTime = 0.0f;

static double       g_Time;
static bool         g_MousePressed[3];
static float        g_MouseWheel;
static GLuint       g_FontTexture;
static int          g_ShaderHandle, g_VertHandle, g_FragHandle;
static int          g_AttribLocationTex, g_AttribLocationProjMtx;
static int          g_AttribLocationPosition, g_AttribLocationUV, g_AttribLocationColor;
static unsigned int g_VboHandle, g_VaoHandle, g_ElementsHandle;

static void ImGui_RenderDrawLists(ImDrawData* draw_data)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0)
    return;
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    // Backup GL state
    GLenum last_active_texture; glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&last_active_texture);
    glActiveTexture(GL_TEXTURE0);
    GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    GLint last_sampler; glGetIntegerv(GL_SAMPLER_BINDING, &last_sampler);
    GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    GLint last_element_array_buffer; glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
    GLint last_vertex_array; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
    GLint last_polygon_mode[2]; glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
    GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
    GLenum last_blend_src_rgb; glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&last_blend_src_rgb);
    GLenum last_blend_dst_rgb; glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&last_blend_dst_rgb);
    GLenum last_blend_src_alpha; glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&last_blend_src_alpha);
    GLenum last_blend_dst_alpha; glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&last_blend_dst_alpha);
    GLenum last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&last_blend_equation_rgb);
    GLenum last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&last_blend_equation_alpha);
    GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
    GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
    GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
    GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Setup viewport, orthographic projection matrix
    glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    const float ortho_projection[4][4] =
    {
        { 2.0f / io.DisplaySize.x, 0.0f,                   0.0f, 0.0f },
        { 0.0f,                  2.0f / -io.DisplaySize.y, 0.0f, 0.0f },
        { 0.0f,                  0.0f,                  -1.0f, 0.0f },
        { -1.0f,                  1.0f,                   0.0f, 1.0f },
    };
    glUseProgram(g_ShaderHandle);
    glUniform1i(g_AttribLocationTex, 0);
    glUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
    glBindVertexArray(g_VaoHandle);
    glBindSampler(0, 0); // Rely on combined texture/sampler state.

    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
    const ImDrawList* cmd_list = draw_data->CmdLists[n];
    const ImDrawIdx* idx_buffer_offset = 0;

    glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ElementsHandle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
    {
        const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
        if (pcmd->UserCallback)
        {
            pcmd->UserCallback(cmd_list, pcmd);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
            glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
            glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
        }
        idx_buffer_offset += pcmd->ElemCount;
    }
    }

    // Restore modified GL state
    glUseProgram(last_program);
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glBindSampler(0, last_sampler);
    glActiveTexture(last_active_texture);
    glBindVertexArray(last_vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
    glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
    glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
    if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, last_polygon_mode[0]);
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
    glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
}

GLFWwindow* GetGlobalWindow() {
    return __GlobalWindow;
}

GLFWmonitor* GetPrimaryMonitor()
{
    return __PrimaryMonitor;
}


void OnGlfwErrorDefault(int Code, const char* Desc)
{
    
}

void FramebufferChangedDefault(GLFWwindow* Window, int Width, int Height) {
    glViewport(0, 0, Width, Height);
    // printf("Current viewport size, Width: %d, Height: %d\n", Width, Height);
}

void OnKeyEventDefault(GLFWwindow* Window, int Key, int ScanCode, int Action, int Mods) {
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
    ImNodes::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    ImGui_ImplGlfw_InitForOpenGL(GetGlobalWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 330");

    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    return 0;
}

// create glfw window
int GLCreateWindow(int InitWidth, int InitHeight, const std::string& Title, bool bHideCursor, bool bWithGUI, 
    int32_t FrameInterval, GLFWerrorfun GlfwErrCallback, GLFWframebuffersizefun FrameBufferSizeChanged, 
    GLFWkeyfun KeyEventCallback, GLFWcursorposfun MouseMoveCallback, 
    GLFWscrollfun MouseScrollCallback, GLFWmousebuttonfun MouseButtonCallback) 
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
    
    if (!__GlobalWindow) 
    {
        glfwTerminate();
        std::cout << "gl_create_window create window failed\n";
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(__GlobalWindow);
    glfwSwapInterval(1);

    if (FrameBufferSizeChanged)
    {
        glfwSetFramebufferSizeCallback(__GlobalWindow, FrameBufferSizeChanged);
    }
        
    if (KeyEventCallback)
    {
        glfwSetKeyCallback(__GlobalWindow, KeyEventCallback);
        printf("Finished register KeyEventCallback\n");
    }

    if (WindowCloseCallback)
    {
        glfwSetWindowCloseCallback(__GlobalWindow, WindowCloseCallback);
        printf("Finished register WindowCloseCallback\n");
    }

    if (MouseMoveCallback)
    {
        glfwSetCursorPosCallback(__GlobalWindow, MouseMoveCallback);
        printf("Finished register MouseMoveCallback\n");
    }

    if (MouseScrollCallback)
    {
        glfwSetScrollCallback(__GlobalWindow, MouseScrollCallback);
        printf("Finished register MouseScrollCallback\n");
    }

    if (MouseButtonCallback)
    {
        glfwSetMouseButtonCallback(__GlobalWindow, MouseButtonCallback);
        printf("Finished register MouseButtonCallback\n");
    }

    glfwFocusWindow(__GlobalWindow);

    // 创建窗口之后，使用OPENGL之前要初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to init GLAD" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

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

    // glViewport(0, 0, InitWidth, InitHeight);

    FPS = 1000 / FrameInterval;

    return EXIT_SUCCESS;    
}

int GLCreateWindow(const FCreateWindowParameters& Params)
{
    return GLCreateWindow(Params.InitWidth, Params.InitHeight, Params.Title,
        Params.bHideCursor, Params.bWithGUI, Params.FrameInterval, 
        Params.GlfwErrCallback, 
        Params.FrameBufferSizeChanged == nullptr ? FramebufferChangedDefault : Params.FrameBufferSizeChanged, 
        Params.KeyEventCallback, 
        Params.MouseMoveCallback, 
        Params.MouseScrollCallback, 
        Params.MouseButtonCallback);
}

void GLDestroyWindow() {
    if (ImGui::GetCurrentContext())
    {
        GLDestroyGUI();
    }

    glfwTerminate();
}


void GLDestroyGUI() 
{
    ImPlot::DestroyContext();
    ImNodes::DestroyContext();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

int GLWindowTick(std::function<void (float)> OnTick, std::function<void (float)> OnGUI)
{
    while (!glfwWindowShouldClose(__GlobalWindow)) 
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glClearColor(BackgroundColor.x, BackgroundColor.y, BackgroundColor.z, BackgroundColor.w);

        
        float currentFrameTime = glfwGetTime();
        DeltaTime = currentFrameTime - LastFrameTime;
        LastFrameTime = currentFrameTime;

        OnTick(DeltaTime);

        // GetCurrentContext == nullptr说明没有初始化GUI
        if (OnGUI && ImGui::GetCurrentContext() != nullptr)
        {
            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            OnGUI(DeltaTime);

            // Rendering
            ImGui::Render();
            
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            ImGui_RenderDrawLists(ImGui::GetDrawData());
        }

        glfwSwapBuffers(__GlobalWindow);
        glfwPollEvents();

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / FPS));
    }

    return EXIT_SUCCESS;
}

void OnKeyboardEventDefault(GLFWwindow* InWindow, int Key, int ScanCode, int Action, int Mods)
{
    if (glfwGetKey(InWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(InWindow, GLFW_TRUE);
    }
}

FCreateWindowParameters FCreateWindowParameters::DefaultWindowParameters(
        /*int32_t InWidth = 1280, int32_t InHeight = 720*/) 
{
    struct CreateWindowParameters Parameters{
        ScreenWidth, 
        ScreenHeight,
        "Default Window",
        false, 
        true, 
        30
    };
    
    Parameters.KeyEventCallback = OnKeyboardEventDefault;

    return Parameters;
}

FCreateWindowParameters FCreateWindowParameters::WindowBySizeParameters(int W, int H)
{
    struct CreateWindowParameters Parameters{
        W, H,
        "Default Window",
        false, 
        true, 
        30
    };
    Parameters.KeyEventCallback = OnKeyboardEventDefault;

    return Parameters;
}