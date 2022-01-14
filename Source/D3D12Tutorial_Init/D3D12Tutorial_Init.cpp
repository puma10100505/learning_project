#include "stdafx.h"

#include <cstdio>
#include <Windows.h>
#include <string>
#include <memory>
#include <iostream>
#include <vector>
#include <stdexcept>

#include "CommonDefines.h"
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

#include "plog/Log.h"
#include "plog/Initializers/RollingFileInitializer.h"

using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Microsoft::WRL;

static ComPtr<ID3D12Device> Device;

// Factory -> Adapter -> Device

void D3DLogAdapters()
{
    IDXGIFactory* Factory = nullptr;
    UINT dxgiFactoryFlag = 0;
    CreateDXGIFactory2(dxgiFactoryFlag, IID_PPV_ARGS(&Factory));
    
    UINT i = 0; 
    ComPtr<IDXGIAdapter> Adapter = nullptr;
    std::vector<IDXGIAdapter*> AdapterList;
    while (Factory->EnumAdapters(i, &Adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC Desc;
        Adapter->GetDesc(&Desc);
        

        std::wstring Text = L"***Adapter: ";
        Text += Desc.Description;
        Text += L"\n";

        OutputDebugString(Text.c_str());
        AdapterList.push_back(Adapter.Get());
        ++i;
        std::wcout << Text << " " << Desc.DeviceId << std::endl;

        if (Desc.DeviceId == 8710)
        {
            break;
        }
    }

    if (FAILED(D3D12CreateDevice(Adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device))))
    {
        PLOGD << "Create device failed";
        return;
    }

    std::cout << "GPU Node Count: " << Device->GetNodeCount() << std::endl;
}

void CheckFeatures()
{
    if (Device.Get() == nullptr)
    {
        return;
    }

    D3D_FEATURE_LEVEL FeatureLevels[4] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3
    };

    D3D12_FEATURE_DATA_FEATURE_LEVELS FeatureLevelsInfo;
    FeatureLevelsInfo.NumFeatureLevels = 4;
    FeatureLevelsInfo.pFeatureLevelsRequested = FeatureLevels;
    if (SUCCEEDED(Device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &FeatureLevelsInfo, sizeof(FeatureLevelsInfo))))
    {
        std::cout << FeatureLevelsInfo.MaxSupportedFeatureLevel << std::endl;
    }

    D3D12_FEATURE_DATA_SHADER_MODEL ShaderModelInfo;
    if (SUCCEEDED(Device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &ShaderModelInfo, sizeof(ShaderModelInfo))))
    {
        std::cout << ShaderModelInfo.HighestShaderModel << std::endl;
    }
    else
    {
        std::cout << "CheckFeatureSupport for shader model failed" << std::endl;
    }
}

// CommandList(CommandAllocator) -> CommandQueue
void CreateCommandQueue()
{
    if (Device.Get() == nullptr)
    {
        return;
    }

    ComPtr<ID3D12CommandQueue> CommandQueue;
    D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
    QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    if (FAILED(Device->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&CommandQueue))))
    {
        PLOGD << "Create commandqueue failed";
        return;
    }
}

int main(int argc, char** argv)
{
    plog::init(plog::debug,  (DefaultLogDirectory + "D3D12Tutorial_Init.log").c_str());

    if (XMVerifyCPUSupport() == false)
    {
        PLOGD << "CPU does not support SSE2";
    }

    PLOG_DEBUG << L"Entry of D3DInit";

    D3DLogAdapters();

    CheckFeatures();

    CreateCommandQueue();

    return EXIT_SUCCESS;
}