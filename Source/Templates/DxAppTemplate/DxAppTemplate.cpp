
/*
* 创建窗口的基础代码模板
*
*/
#ifdef __linux__
#include <unistd.h>
#endif

#include <iostream>
#include <string.h>
#include <memory>
#include <cstring>
#include <vector>

#include "CommonDefines.h"
#include "DirectX12Window.h"
#include "loguru.hpp"

static void OnTick(float DeltaTime)
{
    // TODO: 
}

static void OnGUI(float DeltaTime)
{
    // TODO: 
}

static void OnPostGUI(float DeltaTime)
{
    // TODO: 
}

static void OnInput(float DeltaTime)
{
    // TODO: 
}

int main(int argc, char** argv)
{
    loguru::init(argc, argv);
    loguru::add_file(__FILE__".log", loguru::Append, loguru::Verbosity_MAX);
    loguru::g_stderr_verbosity = 1;

    LOG_F(INFO, "entry of app");

    return dx::CreateWindowInstance("Hello dx window", 900, 600, 0, 0, 
        OnTick, OnGUI, OnPostGUI,OnInput);
}