#pragma once

#include <iostream>
#include "stb_image.h"
#include <glad/glad.h>

static inline uint32_t texture_from_file(const char* path, const std::string& directory, bool gamma = false) {
    std::string filename = std::string(path);
    filename = directory + '/' + filename;

    uint32_t textureid;
    glGenTextures(1, &textureid);

    int width, height, nr_components;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nr_components, 0);
    if (data) {
        GLenum format = GL_RGB;
        if (nr_components == 1){
            format = GL_RED;
        } else if (nr_components == 3) {
            format = GL_RGB;
        } else if (nr_components == 4) {
            format = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureid);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load, filename: " << filename << ", path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureid;
}

// Load a texture from path
static inline uint32_t load_texture(const char* path) {
    uint32_t textureid;
    glGenTextures(1, &textureid);

    int width, height, nr_components;
    unsigned char* data = stbi_load(path, &width, &height, &nr_components, 0);
    if (data) {
        GLenum format = GL_RGB;
        if (nr_components == 1){
            format = GL_RED;
        } else if (nr_components == 3) {
            format = GL_RGB;
        } else if (nr_components == 4) {
            format = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureid);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load\n";
        stbi_image_free(data);
    }

    return textureid;
}