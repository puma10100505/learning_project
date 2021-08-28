#ifdef __linux__
#include <unistd.h>
#endif

#include <iostream>
#include <string.h>
#include <cstring>
#include <memory>
#include "learning.h"
#include "LearningStatics.h"
#include "CommonDefines.h"
#include "NXCube.h"

extern glm::vec4 BackgroundColor;
extern float DeltaTime;

static uint32_t TextureId, TextureId2;

static uint32_t VBO, VAO, EBO;

static float MixAlpha = 0.5f;
static glm::mat4 Trans = glm::mat4(1.f);
static glm::vec3 Translate;

static float Vertices[] = {
//     ---- 位置 ----       ---- 颜色 ----     - 纹理坐标 -
     0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.f, 1.f,   // 右上
     0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.f, 0.f,   // 右下
    -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.f, 0.f,   // 左下
    -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.f, 1.f    // 左上
};

static uint32_t Indices[] = {
    0, 1, 3, 1, 2, 3
};

static float Model_X_Rot = -55.f;
static float Model_Y_Rot = -55.f;
static float Model_Z_Rot = -55.f;

static glm::vec3 Model_Rot = glm::vec3(0.f);

static std::unique_ptr<Shader> GlobalShader = nullptr;

static void OnKeyboardEvent(GLFWwindow* InWindow, int Key, int ScanCode, int Action, int Mods)
{
    if (glfwGetKey(InWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(InWindow, GLFW_TRUE);
    }
}

static void OnTick(float DeltaTime)
{
    if (GlobalShader.get())
    {
        /** 这里一定要先初始化为mat4(1.f)，否则渲染后无法正常显示 */
        glm::mat4 Model = glm::mat4(1.f);
        glm::mat4 View = glm::mat4(1.f);
        glm::mat4 Projection = glm::mat4(1.f);

        Model = glm::rotate(Model, glm::radians(Model_Rot.x), glm::vec3(1.f, 0.f, 0.f));
        Model = glm::rotate(Model, glm::radians(Model_Rot.y), glm::vec3(0.f, 1.f, 0.f));
        Model = glm::rotate(Model, glm::radians(Model_Rot.z), glm::vec3(0.f, 0.f, 1.f));

        View = glm::translate(View, glm::vec3(0.f, 0.f, -3.f));
        Projection = glm::perspective(glm::radians(45.f), ScreenWidth * 1.f / ScreenHeight * 1.f, 0.1f, 100.f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, TextureId);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D,  TextureId2);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        GlobalShader->SetFloatValue("alpha", MixAlpha);
        GlobalShader->SetMatrix4Value("model", Model);
        GlobalShader->SetMatrix4Value("view", View);
        GlobalShader->SetMatrix4Value("projection", Projection);
        GlobalShader->Activate();
    }
}

static void OnGUI(float DeltaTime)
{
    ImGui::Text("Background Color:");
    ImGui::ColorEdit3("Background Color", (float*)&BackgroundColor);
    ImGui::SliderFloat("Mix Alpha", &MixAlpha, 0.f, 1.f, "%.2f");
    ImGui::SliderFloat3("Model Rot", (float *)&Model_Rot, 0.f, 360.f);
}

static void InitTexture()
{
    glGenTextures(1, &TextureId);
    glBindTexture(GL_TEXTURE_2D, TextureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int Width, Height, NrChannels;
    stbi_set_flip_vertically_on_load(1);
    unsigned char* TexData = stbi_load((DefaultTextureDirectory + "container.jpg").c_str(), &Width, &Height, &NrChannels, 0);
    if (TexData)
    {
        printf("After load texture, width: %d, height: %d, channels: %d\n", Width, Height, NrChannels);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Width, Height, 0, GL_RGB, GL_UNSIGNED_BYTE, TexData);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        printf("failed to load texture\n");
    }

    stbi_image_free(TexData);

    // 加载第二个纹理
    glGenTextures(1, &TextureId2);
    glBindTexture(GL_TEXTURE_2D, TextureId2);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    TexData = stbi_load((DefaultTextureDirectory + "awesomeface.png").c_str(), &Width, &Height, &NrChannels, 0);
    if (TexData)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, TexData);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    stbi_image_free(TexData);

    GlobalShader->Activate();
    glUniform1i(glGetUniformLocation(GlobalShader->ID, "texture1"), 0);
    GlobalShader->SetIntValue("texture2", 1);
}

void InitVertexData()
{
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

int main(int argc, char** argv)
{
    if (GLCreateWindow(FCreateWindowParameters::DefaultWindowParameters()) < 0)
    {
        return EXIT_FAILURE;
    }

    GlobalShader = std::make_unique<Shader>("transform.ex");

    InitVertexData();
    InitTexture();

    GLWindowTick(OnTick, OnGUI);

    GLDestroyWindow();

    return EXIT_SUCCESS;
}