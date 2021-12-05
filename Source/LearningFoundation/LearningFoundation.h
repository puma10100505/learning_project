#pragma once
# pragma warning (disable:4819)

#include <chrono>
#include <thread>
#include <cstdio>
#include <iostream>
#include <functional>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "imgui/imgui.h"

#include "LearningShader.h"
#include "stb_image.h"

#include "utility.h"
#include "defines.h"

#include "GL/freeglut.h"





// static bool bIsFullScreen = false;
// static int FullScreenWidth = 0;
// static int FullScreenHeight = 0;
//static int ScreenWidth = 1280;
//static int ScreenHeight = 900;
static const int WINDOW_WIDTH = 1280;
static const int WINDOW_HEIGHT = 900;
// const std::string title = "Sample Window";
// static int32_t FPS = 60;
// static float WINDOW_RATIO = (float)ScreenWidth/(float)ScreenHeight;
// static float LastFrameTime = 0.0f;

// @ 旧版函数，后续不再维护升级，逐步淘汰
// void on_glfw_error_default(int code, const char* desc);
// void on_frame_buffer_size_changed_default(GLFWwindow* window, int width, int height);
// void on_key_event_default(GLFWwindow* window, int key, int scancode, int action, int mods);
// int gl_init_window();
// int gl_init_gui();
// void gl_on_gui_default();
// void gl_destroy_window();
// void gl_destroy_gui();
// int gl_window_loop(std::function<void (void)> on_update, std::function<void (void)> on_gui);
// int gl_create_window(int32_t width = WINDOW_WIDTH, int32_t height = WINDOW_HEIGHT, 
//     const std::string& title = "Sample Window", bool hide_cursor = false, int32_t frameInterval = 60,
//     GLFWerrorfun err_handler = on_glfw_error_default, 
//     GLFWframebuffersizefun fbs_handler = on_frame_buffer_size_changed_default, 
//     GLFWkeyfun key_handler = on_key_event_default);


