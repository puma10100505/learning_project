
/*
* 创建窗口的基础代码模板
*
*/
#include "D3DHelloTriangle.h"
#include <iostream>
#include <string.h>
#include <memory>
#include <cstring>
#include <vector>
#include "d3dx12.h"
#include "CommonDefines.h"
#include "DirectX12Window.h"
#include "loguru.hpp"

using namespace std;

void D3D12HelloTriangle::LoadPipeline()
{
    #if defined(_DEBUG)
    {
        ComPtr<ID3D12Debug> DebugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PV_ARGS(&DebugController))))
        {
            DebugController->EnableDebugLayer();
        }
    }
    #endif

    ComPtr<IDXGIFactory4> Factory;
    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&Factory)));

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

    DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
    SwapChainDesc.BufferCount = FrameCount;
    SwapChainDesc.BufferDesc.Width = m_width;
    SwapChainDesc.BufferDesc.Height = m_height;
    SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    SwapChainDesc.OutputWindow = Win32Application::GetHwnd();
    SwapChainDesc.SampleDesc.Count = 1;
    SwapChainDesc.Windowed = TRUE;

    ComPtr<IDXGISwapChain> SwapChain;
    ThrowIfFailed(Factory->CreateSwapChain(mCommandQueue.Get(), &SwapChainDesc, &SwapChain));

    ThrowIfFailed(SwapChain.As(&mSwapChain));
    ThrowIfFailed(Factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

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
        ThrowIfFailed(mDevice->CreateRootSignature(0, Signature->GetBufferPointer(), 
            Signature->GetBufferPointer(), Signature->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));
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
        }

        D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};
        PSODesc.InputLayout = { InputElementDescs, _countof(InputElementDescs) };
        PSODesc.pRootSignature = mRootSignature.Get();
        PSODesc.VS = { reinterpret_cast<UINT8*>(VertexShader->GetBufferPointer()), VertexShader->GetBufferSize() };
        PSODesc.PS = { reinterpret_cast<UINT8*>(PixelShader->GetBufferPointer()), PixelShader->GetBufferSize() };
        PSODesc.RasterizeState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
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

}
    
void D3D12HelloTriangle::WaitForPreviousFrame()
{

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

////////////////////////////////////////////////////////////////////////////////////

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