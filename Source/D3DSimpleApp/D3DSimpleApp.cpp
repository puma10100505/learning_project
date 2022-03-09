#include <iostream>
#include <string.h>
#include <memory>
#include <cstring>
#include <vector>

#include "d3dx12.h"
#include "stdafx.h"

#include "CommonDefines.h"

#include "plog/Log.h"
#include "plog/Initializers/RollingFileInitializer.h"

#include "d3dApp.h"
#include <DirectXColors.h>

using namespace LearningD3D;
using namespace DirectX;

class InitDirect3DApp: public D3DApp 
{
    public:
        InitDirect3DApp(HINSTANCE hInstance);
        ~InitDirect3DApp();

        virtual bool Initialize() override;

    private:
        virtual void OnResize() override;
        virtual void Update(const GameTimer& InGameTimer) override;
        virtual void Draw(const GameTimer& InGameTimer) override;
};

InitDirect3DApp::InitDirect3DApp(HINSTANCE hInstance): D3DApp(hInstance)
{

}

InitDirect3DApp::~InitDirect3DApp() {}

bool InitDirect3DApp::Initialize()
{
    if (!D3DApp::Initialize())
    {
        return false;
    }

    return true;
}

void InitDirect3DApp::OnResize()
{
    D3DApp::OnResize();
}

void InitDirect3DApp::Update(const GameTimer& InGameTimer)
{

}

void InitDirect3DApp::Draw(const GameTimer& InGameTimer)
{
    ThrowIfFailed(mDirectCmdListAlloc->Reset());

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    auto TempBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    mCommandList->ResourceBarrier(1, &TempBarrier);

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

    D3D12_CPU_DESCRIPTOR_HANDLE CurrBackBufferView = CurrentBackBufferView();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = DepthStencilView();
    mCommandList->OMSetRenderTargets(1, &CurrBackBufferView, true, &dsv);

    CD3DX12_RESOURCE_BARRIER TempBarrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(), 
        D3D12_RESOURCE_STATE_RENDER_TARGET, 
        D3D12_RESOURCE_STATE_PRESENT
    );
    mCommandList->ResourceBarrier(1, &TempBarrier2);

    ThrowIfFailed(mCommandList->Close());

    ID3D12CommandList* cmdsLists[] = {
        mCommandList.Get()
    };

    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    ThrowIfFailed(mSwapChain->Present(0, 0));
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    FlushCommandQueue();

}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    plog::init(plog::debug,  (DefaultLogDirectory + "D3DHello.log").c_str());

    PLOGD << "Entry of application..." << " CmdLine: " << lpCmdLine << ", file: " << __FILE__;

    #if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    #endif
    
    try
    {
        InitDirect3DApp App(hInstance);
        if (!App.Initialize())
        {
            return EXIT_FAILURE;
        }

        return App.Run();
    }
    catch(DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return EXIT_FAILURE;
    }
    


    return EXIT_SUCCESS;
}