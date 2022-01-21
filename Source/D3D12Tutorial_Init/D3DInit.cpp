#include "D3DInit.h"

using namespace D3DInit;

void D3DInit::InitializeDevice()
{
    #if defined(DEBUG) || defined(_DEBUG)
    {
        ComPtr<ID3D12Debug> DebugController;
        if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&DebugController))))
        {
            PLOGD << "create debug interface failed";
        }
        else
        {
            DebugController->EnableDebugLayer();
        }
    }
    #endif

    if (FAILED(CreateDXGIFactory4(IID_PPV_ARGS(&DXGIFactory))))
    {
        PLOGD << "Create dxgi factory \failed";
        return;
    }

    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&MainDevice))))
    {
        // 如果主设备创建失败则使用备用的软件3D加速设备
        ComPtr<IDXGIAdapter> pWarpAdapter;
        if (SUCCEEDED(DXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter))))
        {
            D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0， IID_PPV_ARGS(&MainDevice));
        }
        else
        {
            PLOGD << "No device created";
            return;
        }
    }
    else
    {
        PLOGD << "Create main device succeeded...";
    }
}

void D3DInit::CreateFence()
{
    if (MainDevice.Get() == nullptr)
    {
        PLOGD << "MainDevice is not init";
        return;
    }

    if (FAILED(MainDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence))))
    {
        PLOGD << "Create fence failed";
        return;
    }

    RTVDescSize = MainDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    DSVDescSize = MainDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    CBVSRVDescSize = MainDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void D3DInit::CheckForMSAA()
{
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS MSQualityLevels;
    MSQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    MSQualityLevels.SampleCount = 4;
    MSQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    MSQualityLevels.NumQualityLevels = 0;
    if (FAILED(MainDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, 
        &MSQualityLevels, sizeof(MSQualityLevels))))
    {
        PLOGD << "Check for feature support failed";
        return;
    }

    MSAA4XQuality = MSQualityLevels.NumQualityLevels;
}

void D3DInit::CreateCommandObjects()
{
    D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
    QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    
    if (FAILED(MainDevice->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&CommandQueue))))
    {
        PLOGD << "Create command queue failed";
        return;
    }

    if (FAILED(MainDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, 
        IID_PPV_ARGS(DirectCommandAllocator.GetAddressOf())))
    {
        PLOGD << "Create command allocator failed";
        return;
    }

    if (FAILED(MainDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, DirectCommandAllocator.Get(), 
        nullptr, IID_PPV_ARGS(CommandList.GetAddressOf()))))
    {
        PLOGD << "Create command list failed";
        return ;
    }

    CommandList->Close();
}

void D3DInit::CreateSwapChain(UINT InBufferCount, HWND InWnd)
{
    SwapChain.Reset();
    DXGI_SWAP_CHAIN_DESC SwapChainDesc;
    SwapChainDesc.BufferDesc.Width = 1280;
    SwapChainDesc.BufferDesc.Height = 720;
    SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    SwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    SwapChainDesc.SampleDesc.Count = MSAA4XQuality ? 4 : 1;
    SwapChainDesc.SampleDesc.Quality = MSAA4XQuality ? (MSAA4XQuality - 1) : 0;
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.BufferCount = InBufferCount;
    SwapChainDesc.OutputWindow = InWnd;
    SwapChainDesc.Windowed = true;
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    if (FAILED(DXGIFactory->CreateSwapChain(CommandQueue.Get(), &SwapChainDesc, SwapChain.GetAddressOf())))
    {
        PLOGD << "Create swap chain failed";
        return;
    }
}

void D3DInit::CreateRTVAndDSVDescriptorHeap(UINT InBufferCount)
{
    D3D12_DESCRIPTOR_HEAP_DESC RTVHeapDesc;
    RTVHeapDesc.NumDescriptors = InBufferCount;
    RTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    RTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    RTVHeapDesc.NodeMask = 0;
    if (FAILED(MainDevice->CreateDescriptorHeap(&RTVHeapDesc, IID_PPV_ARGS(RTVHeap.GetAddressOf()))))
    {
        PLOGD << "Create rtv heap failed";
        return;
    }

    D3D12_DESCRIPTOR_HEAP_DESC DSVHeapDesc;
    DSVHeapDesc.NumDescriptors = 1;
    DSVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    DSVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    DSVHeapDesc.NodeMask = 0;
    if (FAILED(MainDevice->CreateDescriptorHeap(&DSVHeapDesc, IID_PPV_ARGS(DSVHeap.GetAddressOf()))))
    {
        PLOGD << "Create dsv heap failed";
        return ;
    }
}

void D3DInit::CreateRenderTargetView()
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHeapHandle(RTVHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < SwapChainBufferCount; ++i)
    {
        if (FAILED(SwapChain->GetBuffer(i, IID_PPV_ARGS(&SwapChainBuffer[i]))))
        {
            PLOGD << "Create get buffer failed";
            continue;
        }

        MainDevice->CreateRenderTargetView(SwapChainBuffer[i].Get(), nullptr, RTVHeapHandle);

        RTVHeapHandle.Offset(1, RTVDescSize);
    }
}

void D3DInit::CreateDepthStencilView()
{
    D3D12_RESOURCE_DESC DepthStencilDesc;
    DepthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    DepthStencilDesc.Alignment = 0; 
    DepthStencilDesc.Width = ClientWidth;
    DepthStencilDesc.Height = ClientHeight;
    DepthStencilDesc.DepthOrArraySize = 1;
    DepthStencilDesc.MipLevels = 1;
    DepthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    DepthStencilDesc.SampleDesc.Count = MSAA4XQuality;
    DepthStencilDesc.SampleDesc.Quality = MSAA4XQuality ? (MSAA4XQuality - 1) : 0;
    DepthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    DepthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    
    D3D12_CLEAR_VALUE OptClear;
    OptClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    OptClear.DepthStencil.Depth = 1.f;
    OptClear.DepthStencil.Stencil = 0;
    MainDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, 
        &DepthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &OptClear, IID_PPV_ARGS(DepthStencilBuffer.GetAddressOf()));

    MainDevice->CreateDepthStencilView(DepthStencilBuffer.Get(), nullptr, DepthStencilView());

    CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(DepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
}

void D3DInit::SetScissorRectangles()
{
    CommandList->RSSetScissorRects(1, &ScissorRect);
}