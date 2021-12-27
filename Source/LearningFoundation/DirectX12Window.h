pragma once

#include <iostream>
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

typedef void (*DxWindowTick)(float DeltaTime);
typedef void (*DxWindowGUI)(float DeltaTime);

class DirectX12Window 
{
private:
    DirectX12Window() {}

public:
    static DirectX12Window* GetInstance();

    Initialize(const std::string& InWinTitle, int InWidth, int InHeight, int InPosX, int InPosY);

    DirectX12Window* Loop(DxWindowTick OnTick, DxWindowGUI OnGUI);

    void Shutdown();

private:
    struct FrameContext
    {
        ID3D12CommandAllocator* CommandAllocator;
        UINT64 FenceValue;
    };

    static int const NUM_FRAMES_IN_FLIGHT = 3;

    struct FrameContext g_frameContext[NUM_FRAMES_IN_FLIGHT] = {};
    UINT g_frameIndex = 0;

    int const NUM_BACK_BUFFERS = 3;
    ID3D12Device* g_pd3dDevice = nullptr;
    ID3D12DescriptorHeap* g_pd3dRtvDescHeap = nullptr;
    ID3D12DescriptorHeap* g_pd3dSrvDescHeap = nullptr;
    ID3D12CommandQueue* g_pd3dCommandQueue = nullptr;
    ID3D12GraphicsCommandList* g_pd3dCommandList = nullptr;
    ID3D12Fence* g_fence = nullptr;
    HANDLE g_fenceEvent = nullptr;
    UINT64 g_fenceLastSignaledValue = 0;
    IDXGISwapChain3* g_pSwapChain = nullptr;
    HANDLE g_hSwapChainWaitableObject = nullptr;
    ID3D12Resource* g_mainRenderTargetResource[NUM_BACK_BUFFERS] = {};
    D3D12_CPU_DESCRIPTOR_HANDLE g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS] = {};

    std::string WindowTitle;
    int WindowWidth; 
    int WindowHeight;
    int WindowPosX;
    int WindowPosY;

    HWND hwnd;

private:
    bool CreateDeviceD3D(HWND hWnd);
    void CleanupDeviceD3D();
    void CreateRenderTarget();
    void CleanupRenderTarget();
    void WaitForLastSubmittedFrame();
    FrameContext* WaitForNextFrameResources();
    void ResizeSwapChain(HWND hWnd, int width, int height);
    LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    static DirectX12Window* Inst;
};

