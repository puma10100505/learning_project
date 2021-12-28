#pragma once

#include <iostream>
#include <chrono>
#include <thread>
#include <string.h>
#include <cstring>
#include <memory>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <tchar.h>

#ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

namespace dx
{
    typedef void (*DxWindowTick)(float DeltaTime);
    typedef void (*DxWindowGUI)(float DeltaTime);

    struct FrameContext
    {
        ID3D12CommandAllocator* CommandAllocator;
        UINT64 FenceValue;
    };

    static int const NUM_FRAMES_IN_FLIGHT = 3;
    static FrameContext g_frameContext[NUM_FRAMES_IN_FLIGHT] = {};
    static UINT g_frameIndex = 0;
    static int Dx12WndFPS = 60;
    static std::chrono::steady_clock::time_point LastFrameTimeSecond;
    static const int NUM_BACK_BUFFERS = 3;
    static ID3D12Device* g_pd3dDevice = nullptr;
    static ID3D12DescriptorHeap* g_pd3dRtvDescHeap = nullptr;
    static ID3D12DescriptorHeap* g_pd3dSrvDescHeap = nullptr;
    static ID3D12CommandQueue* g_pd3dCommandQueue = nullptr;
    static ID3D12GraphicsCommandList* g_pd3dCommandList = nullptr;
    static ID3D12Fence* g_fence = nullptr;
    static HANDLE g_fenceEvent = nullptr;
    static UINT64 g_fenceLastSignaledValue = 0;
    static IDXGISwapChain3* g_pSwapChain = nullptr;
    static HANDLE g_hSwapChainWaitableObject = nullptr;
    static ID3D12Resource* g_mainRenderTargetResource[NUM_BACK_BUFFERS] = {};
    static D3D12_CPU_DESCRIPTOR_HANDLE g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS] = {};

    /* Forward declare of Methods */
    LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static bool CreateDeviceD3D(HWND hWnd);
    static void CleanupDeviceD3D();
    static void CreateRenderTarget();
    static void CleanupRenderTarget();
    static void WaitForLastSubmittedFrame();
    static FrameContext* WaitForNextFrameResources();
    static void ResizeSwapChain(HWND hWnd, int width, int height);    

    // Public 
    int CreateWindowInstance(const std::string& InWinTitle,
            int InWidth, int InHeight, 
            int InPosX, int InPosY, 
            DxWindowTick OnTick, DxWindowGUI OnGUI, DxWindowGUI OnPostGUI); 
    
}