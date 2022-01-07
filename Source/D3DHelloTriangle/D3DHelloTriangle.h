#pragma once

#include "d3dx12.h"
#include "stdafx.h"
#include "DXSample.h"

using namespace std;
using namespace DirectX;
using namespace Microsoft::WRL;

class D3D12HelloTriangle: public DXSample 
{
public:
    D3D12HelloTriangle(){}
    D3D12HelloTriangle(UINT width, UINT height, std::wstring name);

    virtual void OnInit() override;
    virtual void OnUpdate(float DeltaTime) override;
    virtual void OnRender(float DeltaTime) override;
    virtual void OnGUI(float DeltaTime) override;
    virtual void OnPostGUI(float DeltaTime) override;
    virtual void OnDestroy() override;

private:
    static const UINT FrameCount = 2;

    struct Vertex 
    {
        XMFLOAT3 Position;
        XMFLOAT4 Color;
    };

    D3D12_VIEWPORT mViewport;
    D3D12_RECT mScissorRect;
    ComPtr<IDXGISwapChain3> mSwapChain;    
    ComPtr<ID3D12Resource> mRenderTargets[FrameCount];
    ComPtr<ID3D12CommandAllocator> mCommandAllocator;
    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<ID3D12RootSignature> mRootSignature;    
    ComPtr<ID3D12PipelineState> mPipelineState;
    

    UINT mRtvDescriptorSize;

    ComPtr<ID3D12Resource> mVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;

    UINT mFrameIndex;
    HANDLE mFenceEvent;
    ComPtr<ID3D12Fence> mFence;
    UINT64 mFenceValue;

private:
    void LoadPipeline();
    void LoadAssets();
    void PopulateCommandList();
    void WaitForPreviousFrame();
};