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

static unsigned int VBO, VAO;

std::unique_ptr<Shader> GlobalShader = nullptr;

static void OnKeyboardEvent(GLFWwindow* InWindow, int Key, int ScanCode, int Action, int Mods)
{
    if (glfwGetKey(InWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(InWindow, GLFW_TRUE);
    }
}

static const GLfloat LineVertices[] {
    200, 100, 0,
    100, 300, 0
};

static void OnTick(float DeltaTime)
{
    if (GlobalShader.get())
    {
        GlobalShader->use();
        GlobalShader->setVec4("Color", glm::vec4(0.5f, sin(glfwGetTime()) / 2.f + 0.5f, 0.5f, 1.f));
    }

    // glLoadIdentity();
    // glTranslatef(-1.f, -1.f, 0.f);

    // LearningStatics::GLDrawLine(glm::vec3(0.f, 0.f, 0.f), glm::vec3(100.f, 100.f, 100.f), 3.f, glm::vec3(1.f, 0.f, 0.f));

    // glColor3ub(254, 0, 0);
    // glBegin(GL_LINES);        
    // glVertex3f(-1.f, 0.f, 0.f); //定点坐标范围
    // glVertex3f(1.f, 0.f, 0.f);
    // glEnd();

    // glEnableClientState(GL_VERTEX_ARRAY);
    // glVertexPointer(3, GL_FLOAT, 0, LineVertices);
    // glDrawArrays(GL_LINES, 0, 2);
    // glDisableClientState(GL_VERTEX_ARRAY);

    // glDrawArrays(GL_LINES, 0, 2);

    glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
    glDrawArrays(GL_TRIANGLES, 0, 3);

    printf("Get the last gl error: %x\n", glGetError());
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

    // unsigned int Buffer;
    // glGenBuffers(1, &Buffer);
    // glBindBuffer(GL_ARRAY_BUFFER, Buffer);
    // glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), LineVertices, GL_STATIC_DRAW);
    // glEnableVertexAttribArray(0);
    // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);
    // glBindBuffer(GL_ARRAY_BUFFER, 0);
    // printf("Get the last gl error: %x\n", glGetError());

    GlobalShader = std::make_unique<Shader>(
        (solution_base_path + "Assets/Shaders/demo.vs").c_str(), 
        (solution_base_path + "Assets/Shaders/demo.fs").c_str()
    );

    float vertices[] = {
        -0.5f, -0.5f, 0.0f, // left  
         0.5f, -0.5f, 0.0f, // right 
         0.0f,  0.5f, 0.0f  // top   
    }; 

    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0); 

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0); 

    GLWindowTick(OnTick, OnGUI);

    GLDestroyWindow();

    return EXIT_SUCCESS;
}