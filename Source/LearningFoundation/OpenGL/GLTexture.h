#pragma once

#include <cstdio>
#include <iostream>
#include "thirdparty_header.h"
#include "utility.h"

class GLTexture final {
public: 
    GLTexture() {}
    GLTexture(const std::string& texture, int32_t tex_idx) {
        texture_name = texture;
        PrepareTexture(texture, tex_idx);
    }

    ~GLTexture() {}

    inline void PrepareTexture(const std::string& texture, int32_t tex_idx) {
        texture_index = tex_idx;

        texture_handler = load_texture(texture.c_str());
    }

    virtual void Active() {
        glActiveTexture(texture_index);
        glBindTexture(GL_TEXTURE_2D, texture_handler);
    }

private:
    uint32_t texture_handler;
    std::string texture_name;
    // unsigned char* data = nullptr;
    // int32_t width;
    // int32_t height;
    // int32_t nrChannels;
    int32_t texture_index;
};
