
#include "D3DHelloTriangle.h"


using namespace std;

D3D12HelloTriangle::D3D12HelloTriangle(UINT width, UINT height, std::wstring name)
{
    //DXSample(width, height, name);
}

void D3D12HelloTriangle::LoadPipeline()
{
    UINT dxgiFactoryFlags = 0;
    #if defined(_DEBUG)
    {
        ComPtr<ID3D12Debug> DebugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&DebugController))))
        {
            DebugController->EnableDebugLayer();

            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
    #endif

    ComPtr<IDXGIFactory4> Factory;
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&Factory)));

    if (m_useWarpDevice)
    {
        ComPtr<IDXGIAdapter> WarpAdapter;
        ThrowIfFailed(Factory->EnumWarpAdapter(IID_PPV_ARGS(&WarpAdapter)));
        ThrowIfFailed(D3D12CreateDevice(WarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice)));
    }
    else
    {
        ComPtr<IDXGIAdapter1> HardwareAdapter;
        GetHardwareAdapter(Factory.Get(), &HardwareAdapter);
        ThrowIfFailed(D3D12CreateDevice(HardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice)));
    }

    D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
    QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(mDevice->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&mCommandQueue)));

    DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
    SwapChainDesc.BufferCount = FrameCount;
    SwapChainDesc.Width = m_width;
    SwapChainDesc.Height = m_height;
    SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    //SwapChainDesc.OutputWindow = Win32Application::GetHwnd();
    SwapChainDesc.SampleDesc.Count = 1;
    //SwapChainDesc.Windowed = TRUE;

    ComPtr<IDXGISwapChain1> SwapChain;
    ThrowIfFailed(Factory->CreateSwapChainForHwnd(mCommandQueue.Get(), Win32Application::GetHwnd(), 
        &SwapChainDesc, nullptr, nullptr, &SwapChain));
    
    ThrowIfFailed(Factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(SwapChain.As(&mSwapChain));
    
    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

    // Create descriptor heaps;
    {
        D3D12_DESCRIPTOR_HEAP_DESC RtvHeapDesc = {};
        RtvHeapDesc.NumDescriptors = FrameCount;
        RtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        RtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(mDevice->CreateDescriptorHeap(&RtvHeapDesc, IID_PPV_ARGS(&mRtvHeap)));
        mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    // Create frame resources;
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE RtvHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
        
        for (UINT n = 0; n < FrameCount; n++)
        {
            ThrowIfFailed(mSwapChain->GetBuffer(n, IID_PPV_ARGS(&mRenderTargets[n])));
            mDevice->CreateRenderTargetView(mRenderTargets[n].Get(), nullptr, RtvHandle);
            RtvHandle.Offset(1, mRtvDescriptorSize);
        }
    }

    ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator)));
}

