#include "NXCube.h"

#include <glad/glad.h>

const float NXCube::VertexData[] = {
    -1.0f, -1.0f, -1.0f,  0.0f, 0.0f,
     1.0f, -1.0f, -1.0f,  1.0f, 0.0f,
     1.0f,  1.0f, -1.0f,  1.0f, 1.0f,
     1.0f,  1.0f, -1.0f,  1.0f, 1.0f,
    -1.0f,  1.0f, -1.0f,  0.0f, 1.0f,
    -1.0f, -1.0f, -1.0f,  0.0f, 0.0f,

    -1.0f, -1.0f,  1.0f,  0.0f, 0.0f,
     1.0f, -1.0f,  1.0f,  1.0f, 0.0f,
     1.0f,  1.0f,  1.0f,  1.0f, 1.0f,
     1.0f,  1.0f,  1.0f,  1.0f, 1.0f,
    -1.0f,  1.0f,  1.0f,  0.0f, 1.0f,
    -1.0f, -1.0f,  1.0f,  0.0f, 0.0f,

    -1.0f,  1.0f,  1.0f,  1.0f, 0.0f,
    -1.0f,  1.0f, -1.0f,  1.0f, 1.0f,
    -1.0f, -1.0f, -1.0f,  0.0f, 1.0f,
    -1.0f, -1.0f, -1.0f,  0.0f, 1.0f,
    -1.0f, -1.0f,  1.0f,  0.0f, 0.0f,
    -1.0f,  1.0f,  1.0f,  1.0f, 0.0f,

     1.0f,  1.0f,  1.0f,  1.0f, 0.0f,
     1.0f,  1.0f, -1.0f,  1.0f, 1.0f,
     1.0f, -1.0f, -1.0f,  0.0f, 1.0f,
     1.0f, -1.0f, -1.0f,  0.0f, 1.0f,
     1.0f, -1.0f,  1.0f,  0.0f, 0.0f,
     1.0f,  1.0f,  1.0f,  1.0f, 0.0f,

    -1.0f, -1.0f, -1.0f,  0.0f, 1.0f,
     1.0f, -1.0f, -1.0f,  1.0f, 1.0f,
     1.0f, -1.0f,  1.0f,  1.0f, 0.0f,
     1.0f, -1.0f,  1.0f,  1.0f, 0.0f,
    -1.0f, -1.0f,  1.0f,  0.0f, 0.0f,
    -1.0f, -1.0f, -1.0f,  0.0f, 1.0f,

    -1.0f,  1.0f, -1.0f,  0.0f, 1.0f,
     1.0f,  1.0f, -1.0f,  1.0f, 1.0f,
     1.0f,  1.0f,  1.0f,  1.0f, 0.0f,
     1.0f,  1.0f,  1.0f,  1.0f, 0.0f,
    -1.0f,  1.0f,  1.0f,  0.0f, 0.0f,
    -1.0f,  1.0f, -1.0f,  0.0f, 1.0f
};

NXCube::NXCube(const std::string& InShaderName) 
{
    Initialize(InShaderName);
}

NXCube::~NXCube() {}

void NXCube::Initialize(const std::string& InShaderName)
{
    ShaderPtr = std::make_unique<Shader>(InShaderName);

    glGenBuffers(1, &PrimitiveVBO);
    // glGenBuffers(1, &PrimitiveEBO);
    glGenVertexArrays(1, &PrimitiveVAO);

    glBindVertexArray(PrimitiveVAO);

    glBindBuffer(GL_ARRAY_BUFFER, PrimitiveVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VertexData), VertexData, GL_STATIC_DRAW);
    
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, PrimitiveEBO);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    // glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void NXCube::AddTexture(const std::string& InTexName)
{
    if (LastTextureIndex >= MAX_TEXTURE_NUM)
    {
        return;
    }

    glGenTextures(1, &TextureHandles[LastTextureIndex]);
    glBindTexture(GL_TEXTURE_2D, TextureHandles[LastTextureIndex]);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int Width, Height, NrChannels;
    stbi_set_flip_vertically_on_load(1);
    unsigned char* TexData = stbi_load((DefaultTextureDirectory + InTexName).c_str(), &Width, &Height, &NrChannels, 0);
    if (TexData)
    {
        printf("After load texture, width: %d, height: %d, channels: %d\n", Width, Height, NrChannels);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Width, Height, 0, GL_RGB, GL_UNSIGNED_BYTE, TexData);
        glGenerateMipmap(GL_TEXTURE_2D);
        LastTextureIndex++;
    }
    else
    {
        printf("failed to load texture\n");
    }

    stbi_image_free(TexData);
}

void NXCube::Render()
{
    ActivateAllTextures();

    ActivateShader();

    DrawVertices();
}

void NXCube::ActivateAllTextures ()
{
    for (int32_t i = 0; i < LastTextureIndex; ++i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, TextureHandles[i]);
    }
}

Transform& NXCube::GetTransformMutable()
{
    ModelTransform.bIsDirty = true;
    return ModelTransform;
}

glm::vec3& NXCube::GetLocationMutable()
{
    ModelTransform.bIsDirty = true;
    return ModelTransform.position;
}

glm::vec3& NXCube::GetRotationMutable()
{
    ModelTransform.bIsDirty = true;
    return ModelTransform.rotation;
}

glm::vec3& NXCube::GetScaleMutable()
{
    ModelTransform.bIsDirty = true;
    return ModelTransform.scale;
}

void NXCube::ActivateShader()
{
    if (ShaderPtr.get())
    {
        ShaderPtr->Activate();

        if (ModelTransform.bIsDirty)
        {
            // 模型变换
            ModelMatrix = glm::rotate(ModelMatrix, glm::radians(GetRotation().x), glm::vec3(1.0f, 0.0f, 0.0f));
            ModelMatrix = glm::rotate(ModelMatrix, glm::radians(GetRotation().y), glm::vec3(0.0f, 1.0f, 0.0f));
            ModelMatrix = glm::rotate(ModelMatrix, glm::radians(GetRotation().z), glm::vec3(0.0f, 0.0f, 1.0f));
            ModelMatrix = glm::scale(ModelMatrix, GetScale());
            ModelMatrix = glm::translate(ModelMatrix, GetLocation());

            ModelTransform.bIsDirty = false;
        }

        ShaderPtr->SetMatrix4Value("model", ModelMatrix);
    }
}

void NXCube::DrawVertices()
{
    glBindVertexArray(PrimitiveVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void NXCube::Destroy()
{
    glDeleteVertexArrays(1, &PrimitiveVAO);
    glDeleteBuffers(1, &PrimitiveVBO);
}