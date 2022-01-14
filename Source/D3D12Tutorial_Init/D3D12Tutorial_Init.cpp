#include <Windows.h>
#include <string>
#include <memory>
#include <iostream>
#include <vector>

#include "CommonDefines.h"

#include <DirectXMath.h>
#include <DirectXPackedVector.h>

#include "plog/Log.h"
#include "plog/Initializers/RollingFileInitializer.h"

using namespace DirectX;
using namespace DirectX::PackedVector;

void D3DLogAdapters()
{
    
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    plog::init(plog::debug,  (DefaultLogDirectory + "D3D12Tutorial_Init.log").c_str());

    if (XMVerifyCPUSupport() == false)
    {
        PLOGD << "CPU does not support SSE2";
    }

    PLOG_DEBUG << L"Entry of D3DInit";



    return EXIT_SUCCESS;
}