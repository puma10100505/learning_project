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

struct FrameContext
{
    ID3D12CommandAllocator* CommandAllocator;
    UINT64 FenceValue;
};

LRESULT WINAPI Dx12Window_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int const NUM_FRAMES_IN_FLIGHT = 3;

FrameContext g_frameContext[NUM_FRAMES_IN_FLIGHT] = {};
UINT g_frameIndex = 0;
int Dx12WndFPS = 60;
std::chrono::steady_clock::time_point LastFrameTimeSecond;
const int NUM_BACK_BUFFERS = 3;
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

typedef void (*DxWindowTick)(float DeltaTime);
typedef void (*DxWindowGUI)(float DeltaTime);

bool Dx12Window_CreateDeviceD3D(HWND hWnd);
void Dx12Window_CleanupDeviceD3D();
void Dx12Window_CreateRenderTarget();
void Dx12Window_CleanupRenderTarget();
void Dx12Window_WaitForLastSubmittedFrame();
FrameContext* Dx12Window_WaitForNextFrameResources();
void Dx12Window_ResizeSwapChain(HWND hWnd, int width, int height);    

// Public
int Dx12Window_CreateInstance( 
    int InWidth, int InHeight, 
    int InPosX, int InPosY, 
    DxWindowTick OnTick, DxWindowGUI OnGUI);