#include <iostream>
#include <string.h>
#include <memory>
#include <cstring>
#include <vector>

#include "CommonDefines.h"
#include "D3DHelloTriangle.h"

#include "plog/Log.h"
#include "plog/Initializers/RollingFileInitializer.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    plog::init(plog::debug,  (DefaultLogDirectory + "D3DHello.log").c_str());

    PLOGD << "Entry of application..." << " CmdLine: " << lpCmdLine << ", file: " << __FILE__;

    std::cout << "Hello App" << std::endl;

    D3D12HelloTriangle Sample(1280, 720, L"D3D12 Hello Win");
    return Win32Application::Run(&Sample, hInstance, nCmdShow);
}