void D3D12HelloTriangle::LoadAssets()
{
    // Create an empty root signature;
    {
        CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;
        RootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
        ComPtr<ID3DBlob> Signature;
        ComPtr<ID3DBlob> Error;
        ThrowIfFailed(D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &Signature, &Error));
        ThrowIfFailed(mDevice->CreateRootSignature(0, Signature->GetBufferPointer(), Signature->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));
    }

    // Create the pipeline state, which includes compiling and loading shaders.
    {
        ComPtr<ID3DBlob> VertexShader;
        ComPtr<ID3DBlob> PixelShader;
        
#if defined(_DEBUG)
        UINT CompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else 
        UINT CompileFlags = 0;
#endif
        ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, 
            "VSMain", "vs_5_0", CompileFlags, 0, &VertexShader, nullptr));
        ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, 
            "PSMain", "ps_5_0", CompileFlags, 0, &PixelShader, nullptr));

        // Define the vertex input layout
        D3D12_INPUT_ELEMENT_DESC InputElementDescs[] = 
        {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};
        PSODesc.InputLayout = { InputElementDescs, _countof(InputElementDescs) };
        PSODesc.pRootSignature = mRootSignature.Get();
        PSODesc.VS = { reinterpret_cast<UINT8*>(VertexShader->GetBufferPointer()), VertexShader->GetBufferSize() };
        PSODesc.PS = { reinterpret_cast<UINT8*>(PixelShader->GetBufferPointer()), PixelShader->GetBufferSize() };
        PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        PSODesc.DepthStencilState.DepthEnable = FALSE;
        PSODesc.DepthStencilState.StencilEnable = FALSE;
        PSODesc.SampleMask = UINT_MAX;
        PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        PSODesc.NumRenderTargets = 1;
        PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        PSODesc.SampleDesc.Count = 1;
        ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&mPipelineState)));
    }

    // Create the command list
    ThrowIfFailed(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), 
        mPipelineState.Get(), IID_PPV_ARGS(&mCommandList)));

    ThrowIfFailed(mCommandList->Close());

    // Create the vertex buffer
    {
        Vertex TriangleVertices[] = 
        {
            { { 0.0f, 0.25f * m_aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        };

        const UINT VertexBufferSize = sizeof(TriangleVertices);

        CD3DX12_HEAP_PROPERTIES HeapProps(D3D12_HEAP_TYPE_UPLOAD);
        auto Desc = CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize);
        ThrowIfFailed(mDevice->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &Desc, D3D12_RESOURCE_STATE_GENERIC_READ, 
            nullptr, IID_PPV_ARGS(&mVertexBuffer)));
        
        UINT8* pVertexDataBegin;
        CD3DX12_RANGE ReadRange(0, 0);
        ThrowIfFailed(mVertexBuffer->Map(0, &ReadRange, reinterpret_cast<void**>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, TriangleVertices, sizeof(TriangleVertices));
        mVertexBuffer->Unmap(0, nullptr);

        mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
        mVertexBufferView.StrideInBytes = sizeof(Vertex);
        mVertexBufferView.SizeInBytes = VertexBufferSize;
    }

    // Create sync objects and wait until assets have been uploaded to the GPU
    {
        ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
        mFenceValue = 1;

        mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (mFenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        WaitForPreviousFrame();
    }
    
}
    
void D3D12HelloTriangle::PopulateCommandList()
{
    ThrowIfFailed(mCommandAllocator->Reset());
    ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), mPipelineState.Get()));
    
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
    mCommandList->RSSetViewports(1, &mViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets[mFrameIndex].Get(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    mCommandList->ResourceBarrier(1, &Barrier);
    
    CD3DX12_CPU_DESCRIPTOR_HANDLE RtvHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), mFrameIndex, mRtvDescriptorSize);
    mCommandList->OMSetRenderTargets(1, &RtvHandle, FALSE, nullptr);

    // Record commands;
    const float clearColor[] = { 0.f, .2f, .4f, 1.f};
    mCommandList->ClearRenderTargetView(RtvHandle, clearColor, 0, nullptr);
    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mCommandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
    mCommandList->DrawInstanced(3, 1, 0, 0);

    Barrier = CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets[mFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, 
        D3D12_RESOURCE_STATE_PRESENT);

    mCommandList->ResourceBarrier(1, &Barrier);
    ThrowIfFailed(mCommandList->Close());
}
    
void D3D12HelloTriangle::WaitForPreviousFrame()
{
    const UINT64 Fence = mFenceValue;
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), Fence));
    mFenceValue++;

    if (mFence->GetCompletedValue() < Fence)
    {
        ThrowIfFailed(mFence->SetEventOnCompletion(Fence, mFenceEvent));
        WaitForSingleObject(mFenceEvent, INFINITE);
    }

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
}

void D3D12HelloTriangle::OnInit()
{
    LoadPipeline();
    LoadAssets();
}

void D3D12HelloTriangle::OnUpdate()
{

}
    
void D3D12HelloTriangle::OnRender()
{
    PopulateCommandList();

    ID3D12CommandList* ppCommandLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    ThrowIfFailed(mSwapChain->Present(1, 0));

    WaitForPreviousFrame();
}

void D3D12HelloTriangle::OnDestroy()
{
    WaitForPreviousFrame();

    CloseHandle(mFenceEvent);
}
