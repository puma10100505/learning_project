/*
 * NavViewTextureBridge.cpp
 * ------------------------
 */

#include "NavViewTextureBridge.h"

#if !defined(_WIN32)
#  include <glad/glad.h>
#endif

#if defined(_WIN32)

#  include "DirectX/DirectX12Window.h"

#  include <d3d12.h>
#  include <wrl/client.h>
#  include <cstring>

#  include "d3dx12.h"

namespace
{
using Microsoft::WRL::ComPtr;

ComPtr<ID3D12Resource>      g_TexDefault;
ComPtr<ID3D12Resource>      g_UploadBuffer;
UINT64                      g_UploadBufferSize = 0;

ComPtr<ID3D12CommandAllocator> g_UploadAlloc;
ComPtr<ID3D12GraphicsCommandList> g_UploadList;
ComPtr<ID3D12Fence>         g_UploadFence;
UINT64                      g_UploadFenceValue = 0;
HANDLE                      g_UploadEvent = nullptr;

int                         g_W = 0;
int                         g_H = 0;
ImTextureID                 g_TexId = (ImTextureID)0;
bool                        g_FirstUploadAfterAlloc = true;

bool EnsureUploadInfra(ID3D12Device* dev)
{
    if (g_UploadAlloc && g_UploadList && g_UploadFence && g_UploadEvent)
        return true;
    if (FAILED(dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_UploadAlloc))))
        return false;
    if (FAILED(dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_UploadAlloc.Get(), nullptr,
                                      IID_PPV_ARGS(&g_UploadList))))
        return false;
    g_UploadList->Close();
    if (FAILED(dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_UploadFence))))
        return false;
    g_UploadEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    return g_UploadEvent != nullptr;
}

void GpuWaitForFence(ID3D12CommandQueue* queue)
{
    if (!g_UploadFence || !queue || !g_UploadEvent)
        return;
    ++g_UploadFenceValue;
    if (SUCCEEDED(queue->Signal(g_UploadFence.Get(), g_UploadFenceValue)))
    {
        if (g_UploadFence->GetCompletedValue() < g_UploadFenceValue)
        {
            g_UploadFence->SetEventOnCompletion(g_UploadFenceValue, g_UploadEvent);
            WaitForSingleObject(g_UploadEvent, INFINITE);
        }
    }
}

} // namespace

bool NavViewTextureBridgeAvailable()
{
    return DirectX::GetD3D12Device() != nullptr && DirectX::GetD3D12SrvHeap() != nullptr;
}

ImTextureID NavViewTextureBridgeTextureId() { return g_TexId; }

