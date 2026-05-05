/*
 * NavViewTextureBridge.cpp
 * ------------------------
 */

#include "NavViewTextureBridge.h"

#if !defined(_WIN32)
#  include <glad/glad.h>
#endif

#if defined(_WIN32)

bool NavViewTextureBridgeAvailable() { return false; }

ImTextureID NavViewTextureBridgeTextureId() { return (ImTextureID)0; }

void NavViewTextureBridgeUpload(int, int, const uint8_t*) {}

#else

namespace
{
GLuint       g_Tex = 0;
ImTextureID  g_Id  = (ImTextureID)0;
int          g_W   = 0;
int          g_H   = 0;
} // namespace

bool NavViewTextureBridgeAvailable() { return true; }

ImTextureID NavViewTextureBridgeTextureId()
{
    if (g_Tex == 0)
    {
        glGenTextures(1, &g_Tex);
        g_Id = (ImTextureID)(intptr_t)g_Tex;
        glBindTexture(GL_TEXTURE_2D, g_Tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    return g_Id;
}

void NavViewTextureBridgeUpload(int w, int h, const uint8_t* rgba)
{
    if (w <= 0 || h <= 0 || !rgba) return;
    NavViewTextureBridgeTextureId();
    glBindTexture(GL_TEXTURE_2D, g_Tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (w != g_W || h != g_H)
    {
        g_W = w;
        g_H = h;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    }
    else
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
}

#endif
