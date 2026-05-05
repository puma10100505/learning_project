#pragma once
/*
 * NavViewTextureBridge.h
 * ----------------------
 * 将 RGBA 像素上传到 GPU 纹理供 ImGui::Image 使用。
 * - macOS/Linux (GLFW+GL): 使用 OpenGL 纹理
 * - Windows (D3D12 主后端): 暂未实现，Available() 为 false
 */

#include "imgui.h"

bool        NavViewTextureBridgeAvailable();
ImTextureID NavViewTextureBridgeTextureId();
void        NavViewTextureBridgeUpload(int w, int h, const uint8_t* rgba);