void NavViewTextureBridgeUpload(int w, int h, const uint8_t* rgba)
{
    ID3D12Device*       dev   = DirectX::GetD3D12Device();
    ID3D12CommandQueue* queue = DirectX::GetD3D12CommandQueue();
    ID3D12DescriptorHeap* srvHeap = DirectX::GetD3D12SrvHeap();
    if (!dev || !queue || !srvHeap || w <= 0 || h <= 0 || !rgba)
        return;
    if (!EnsureUploadInfra(dev))
        return;

    const UINT srvStride = DirectX::GetD3D12SrvDescriptorStride();
    if (srvStride == 0)
        return;

    if (w != g_W || h != g_H || !g_TexDefault)
    {
        g_TexDefault.Reset();
        g_UploadBuffer.Reset();
        g_UploadBufferSize = 0;
        g_W                = w;
        g_H                = h;
        g_FirstUploadAfterAlloc = true;

        const CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
        const CD3DX12_RESOURCE_DESC   texDesc =
            CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM,
                                         static_cast<UINT64>(w), static_cast<UINT>(h),
                                         1, 1, 1, 0, D3D12_RESOURCE_FLAG_NONE);
        if (FAILED(dev->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &texDesc,
                                                D3D12_RESOURCE_STATE_COMMON, nullptr,
                                                IID_PPV_ARGS(&g_TexDefault))))
        {
            g_TexId = (ImTextureID)0;
            g_W = g_H = 0;
            return;
        }

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels       = 1;

        D3D12_CPU_DESCRIPTOR_HANDLE cpu = srvHeap->GetCPUDescriptorHandleForHeapStart();
        cpu.ptr += static_cast<SIZE_T>(DirectX::kNavViewTextureSrvHeapIndex) * srvStride;
        dev->CreateShaderResourceView(g_TexDefault.Get(), &srvDesc, cpu);

        D3D12_GPU_DESCRIPTOR_HANDLE gpu = srvHeap->GetGPUDescriptorHandleForHeapStart();
        gpu.ptr += static_cast<UINT64>(DirectX::kNavViewTextureSrvHeapIndex) * srvStride;
        g_TexId = (ImTextureID)gpu.ptr;
    }

    const UINT64 reqIntermediate = GetRequiredIntermediateSize(g_TexDefault.Get(), 0, 1);
    if (reqIntermediate == 0)
        return;

    if (!g_UploadBuffer || g_UploadBufferSize < reqIntermediate)
    {
        g_UploadBuffer.Reset();
        g_UploadBufferSize = 0;

        const CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
        const auto uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(reqIntermediate);
        if (FAILED(dev->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &uploadDesc,
                                                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                IID_PPV_ARGS(&g_UploadBuffer))))
            return;
        g_UploadBufferSize = reqIntermediate;
    }

    D3D12_SUBRESOURCE_DATA sub{};
    sub.pData      = rgba;
    sub.RowPitch   = static_cast<UINT>(w * 4);
    sub.SlicePitch = static_cast<UINT>(w * h * 4);

    if (FAILED(g_UploadAlloc->Reset()))
        return;
    if (FAILED(g_UploadList->Reset(g_UploadAlloc.Get(), nullptr)))
        return;

    if (g_FirstUploadAfterAlloc)
    {
        const CD3DX12_RESOURCE_BARRIER b0 = CD3DX12_RESOURCE_BARRIER::Transition(
            g_TexDefault.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
        g_UploadList->ResourceBarrier(1, &b0);
        g_FirstUploadAfterAlloc = false;
    }
    else
    {
        const CD3DX12_RESOURCE_BARRIER b0 = CD3DX12_RESOURCE_BARRIER::Transition(
            g_TexDefault.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_COPY_DEST);
        g_UploadList->ResourceBarrier(1, &b0);
    }

    const UINT64 updated = UpdateSubresources<1>(
        g_UploadList.Get(), g_TexDefault.Get(), g_UploadBuffer.Get(), 0, 0, 1, &sub);
    if (updated == 0)
        return;

    const CD3DX12_RESOURCE_BARRIER b1 = CD3DX12_RESOURCE_BARRIER::Transition(
        g_TexDefault.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    g_UploadList->ResourceBarrier(1, &b1);

    if (FAILED(g_UploadList->Close()))
        return;

    ID3D12CommandList* const lists[] = { g_UploadList.Get() };
    queue->ExecuteCommandLists(1, lists);
    GpuWaitForFence(queue);
}

#else

namespace
{
GLuint       g_Tex = 0;
ImTextureID  g_Id  = (ImTextureID)0;
int          g_W   = 0;
int          g_H   = 0;
} // namespace

bool NavViewTextureBridgeAvailable() { return true; }

ImTextureID NavViewTextureBridgeTextureId()
{
    if (g_Tex == 0)
    {
        glGenTextures(1, &g_Tex);
        g_Id = (ImTextureID)(intptr_t)g_Tex;
        glBindTexture(GL_TEXTURE_2D, g_Tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    return g_Id;
}

void NavViewTextureBridgeUpload(int w, int h, const uint8_t* rgba)
{
    if (w <= 0 || h <= 0 || !rgba) return;
    NavViewTextureBridgeTextureId();
    glBindTexture(GL_TEXTURE_2D, g_Tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (w != g_W || h != g_H)
    {
        g_W = w;
        g_H = h;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    }
    else
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
}

#endif
