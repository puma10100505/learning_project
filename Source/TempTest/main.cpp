#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>

const char* gameTitle = "TEST";
GLFWwindow* window;

GLfloat vertices[] =
{
    -1, -1, -1,   -1, -1,  1,   -1,  1,  1,   -1,  1, -1,
    1, -1, -1,    1, -1,  1,    1,  1,  1,    1,  1, -1,
    -1, -1, -1,   -1, -1,  1,    1, -1,  1,    1, -1, -1,
    -1,  1, -1,   -1,  1,  1,    1,  1,  1,    1,  1, -1,
    -1, -1, -1,   -1,  1, -1,    1,  1, -1,    1, -1, -1,
    -1, -1,  1,   -1,  1,  1,    1,  1,  1,    1, -1,  1
};

GLfloat colors[] =
{
    0, 0, 0,   0, 0, 1,   0, 1, 1,   0, 1, 0,
    1, 0, 0,   1, 0, 1,   1, 1, 1,   1, 1, 0,
    0, 0, 0,   0, 0, 1,   1, 0, 1,   1, 0, 0,
    0, 1, 0,   0, 1, 1,   1, 1, 1,   1, 1, 0,
    0, 0, 0,   0, 1, 0,   1, 1, 0,   1, 0, 0,
    0, 0, 1,   0, 1, 1,   1, 1, 1,   1, 0, 1
};

static void controls(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if(action == GLFW_PRESS)
        if(key == GLFW_KEY_ESCAPE)
            glfwSetWindowShouldClose(window, GL_TRUE);
}

bool initWindow(const int resX, const int resY)
{
    if(!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return false;
    }

    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing

    // Open a window and create its OpenGL context
    window = glfwCreateWindow(resX, resY, gameTitle, NULL, NULL);

    if(window == NULL)
    {
        fprintf(stderr, "Failed to open GLFW window.\n");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, controls);

    // Get info of GPU and supported OpenGL version
    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    printf("OpenGL version supported %s\n", glGetString(GL_VERSION));

    glEnable(GL_DEPTH_TEST); // Depth Testing
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    return true;
}

static void drawCube()
{
    static float alpha = 0;
    glMatrixMode(GL_PROJECTION_MATRIX);
    glLoadIdentity();
    glTranslatef(0,0,-2);
    glMatrixMode(GL_MODELVIEW_MATRIX);
    //attempt to rotate cube
    //glRotatef(alpha, 1, 0, 0);

    /* We have a color array and a vertex array */
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glColorPointer(3, GL_FLOAT, 0, colors);

    /* Send data : 24 vertices */
    glDrawArrays(GL_QUADS, 0, 24);

    /* Cleanup states */
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    alpha += 0.1;
}

static void display()
{
    glClearColor(0.0, 0.8, 0.3, 1.0);
    while(!glfwWindowShouldClose(window))
    {
        // Draw stuff
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawCube();

        // Update Screen
        //glFlush();
        glfwSwapBuffers(window);

        // Check for any input, or window movement
        glfwPollEvents();

        // Scale to window size
        GLint windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);
        glViewport(0, 0, windowWidth, windowHeight);
    }
}

int main(int argc, char** argv)
{
    if(initWindow(1024, 620))
    {
        display();
    }
    printf("Goodbye!\n");
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}