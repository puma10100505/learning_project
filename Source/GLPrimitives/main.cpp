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

extern glm::vec4 BackgroundColor;
extern float DeltaTime;

static unsigned int VBO, VAO, EBO;

std::unique_ptr<Shader> GlobalShader = nullptr;

static void OnKeyboardEvent(GLFWwindow* InWindow, int Key, int ScanCode, int Action, int Mods)
{
    if (glfwGetKey(InWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(InWindow, GLFW_TRUE);
    }
}

static const GLfloat LineVertices[] {
   -.8f, .8f, 0.f, .8f, -.8f, 10.f
};

static const unsigned int Indices[] = {
    0, 1
};

static void OnTick(float DeltaTime)
{
    if (GlobalShader.get())
    {
        GlobalShader->use();
        GlobalShader->setVec4("Color", glm::vec4(.5f, .5f, .6f, 1.f));
        //GlobalShader->setVec4("Color", glm::vec4(0.5f, sin(glfwGetTime()) / 2.f + 0.5f, 0.5f, 1.f));
    }

    // 先绑定VAO对象
    glBindVertexArray(VAO); 
    glDrawElements(GL_LINES, 2, GL_UNSIGNED_INT, 0);
    // 渲染完成后再解绑VAO对象
    glBindVertexArray(0);


    //printf("Get the last gl error: %x\n", glGetError());
}

static void OnGUI(float DeltaTime)
{
    ImGui::Text("Background Color:");
    ImGui::ColorEdit3("Background Color", (float*)&BackgroundColor);
}

int main(int argc, char** argv)
{
    bool bShowGUI = true;
    if (argc > 1)
    {
        bShowGUI = (atoi(argv[1]) == 1);
    }

    FCreateWindowParameters Parameters
    {
        ScreenWidth, 
        ScreenHeight,
        "GL Primitives Test",
        false, 
        bShowGUI, 
        30
    };

    Parameters.KeyEventCallback = OnKeyboardEvent;

    int iRet = GLCreateWindow(Parameters);
    if (iRet < 0)
    {
        return EXIT_FAILURE;
    }

    GlobalShader = std::make_unique<Shader>(
        (solution_base_path + "Assets/Shaders/demo.vs").c_str(), (solution_base_path + "Assets/Shaders/demo.fs").c_str());

    float vertices[] = {
        -0.5f, -0.5f, 0.0f, // left  
         0.5f, -0.5f, 0.0f, // right 
         0.0f,  0.5f, 0.0f  // top   
    }; 

    // float LineVertices[] = {
    //     -.8f, .8f, 0.f,
    //     .8f, -.8f, 10.f
    // };
    
    // glGenVertexArrays(1, &VAO);
    // glGenBuffers(1, &VBO);
    // glBindVertexArray(VAO);
    // glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    // glEnableVertexAttribArray(0);
    // glBindBuffer(GL_ARRAY_BUFFER, 0); 
    // glBindVertexArray(0); 

    // -----------------

    // glGenVertexArrays(1, &VAO);
    // glGenBuffers(1, &VBO);
    // glBindVertexArray(VAO);
    // glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(LineVertices), LineVertices, GL_STATIC_DRAW);
    // glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    // glEnableVertexAttribArray(0);
    // glBindBuffer(GL_ARRAY_BUFFER, 0);
    // glBindVertexArray(0);

// 1. 将顶点数据存入到BUFFER中，但是还不知道如何解释这些数据
    
    glGenBuffers(1, &VBO);  // 生成顶点BUFFER
    glGenBuffers(1, &EBO);  //  生成索引BUFFER 
    glGenVertexArrays(1, &VAO); // 顶点数组
    
    // 先绑定VAO，在此之后绑定的VBO和EBO会自动记录在VAO中
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO); // 绑定VBO到GL_ARRAY_BUFFER类型，指定VBO变量对应的BUFFER是VBO类型
    glBufferData(GL_ARRAY_BUFFER, sizeof(LineVertices), LineVertices, GL_STATIC_DRAW); // 发送顶点数据

    // 2. 解析BUFFER中的数据
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // 因为之前已经绑定了VAO， 所以在这里绑定的EBO会记录在VAO中
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);       // 这个调用的参数对应Vertex Shader中 location=0的参数

    
    GLWindowTick(OnTick, OnGUI);

    GLDestroyWindow();

    return EXIT_SUCCESS;
}