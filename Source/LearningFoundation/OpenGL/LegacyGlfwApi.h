#pragma once

#include <functional>
#include <string>

// 供 GLBasic / GLAdvanced / GLMultilights 等老示例使用，实现见 LegacyGlfwApi.cpp（基于 GlfwWindows）。
int gl_init_window();
int gl_create_window(int width, int height, const std::string& title, bool hide_cursor);
int gl_create_window(int width, int height, const std::string& title, bool hide_cursor, int frame_interval);
int gl_init_gui();
void gl_destroy_gui();
void gl_destroy_window();
int gl_window_loop(std::function<void(void)> on_update, std::function<void(void)> on_gui);
