#include "LegacyGlfwApi.h"
#include "GlfwWindows.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <chrono>
#include <thread>

int gl_init_window()
{
    return InitGlfwWindow();
}

int gl_create_window(int width, int height, const std::string& title, bool hide_cursor, int frame_interval)
{
    return GLCreateWindow(width, height, title, hide_cursor, true, frame_interval,
        nullptr, nullptr, nullptr, nullptr, nullptr);
}

int gl_create_window(int width, int height, const std::string& title, bool hide_cursor)
{
    return gl_create_window(width, height, title, hide_cursor, 60);
}

int gl_init_gui()
{
    return GLInitGUI();
}

void gl_destroy_gui()
{
    GLDestroyGUI();
}

void gl_destroy_window()
{
    GLDestroyWindow();
}

int gl_window_loop(std::function<void(void)> on_update, std::function<void(void)> on_gui)
{
    GLFWwindow* wnd = GetGlobalWindow();
    static float s_last_frame = 0.f;
    while (wnd && !glfwWindowShouldClose(wnd))
    {
        glClearColor(BackgroundColor.x, BackgroundColor.y, BackgroundColor.z, BackgroundColor.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        const float current_frame = static_cast<float>(glfwGetTime());
        DeltaTime = current_frame - s_last_frame;
        s_last_frame = current_frame;

        if (ImGui::GetCurrentContext() != nullptr)
        {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
        }

        if (on_gui)
            on_gui();
        if (on_update)
            on_update();

        if (ImGui::GetCurrentContext() != nullptr)
        {
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        glfwSwapBuffers(wnd);
        glfwPollEvents();

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60));
    }
    return 0;
}
