#pragma once

#include <stdint.h>
#include <memory>
#include <string>
#include "defines.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "LearningShader.h"

#define MAX_TEXTURE_NUM 32

class NXCube
{
    public:
        NXCube(const std::string& InShaderName);
        ~NXCube();

        void AddTexture(const std::string& InTexName);

        void Render();

        const Transform& GetTransform() const { return ModelTransform; }
        const glm::vec3 GetLocation() const { return ModelTransform.position; }
        const glm::vec3 GetRotation() const { return ModelTransform.rotation; }
        const glm::vec3 GetScale() const { return ModelTransform.scale; }

        Transform& GetTransformMutable();
        glm::vec3& GetLocationMutable();
        glm::vec3& GetRotationMutable();
        glm::vec3& GetScaleMutable();

        void Destroy();

    protected: 
        void Initialize(const std::string& InShaderName);

        void ActivateAllTextures();
        void ActivateShader();
        void DrawVertices();

    private:
        const static float VertexData[];

        uint32_t PrimitiveVBO = 0;
        uint32_t PrimitiveVAO = 0;
        uint32_t PrimitiveEBO = 0;

        // Shader
        std::unique_ptr<Shader> ShaderPtr;

        // Textures
        uint32_t TextureHandles[MAX_TEXTURE_NUM];
        int32_t LastTextureIndex = 0;

        Transform ModelTransform;

        glm::mat4 ModelMatrix;
};