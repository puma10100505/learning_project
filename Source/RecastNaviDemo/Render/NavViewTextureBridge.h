#pragma once
/*
 * NavViewTextureBridge.h
 * ----------------------
 * 将 RGBA 像素上传到 GPU 纹理供 ImGui::Image 使用。
 * - macOS/Linux (GLFW+GL): 使用 OpenGL 纹理
 * - Windows (D3D12): RGBA 上传到默认堆纹理，在 CBV/SRV 堆槽位 kNavViewTextureSrvHeapIndex 建 SRV，
 *   ImTextureID = GPU 描述符句柄（与 imgui_impl_dx12 约定一致）。
 */

#include "imgui.h"
#include <cstdint>

bool        NavViewTextureBridgeAvailable();
ImTextureID NavViewTextureBridgeTextureId();
void        NavViewTextureBridgeUpload(int w, int h, const uint8_t* rgba);
