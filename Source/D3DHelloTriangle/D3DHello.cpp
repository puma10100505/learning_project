#include <iostream>
#include <string.h>
#include <memory>
#include <cstring>
#include <vector>

#include "loguru.hpp"
#include "D3DHelloTriangle.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    // loguru::init(argc, argv);
    // loguru::add_file(__FILE__".log", loguru::Append, loguru::Verbosity_MAX);
    // loguru::g_stderr_verbosity = 1;

    // LOG_F(INFO, "entry of app");

    D3D12HelloTriangle Sample(1280, 720, L"D3D12 Hello Win");
    return Win32Application::Run(&Sample, hInstance, nCmdShow);
}