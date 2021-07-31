#ifdef __linux__
    #include <unistd.h>
#endif 
#include <cstdio>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

#include "CommonDefines.h"
#include "learning.h"


// static std::unique_ptr<Shader> GlobalShader(new Shader(
//     (solution_base_path + "Assets/Shaders/multilights.object.vs").c_str(), 
//     (solution_base_path + "Assets/Shaders/multilights.object.fs").c_str())
// );

static void WindowKeyCallback(GLFWwindow* InWindow, int Key, int ScanCode, int Action, int Mods)
{

}

static void OnGUI(float DeltaTime)
{
    printf("Entry of OnGUI\n");
}

static void OnTick(float DeltaTime)
{
    printf("Entry of OnTick\n");
}

int main()
{
    int iRet = GLCreateWindow({
        ScreenWidth,
        ScreenHeight,
        "Camera System",
        false,
        true,
        30,
        WindowKeyCallback
    });

    if (iRet < 0)
    {
        printf("Create window failed, ret: %d\n", iRet);
        return EXIT_FAILURE;
    }

    GLWindowTick(OnTick, OnGUI);

    GLDestroyWindow();

    return EXIT_SUCCESS;
}