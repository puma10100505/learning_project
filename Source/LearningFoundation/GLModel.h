#pragma once
#include <vector>
#include <cstring>

#include "defines.h"
#include "GLMesh.h"
#include "shader.h"

#include "thirdparty_header.h"

#include "stb_image.h"

class GLModel {
public:
    GLModel(const std::string& path) {
        loadModel(path);
    }

    void Draw(Shader shader, int draw_mode = GL_FILL);

private:
    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    GLMesh processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, 
        aiTextureType type, const std::string& typeName);

private:
    std::vector<GLMesh> meshes;
    std::string directory;

    std::vector<Texture> textures_loaded;

};