
#include "stdafx.h"
#include "d3dx12.h"
#include <Windows.h>
#include <cstdio>
#include <iostream>

#include "DirectXMath.h"
#include "DirectXPackedVector.h"

#include "CommonDefines.h"
#include "plog/Log.h"
#include "plog/Initializers/RollingFileInitializer.h"

using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Microsoft::WRL;

namespace D3DInit
{
    static const int SwapChainBufferCount = 2;
    int CurrentBackBuffer = 0;
    UINT ClientWidth = 1280;
    UINT ClientHeight = 720;

    ComPtr<ID3D12Device> MainDevice;
    ComPtr<IDXGIFactory4> DXGIFactory;
    ComPtr<ID3D12Fence> Fence;
    ComPtr<ID3D12CommandQueue> CommandQueue;
    ComPtr<ID3D12GraphicsCommandList> CommandList; 
    ComPtr<ID3D12CommandAllocator> DirectCommandAllocator;
    ComPtr<IDXGISwapChain> SwapChain;
    ComPtr<ID3D12DescriptorHeap> RTVHeap;
    ComPtr<ID3D12DescriptorHeap> DSVHeap;
    ComPtr<ID3D12Resource> SwapChainBuffer[SwapChainBufferCount];
    ComPtr<ID3D12Resource> DepthStencilBuffer;

    UINT RTVDescSize = 0;
    UINT DSVDescSize = 0;
    UINT CBVSRVDescSize = 0;
    UINT MSAA4XQuality = 0;  

    RECT ScissorRect = {0, 0, ClientWidth / 2, ClientHeight / 2};

    void InitializeDevice();
    void CreateFence();
    void CheckForMSAA();
    void CreateCommandObjects();
    void CreateSwapChain();
    void CreateRTVAndDSVDescriptorHeap();
    
    inline D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const 
    {
        return CD3DX12_CPU_DESCRIPTOR_HANDLE(RTVHeap->GetCPUDescriptorHandleForHeapStart(), 
            CurrentBackBuffer, RTVDescSize);
    }

    inline D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const 
    {
        return DSVHeap->GetCPUDescriptorHandleForHeapStart();
    }

    void CreateRenderTargetView();
    void CreateDepthStencilView();

    void SetScissorRectangles();
};
