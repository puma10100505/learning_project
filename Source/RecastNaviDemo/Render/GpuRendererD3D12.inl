/*
 * GpuRendererD3D12.inl — 仅由 GpuRenderer.cpp 在 _WIN32 下 #include
 * D3D12 离屏渲染：与 OpenGL 版同接口；Lambert + 方向光 ShadowMap + 深度测试；片元不透明输出。
 */

#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

#include "d3dx12.h"
#include "DirectX/DirectX12Window.h"

#pragma comment(lib, "d3dcompiler.lib")

namespace GpuRenderer
{
namespace D3D
{

using Microsoft::WRL::ComPtr;

struct Vertex
{
    float x, y, z, r, g, b, a, nx, ny, nz, lit;
};

/// 与下方 HLSL cbuffer Globals 布局一致（256B，float4 对齐）
struct alignas(256) GlobalsCB
{
    float vp[16];
    float lightVp[16];
    float pack0[4]; // lightType(0/1), intensity, ambient, shadowStrength
    float pack1[4]; // lightVec.xyz, shadowBias
    float pack2[4]; // lightColor.rgb, shadowPcf(0/1)
    float pack3[4]; // shadowsOn(0/1), 0,0,0
};

bool                      g_InFrame  = false;
bool                      g_ShaderOk = false;

ComPtr<ID3D12RootSignature> g_RootSig;
ComPtr<ID3D12PipelineState>  g_PsoMain;
ComPtr<ID3D12PipelineState>  g_PsoShadow;

ComPtr<ID3D12Resource> g_ColorTex, g_DepthMain, g_ShadowMap, g_VbDefault, g_UploadScratch, g_UploadCB;
ComPtr<ID3D12CommandAllocator>     g_UploadAlloc;
ComPtr<ID3D12GraphicsCommandList> g_UploadList;
ComPtr<ID3D12Fence>               g_Fence;
HANDLE g_FenceEvent = nullptr;
UINT64 g_FenceValue = 0;

ComPtr<ID3D12DescriptorHeap> g_RtvHeap, g_DsvHeap;
UINT g_RtvStride = 0, g_DsvStride = 0;
UINT g_DsvShadowOff = 0;

D3D12_RESOURCE_STATES g_ColorState     = D3D12_RESOURCE_STATE_COMMON;
D3D12_RESOURCE_STATES g_DepthMainState = D3D12_RESOURCE_STATE_COMMON;
D3D12_RESOURCE_STATES g_ShadowState    = D3D12_RESOURCE_STATE_DEPTH_WRITE;

bool                  g_VbOnce = false;
float                 g_ClearC[4]    = { 0.11f, 0.12f, 0.14f, 1.f };
int                   g_Rw = 0, g_Rh = 0, g_ShadowSize = 0;
Mat4                  g_Vp{}, g_LightVp{};
LightParams           g_L{};
Vec3                  g_Eye{};
float                 g_Fovy = 0.f, g_WorldDiag = 1.f;
int                   g_SamplesIgnored = 0;
std::vector<Vertex>   g_Verts;

void CopyMat4(const Mat4& m, float* o) { std::memcpy(o, m.m, 64); }

Mat4 ComputeDirectionalLightVP(Vec3 lightDirWorld, Vec3 sceneCenter, float sceneRadius)
{
    Vec3 dir = Normalize(lightDirWorld);
    if (Length(dir) < 1e-6f) dir = V3(0, 1, 0);
    const float r        = std::max(sceneRadius, 1.0f);
    const Vec3  lightPos = sceneCenter + dir * (r * 1.5f);
    Vec3        up       = (std::fabs(dir.y) > 0.95f) ? V3(0, 0, 1) : V3(0, 1, 0);
    const Mat4 view = MakeLookAtRH(lightPos, sceneCenter, up);
    const float ext  = r * 1.2f;
    const Mat4  proj = MakeOrthoRH(-ext, ext, -ext, ext, 0.1f, 3.0f * r);
    return MatMul(proj, view);
}

void UnpackCol(ImU32 col, float& r, float& g, float& b, float& a)
{
    r = static_cast<float>((col >> IM_COL32_R_SHIFT) & 0xFF) * (1.0f / 255.0f);
    g = static_cast<float>((col >> IM_COL32_G_SHIFT) & 0xFF) * (1.0f / 255.0f);
    b = static_cast<float>((col >> IM_COL32_B_SHIFT) & 0xFF) * (1.0f / 255.0f);
    a = static_cast<float>((col >> IM_COL32_A_SHIFT) & 0xFF) * (1.0f / 255.0f);
}

void EmitV(std::vector<Vertex>& v, Vec3 p, float r, float g, float b, float a, Vec3 n, float lit)
{
    v.push_back({ p.x, p.y, p.z, r, g, b, a, n.x, n.y, n.z, lit });
}
void EmitTri(std::vector<Vertex>& v, Vec3 a, Vec3 b, Vec3 c, ImU32 col, Vec3 n, float lit)
{
    float r, g, bl, al;
    UnpackCol(col, r, g, bl, al);
    EmitV(v, a, r, g, bl, al, n, lit);
    EmitV(v, b, r, g, bl, al, n, lit);
    EmitV(v, c, r, g, bl, al, n, lit);
}

void GpuWait(ID3D12CommandQueue* queue)
{
    if (!g_Fence || !queue || !g_FenceEvent) return;
    ++g_FenceValue;
    if (SUCCEEDED(queue->Signal(g_Fence.Get(), g_FenceValue)))
    {
        if (g_Fence->GetCompletedValue() < g_FenceValue)
        {
            g_Fence->SetEventOnCompletion(g_FenceValue, g_FenceEvent);
            WaitForSingleObject(g_FenceEvent, INFINITE);
        }
    }
}

bool EnsureInfra()
{
    ID3D12Device* dev = DirectX::GetD3D12Device();
    if (!dev) return false;
    if (g_UploadAlloc) return true;
    if (FAILED(dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_UploadAlloc))))
        return false;
    if (FAILED(dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_UploadAlloc.Get(), nullptr,
                                      IID_PPV_ARGS(&g_UploadList))))
        return false;
    g_UploadList->Close();
    if (FAILED(dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_Fence)))) return false;
    g_FenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    return g_FenceEvent != nullptr;
}

static const char* kVsMain = R"(
struct VSIn { float3 pos:POSITION; float4 col:COLOR; float3 nrm:NORMAL; float lit:TEXCOORD0; };
struct VSOut {
  float4 clip:SV_POSITION; float4 col:COLOR; float3 nrm:NORMAL; float3 wpos:WORLDPOS;
  float lit:TEXCOORD0; float4 lsp:TEXCOORD1;
};
cbuffer Globals : register(b0) {
  row_major float4x4 VP; row_major float4x4 LightVP;
  float4 Pack0,Pack1,Pack2,Pack3;
};
// C++ 端 Mat4 是 row-major 行优先存储、按 M·v（列向量）约定使用，
// 与 row_major 标记的 HLSL 矩阵在内存布局上完全一致；HLSL 中 M·v 必须写成 mul(M, v)，
// 写成 mul(v, M) 会得到 v·M（相当于乘了 M 的转置），场景就会"乱"。
float4 GLClipToD3D(float4 h) {
  h.z = mad(0.5, h.w, 0.5 * h.z);
  return h;
}
VSOut main(VSIn i) {
  VSOut o;
  float4 wp=float4(i.pos,1);
  o.clip = GLClipToD3D(mul(VP, wp));
  o.col=i.col; o.nrm=i.nrm; o.wpos=i.pos; o.lit=i.lit;
  o.lsp = GLClipToD3D(mul(LightVP, wp));
  return o;
}
)";

static const char* kPsMain = R"(
struct PSIn {
  float4 clip:SV_POSITION; float4 col:COLOR; float3 nrm:NORMAL; float3 wpos:WORLDPOS;
  float lit:TEXCOORD0; float4 lsp:TEXCOORD1;
};
cbuffer Globals : register(b0) {
  row_major float4x4 VP; row_major float4x4 LightVP;
  float4 Pack0,Pack1,Pack2,Pack3;
};
Texture2D<float> ShadowMap : register(t0);
SamplerState ShadowSamp : register(s0);
float SVis(PSIn pin, float3 N, float3 L) {
  float4 ls=pin.lsp;
  float w=max(abs(ls.w),1e-6);
  // D3D: v=0 在纹理顶部，NDC y=+1 渲染到 v=0；GL 公式 (ndc.y*0.5+0.5) 在 D3D 下是上下镜像的，
  // 阴影会错位。这里对 v 轴做翻转：uv.y = 1 - (ndc.y*0.5+0.5) = 0.5 - 0.5*ndc.y。
  float2 uv = float2(ls.x / w * 0.5 + 0.5, 0.5 - ls.y / w * 0.5);
  float z_ndc = ls.z / w;
  if(z_ndc>1.0||z_ndc<0.0||uv.x<0.0||uv.x>1.0||uv.y<0.0||uv.y>1.0) return 1.0;
  float sb=Pack1.w;
  float bias=max(sb*(1.0-dot(N,L)), sb*0.1);
  float cur=z_ndc-bias;
  int pcf=(int)round(Pack2.w);
  float vis=1.0;
  if(pcf==0){ float c=ShadowMap.Sample(ShadowSamp,uv).r; vis=(cur>c)?0.0:1.0; }
  else {
    uint W,H; ShadowMap.GetDimensions(W,H); float2 t=1.0/float2(W,H);
    float s=0;
    for(int y=-1;y<=1;++y)for(int x=-1;x<=1;++x){
      float c=ShadowMap.Sample(ShadowSamp,uv+float2(x,y)*t).r;
      s+=(cur>c)?0.0:1.0;
    }
    vis=s/9.0;
  }
  return vis;
}
float4 main(PSIn pin):SV_Target0 {
  if(pin.lit<0.5) return float4(pin.col.rgb,1);
  float3 N=normalize(pin.nrm);
  int lty=(int)round(Pack0.x);
  float3 L; float att=1.0;
  if(lty==1){
    float3 toL=float3(Pack1.x,Pack1.y,Pack1.z)-pin.wpos;
    float d2=dot(toL,toL); float d=sqrt(max(d2,1e-8));
    L=toL/d;
    float r2=max(dot(float3(Pack1.x,Pack1.y,Pack1.z),float3(Pack1.x,Pack1.y,Pack1.z)),1e-4);
    att=r2/(r2+d2);
  } else L=normalize(float3(Pack1.x,Pack1.y,Pack1.z));
  float vis=1.0;
  if((int)round(Pack3.x)==1 && lty==0){
    float v=SVis(pin,N,L);
    vis=lerp(1.0-Pack0.w,1.0,v);
  }
  float NdotL=max(dot(N,L),0.0);
  float diffK=NdotL*Pack0.y*att*vis;
  float3 litMul=float3(Pack0.z,Pack0.z,Pack0.z)+(1.0-Pack0.z)*diffK*float3(Pack2.x,Pack2.y,Pack2.z);
  float3 mult=lerp(float3(1,1,1),litMul,pin.lit);
  return float4(pin.col.rgb*mult,1);
}
)";

static const char* kVsShadow = R"(
struct VSIn { float3 pos:POSITION; float4 col:COLOR; float3 nrm:NORMAL; float lit:TEXCOORD0; };
cbuffer Globals : register(b0) {
  row_major float4x4 VP; row_major float4x4 LightVP;
  float4 Pack0,Pack1,Pack2,Pack3;
};
float4 main(VSIn i):SV_POSITION {
  if(i.lit<0.5) return float4(2,2,2,1);
  float4 h = mul(LightVP, float4(i.pos,1.0));
  h.z = mad(0.5, h.w, 0.5 * h.z);
  return h;
}
)";

bool Compile(const char* src, const char* entry, const char* prof, ComPtr<ID3DBlob>& out)
{
    ComPtr<ID3DBlob> err;
    UINT fl = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
    fl |= D3DCOMPILE_DEBUG;
#endif
    if (FAILED(D3DCompile(src, strlen(src), nullptr, nullptr, nullptr, entry, prof, fl, 0, &out, &err)))
    {
        if (err && err->GetBufferPointer())
            std::fprintf(stderr, "[GpuRenderer D3D12] %s\n", (char*)err->GetBufferPointer());
        return false;
    }
    return true;
}

bool BuildPipelines()
{
    ID3D12Device* dev = DirectX::GetD3D12Device();
    if (!dev) return false;
    ComPtr<ID3DBlob> vs, ps, sh, rsb, ser;
    if (!Compile(kVsMain, "main", "vs_5_0", vs)) return false;
    if (!Compile(kPsMain, "main", "ps_5_0", ps)) return false;
    if (!Compile(kVsShadow, "main", "vs_5_0", sh)) return false;

    CD3DX12_DESCRIPTOR_RANGE rng;
    rng.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    CD3DX12_ROOT_PARAMETER rp[2];
    rp[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
    rp[1].InitAsDescriptorTable(1, &rng, D3D12_SHADER_VISIBILITY_PIXEL);
    CD3DX12_STATIC_SAMPLER_DESC ss(
        0, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER, 0, 0,
        D3D12_COMPARISON_FUNC_NEVER, D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
        0.f, D3D12_FLOAT32_MAX, D3D12_SHADER_VISIBILITY_PIXEL);
    CD3DX12_ROOT_SIGNATURE_DESC rs(2, rp, 1, &ss, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    if (FAILED(D3D12SerializeRootSignature(&rs, D3D_ROOT_SIGNATURE_VERSION_1, &rsb, &ser))) return false;
    if (FAILED(dev->CreateRootSignature(0, rsb->GetBufferPointer(), rsb->GetBufferSize(), IID_PPV_ARGS(&g_RootSig))))
        return false;

    D3D12_INPUT_ELEMENT_DESC il[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    // 用 D3D12_DEFAULT 给 RT/DS/Blend 全字段填正确默认值（enum 从 1 起，零初始化会触发
    // 部分驱动/调试层拒绝创建 PSO；这是“场景什么都没渲染”问题的核心根因）。
    CD3DX12_RASTERIZER_DESC   raster(D3D12_DEFAULT);
    raster.FillMode            = D3D12_FILL_MODE_SOLID;
    raster.CullMode            = D3D12_CULL_MODE_NONE;
    raster.FrontCounterClockwise = FALSE;
    raster.DepthClipEnable     = TRUE;

    CD3DX12_DEPTH_STENCIL_DESC depth(D3D12_DEFAULT);
    depth.DepthEnable    = TRUE;
    depth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depth.DepthFunc      = D3D12_COMPARISON_FUNC_LESS;
    depth.StencilEnable  = FALSE;

    CD3DX12_BLEND_DESC blend(D3D12_DEFAULT);
    blend.RenderTarget[0].BlendEnable           = FALSE;
    blend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC P{};
    P.pRootSignature      = g_RootSig.Get();
    P.VS                  = { vs->GetBufferPointer(), vs->GetBufferSize() };
    P.PS                  = { ps->GetBufferPointer(), ps->GetBufferSize() };
    P.BlendState          = blend;
    P.SampleMask          = UINT_MAX;
    P.RasterizerState     = raster;
    P.DepthStencilState   = depth;
    P.InputLayout.pInputElementDescs = il;
    P.InputLayout.NumElements        = _countof(il);
    P.PrimitiveTopologyType          = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    P.NumRenderTargets               = 1;
    P.RTVFormats[0]                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    P.DSVFormat                      = DXGI_FORMAT_D32_FLOAT;
    P.SampleDesc.Count               = 1;
    P.SampleDesc.Quality             = 0;
    if (FAILED(dev->CreateGraphicsPipelineState(&P, IID_PPV_ARGS(&g_PsoMain))))
    {
        std::fprintf(stderr, "[GpuRenderer D3D12] CreateGraphicsPipelineState(main) failed\n");
        return false;
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC S = P;
    S.VS                 = { sh->GetBufferPointer(), sh->GetBufferSize() };
    S.PS                 = { nullptr, 0 };
    S.NumRenderTargets   = 0;
    for (UINT i = 0; i < 8; ++i) S.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
    if (FAILED(dev->CreateGraphicsPipelineState(&S, IID_PPV_ARGS(&g_PsoShadow))))
    {
        std::fprintf(stderr, "[GpuRenderer D3D12] CreateGraphicsPipelineState(shadow) failed\n");
        return false;
    }
    return true;
}

bool BuildPipelinesOnce()
{
    if (g_ShaderOk) return true;
    if (!BuildPipelines()) return false;
    g_ShaderOk = true;
    return true;
}

bool EnsureRtvDsvHeaps(ID3D12Device* dev)
{
    if (g_RtvHeap && g_DsvHeap) return true;
    D3D12_DESCRIPTOR_HEAP_DESC h{};
    h.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    h.NumDescriptors = 1;
    if (FAILED(dev->CreateDescriptorHeap(&h, IID_PPV_ARGS(&g_RtvHeap)))) return false;
    h.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    h.NumDescriptors = 2;
    if (FAILED(dev->CreateDescriptorHeap(&h, IID_PPV_ARGS(&g_DsvHeap)))) return false;
    g_RtvStride     = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    g_DsvStride     = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    g_DsvShadowOff  = g_DsvStride;
    return true;
}

bool EnsureCbUpload(ID3D12Device* dev)
{
    if (g_UploadCB) return true;
    CD3DX12_HEAP_PROPERTIES hp(D3D12_HEAP_TYPE_UPLOAD);
    auto bd = CD3DX12_RESOURCE_DESC::Buffer(sizeof(GlobalsCB));
    return SUCCEEDED(dev->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &bd,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&g_UploadCB)));
}

void ReleaseSized()
{
    g_ColorTex.Reset();
    g_DepthMain.Reset();
    g_ShadowMap.Reset();
}

bool EnsureTargets(int rw, int rh, int shadowSz)
{
    ID3D12Device* dev = DirectX::GetD3D12Device();
    if (!dev || rw <= 0 || rh <= 0) return false;
    shadowSz = std::max(4, shadowSz);

    if (g_Rw == rw && g_Rh == rh && g_ShadowSize == shadowSz && g_ColorTex) return true;

    ReleaseSized();
    g_Rw = rw;
    g_Rh = rh;
    g_ShadowSize = shadowSz;

    if (!EnsureRtvDsvHeaps(dev)) return false;

    CD3DX12_HEAP_PROPERTIES def(D3D12_HEAP_TYPE_DEFAULT);
    float clearC[4]       = { 0, 0, 0, 0 };
    CD3DX12_CLEAR_VALUE   cv(DXGI_FORMAT_R8G8B8A8_UNORM, clearC);
    CD3DX12_RESOURCE_DESC cr =
        CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, static_cast<UINT64>(rw),
                                   static_cast<UINT>(rh));
    cr.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    if (FAILED(dev->CreateCommittedResource(&def, D3D12_HEAP_FLAG_NONE, &cr, D3D12_RESOURCE_STATE_COMMON,
                                            &cv, IID_PPV_ARGS(&g_ColorTex))))
        return false;
    g_ColorState = D3D12_RESOURCE_STATE_COMMON;

    dev->CreateRenderTargetView(g_ColorTex.Get(), nullptr, g_RtvHeap->GetCPUDescriptorHandleForHeapStart());

    CD3DX12_CLEAR_VALUE cvD(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);
    CD3DX12_RESOURCE_DESC dp =
        CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, static_cast<UINT64>(rw), static_cast<UINT>(rh));
    dp.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    if (FAILED(dev->CreateCommittedResource(&def, D3D12_HEAP_FLAG_NONE, &dp, D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                            &cvD, IID_PPV_ARGS(&g_DepthMain))))
        return false;
    g_DepthMainState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

    D3D12_CPU_DESCRIPTOR_HANDLE dsv0 = g_DsvHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_DEPTH_STENCIL_VIEW_DESC dvd{};
    dvd.Format        = DXGI_FORMAT_D32_FLOAT;
    dvd.ViewDimension   = D3D12_DSV_DIMENSION_TEXTURE2D;
    dvd.Texture2D.MipSlice = 0;
    dev->CreateDepthStencilView(g_DepthMain.Get(), &dvd, dsv0);

    CD3DX12_RESOURCE_DESC sh =
        CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_TYPELESS, static_cast<UINT64>(shadowSz),
                                   static_cast<UINT>(shadowSz));
    sh.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    if (FAILED(dev->CreateCommittedResource(&def, D3D12_HEAP_FLAG_NONE, &sh, D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                            &cvD, IID_PPV_ARGS(&g_ShadowMap))))
        return false;
    g_ShadowState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

    D3D12_CPU_DESCRIPTOR_HANDLE dsv1 = dsv0;
    dsv1.ptr += g_DsvShadowOff;
    dev->CreateDepthStencilView(g_ShadowMap.Get(), &dvd, dsv1);

    ID3D12DescriptorHeap* srvH = DirectX::GetD3D12SrvHeap();
    UINT                  inc  = DirectX::GetD3D12SrvDescriptorStride();
    D3D12_CPU_DESCRIPTOR_HANDLE sc = srvH->GetCPUDescriptorHandleForHeapStart();
    sc.ptr += DirectX::kGpuRendererShadowSrvHeapIndex * inc;
    D3D12_SHADER_RESOURCE_VIEW_DESC sv{};
    sv.Format                    = DXGI_FORMAT_R32_FLOAT;
    sv.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
    sv.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    sv.Texture2D.MipLevels       = 1;
    dev->CreateShaderResourceView(g_ShadowMap.Get(), &sv, sc);

    sc = srvH->GetCPUDescriptorHandleForHeapStart();
    sc.ptr += DirectX::kGpuRendererColorSrvHeapIndex * inc;
    D3D12_SHADER_RESOURCE_VIEW_DESC svc{};
    svc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    svc.ViewDimension            = D3D12_SRV_DIMENSION_TEXTURE2D;
    svc.Shader4ComponentMapping  = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    svc.Texture2D.MipLevels      = 1;
    dev->CreateShaderResourceView(g_ColorTex.Get(), &svc, sc);

    return true;
}

bool EnsureVb(ID3D12Device* dev, UINT64 bytes)
{
    if (g_VbDefault && g_VbDefault->GetDesc().Width >= bytes) return true;
    g_VbDefault.Reset();
    g_VbOnce = false;
    CD3DX12_HEAP_PROPERTIES hp(D3D12_HEAP_TYPE_DEFAULT);
    auto bd = CD3DX12_RESOURCE_DESC::Buffer(bytes);
    return SUCCEEDED(dev->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &bd, D3D12_RESOURCE_STATE_COMMON,
                                                  nullptr, IID_PPV_ARGS(&g_VbDefault)));
}

bool UploadVb(ID3D12GraphicsCommandList* cmd, const std::vector<Vertex>& verts, UINT64& gpuAddr)
{
    if (verts.empty()) return false;
    ID3D12Device* dev = DirectX::GetD3D12Device();
    UINT64        sz  = verts.size() * sizeof(Vertex);
    if (!EnsureVb(dev, sz)) return false;

    UINT64 req = GetRequiredIntermediateSize(g_VbDefault.Get(), 0, 1);
    if (!g_UploadScratch || g_UploadScratch->GetDesc().Width < req)
    {
        g_UploadScratch.Reset();
        CD3DX12_HEAP_PROPERTIES up(D3D12_HEAP_TYPE_UPLOAD);
        auto ud = CD3DX12_RESOURCE_DESC::Buffer(req);
        if (FAILED(dev->CreateCommittedResource(&up, D3D12_HEAP_FLAG_NONE, &ud,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&g_UploadScratch))))
            return false;
    }
    D3D12_SUBRESOURCE_DATA sub{};
    sub.pData      = verts.data();
    sub.RowPitch   = static_cast<UINT>(sz);
    sub.SlicePitch = static_cast<UINT>(sz);

    const D3D12_RESOURCE_STATES before =
        g_VbOnce ? D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER : D3D12_RESOURCE_STATE_COMMON;
    auto b0 = CD3DX12_RESOURCE_BARRIER::Transition(g_VbDefault.Get(), before,
                                                   D3D12_RESOURCE_STATE_COPY_DEST);
    cmd->ResourceBarrier(1, &b0);
    UpdateSubresources<1>(cmd, g_VbDefault.Get(), g_UploadScratch.Get(), 0, 0, 1, &sub);
    auto b1 = CD3DX12_RESOURCE_BARRIER::Transition(g_VbDefault.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                   D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    cmd->ResourceBarrier(1, &b1);
    g_VbOnce = true;
    gpuAddr  = g_VbDefault->GetGPUVirtualAddress();
    return true;
}

} // namespace D3D

bool Available()
{
    if (!DirectX::GetD3D12Device()) return false;
    if (!D3D::EnsureInfra()) return false;
    // 延迟创建设备对象（RTV/SRV 堆在首次 Begin 才需要完整尺寸）
    return D3D::BuildPipelinesOnce();
}

bool IsActive() { return D3D::g_InFrame; }
int  BufferWidth() { return D3D::g_Rw; }
int  BufferHeight() { return D3D::g_Rh; }

ImTextureID TextureId()
{
    ID3D12DescriptorHeap* h = DirectX::GetD3D12SrvHeap();
    if (!h) return (ImTextureID)0;
    D3D12_GPU_DESCRIPTOR_HANDLE g = h->GetGPUDescriptorHandleForHeapStart();
    g.ptr += UINT64(DirectX::kGpuRendererColorSrvHeapIndex) * DirectX::GetD3D12SrvDescriptorStride();
    return (ImTextureID)g.ptr;
}

void Begin(int rw, int rh, ImU32 clearColor, const Mat4& vp, float fovyRad, const Vec3& eyeWorld,
           float worldBoundsDiagonal, int samples, const LightParams& light)
{
    using namespace D3D;
    float cr, cg, cb, ca;
    UnpackCol(clearColor, cr, cg, cb, ca);
    g_ClearC[0] = cr;
    g_ClearC[1] = cg;
    g_ClearC[2] = cb;
    g_ClearC[3] = ca;

    ID3D12Device* dev = DirectX::GetD3D12Device();
    if (!dev || !BuildPipelinesOnce()) return;
    if (!EnsureCbUpload(dev)) return;

    if (samples > 1) g_SamplesIgnored = samples;

    rw = std::max(1, rw);
    rh = std::max(1, rh);
    const int shSz =
        (light.shadowsOn && light.type == 0) ? std::clamp(light.shadowMapSize, 64, 4096) : 256;
    if (!EnsureTargets(rw, rh, shSz)) return;

    g_InFrame   = true;
    g_Vp        = vp;
    g_Eye       = eyeWorld;
    g_Fovy      = fovyRad;
    g_WorldDiag = std::max(worldBoundsDiagonal, 1.f);
    g_L         = light;
    g_LightVp   = (light.shadowsOn && light.type == 0)
                    ? ComputeDirectionalLightVP(light.vec, light.sceneCenter, light.sceneRadius)
                    : MakeIdentity();
    g_Verts.clear();
}

void DrawTriangleWorld(const Mat4&, Vec3 a, Vec3 b, Vec3 c, ImU32 col, bool cullBackface)
{
    using namespace D3D;
    if (!g_InFrame) return;
    const Vec3  e1 = b - a, e2 = c - a;
    Vec3        n  = Cross(e1, e2);
    const float nl = Length(n);
    if (nl < 1e-8f) return;
    n = n * (1.0f / nl);
    if (cullBackface)
    {
        const Vec3 ctr   = (a + b + c) * (1.0f / 3.0f);
        const Vec3 toEye = Normalize(g_Eye - ctr);
        if (Dot(n, toEye) <= 0.0f) return;
    }
    EmitTri(g_Verts, a, b, c, col, n, 1.0f);
}

void DrawLineWorld(const Mat4&, Vec3 a, Vec3 b, ImU32 col, float thicknessPixels)
{
    using namespace D3D;
    if (!g_InFrame) return;
    const Vec3  ab  = b - a;
    const float len = Length(ab);
    if (len < 1e-6f) return;
    // 每端各算 halfWidth，绘制成梯形，确保屏幕宽度与线长无关（恒定 thicknessPixels）。
    const float distA = std::max(Length(a - g_Eye), 0.1f);
    const float distB = std::max(Length(b - g_Eye), 0.1f);
    const float halfH = static_cast<float>(std::max(g_Rh, 1));
    const float k     = (2.0f * std::tan(g_Fovy * 0.5f)) / std::max(halfH, 1.0f);
    const float floorW   = g_WorldDiag * 1e-4f;
    const float halfWidA = std::max(thicknessPixels * k * 0.5f * distA, floorW);
    const float halfWidB = std::max(thicknessPixels * k * 0.5f * distB, floorW);
    const Vec3 eab = ab * (1.0f / len);
    Vec3       toEye = g_Eye - (a + b) * 0.5f;
    if (Length(toEye) < 1e-4f) toEye = V3(0, 1, 0);
    toEye = Normalize(toEye);
    Vec3 side = Cross(eab, toEye);
    if (Length(side) < 1e-4f) side = Cross(eab, V3(0, 1, 0));
    if (Length(side) < 1e-4f) return;
    side = Normalize(side);
    const Vec3 sA = side * halfWidA;
    const Vec3 sB = side * halfWidB;
    const Vec3 p0 = a + sA, p1 = a - sA, p2 = b - sB, p3 = b + sB;
    const Vec3 nd = V3(0, 1, 0);
    EmitTri(g_Verts, p0, p1, p2, col, nd, 0.0f);
    EmitTri(g_Verts, p0, p2, p3, col, nd, 0.0f);
}

void End()
{
    using namespace D3D;
    if (!g_InFrame) return;
    g_InFrame = false;

    ID3D12Device*       dev   = DirectX::GetD3D12Device();
    ID3D12CommandQueue* queue = DirectX::GetD3D12CommandQueue();
    if (!dev || !queue || !g_ShaderOk || !g_ColorTex) return;

    const bool shadowPass = !g_Verts.empty() && g_L.shadowsOn && g_L.type == 0 && g_ShadowMap;
    const bool hasGeom    = !g_Verts.empty();

    GlobalsCB G{};
    CopyMat4(g_Vp, G.vp);
    CopyMat4(g_LightVp, G.lightVp);
    G.pack0[0] = static_cast<float>(g_L.type);
    G.pack0[1] = g_L.intensity;
    G.pack0[2] = g_L.ambient;
    G.pack0[3] = g_L.shadowStrength;
    G.pack1[0] = g_L.vec.x;
    G.pack1[1] = g_L.vec.y;
    G.pack1[2] = g_L.vec.z;
    G.pack1[3] = g_L.shadowBias;
    G.pack2[0] = g_L.color.x;
    G.pack2[1] = g_L.color.y;
    G.pack2[2] = g_L.color.z;
    G.pack2[3] = static_cast<float>(g_L.shadowPcf);
    G.pack3[0] = shadowPass ? 1.0f : 0.0f;

    void* map = nullptr;
    if (FAILED(g_UploadCB->Map(0, nullptr, &map)) || !map) return;
    std::memcpy(map, &G, sizeof(G));
    g_UploadCB->Unmap(0, nullptr);
    const D3D12_GPU_VIRTUAL_ADDRESS cbAddr = g_UploadCB->GetGPUVirtualAddress();

    g_UploadAlloc->Reset();
    g_UploadList->Reset(g_UploadAlloc.Get(), nullptr);
    ID3D12GraphicsCommandList* cmd = g_UploadList.Get();
    ID3D12DescriptorHeap*      heaps[] = { DirectX::GetD3D12SrvHeap() };
    cmd->SetDescriptorHeaps(1, heaps);

    UINT64 vbGpu = 0;
    if (hasGeom && !UploadVb(cmd, g_Verts, vbGpu)) return;

    // --- Shadow ---
    if (shadowPass)
    {
        if (g_ShadowState != D3D12_RESOURCE_STATE_DEPTH_WRITE)
        {
            auto bsh = CD3DX12_RESOURCE_BARRIER::Transition(g_ShadowMap.Get(), g_ShadowState,
                                                            D3D12_RESOURCE_STATE_DEPTH_WRITE);
            cmd->ResourceBarrier(1, &bsh);
        }
        g_ShadowState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

        D3D12_CPU_DESCRIPTOR_HANDLE dsh = g_DsvHeap->GetCPUDescriptorHandleForHeapStart();
        dsh.ptr += g_DsvShadowOff;
        cmd->OMSetRenderTargets(0, nullptr, FALSE, &dsh);
        cmd->ClearDepthStencilView(dsh, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
        D3D12_VIEWPORT vp{};
        vp.Width    = static_cast<float>(g_ShadowSize);
        vp.Height   = static_cast<float>(g_ShadowSize);
        vp.MaxDepth = 1.f;
        cmd->RSSetViewports(1, &vp);
        D3D12_RECT sc{ 0, 0, (LONG)g_ShadowSize, (LONG)g_ShadowSize };
        cmd->RSSetScissorRects(1, &sc);

        cmd->SetPipelineState(g_PsoShadow.Get());
        cmd->SetGraphicsRootSignature(g_RootSig.Get());
        cmd->SetGraphicsRootConstantBufferView(0, cbAddr);
        D3D12_VERTEX_BUFFER_VIEW vbv{ vbGpu, UINT(g_Verts.size() * sizeof(Vertex)), sizeof(Vertex) };
        cmd->IASetVertexBuffers(0, 1, &vbv);
        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmd->DrawInstanced(static_cast<UINT>(g_Verts.size()), 1, 0, 0);

        auto bshDone = CD3DX12_RESOURCE_BARRIER::Transition(g_ShadowMap.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                                            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        cmd->ResourceBarrier(1, &bshDone);
        g_ShadowState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }

    // --- main color ---
    auto bcol = CD3DX12_RESOURCE_BARRIER::Transition(g_ColorTex.Get(), g_ColorState,
                                                     D3D12_RESOURCE_STATE_RENDER_TARGET);
    cmd->ResourceBarrier(1, &bcol);
    g_ColorState = D3D12_RESOURCE_STATE_RENDER_TARGET;
    if (g_DepthMainState != D3D12_RESOURCE_STATE_DEPTH_WRITE)
    {
        auto bdm = CD3DX12_RESOURCE_BARRIER::Transition(g_DepthMain.Get(), g_DepthMainState,
                                                        D3D12_RESOURCE_STATE_DEPTH_WRITE);
        cmd->ResourceBarrier(1, &bdm);
    }
    g_DepthMainState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = g_RtvHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = g_DsvHeap->GetCPUDescriptorHandleForHeapStart();
    cmd->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    cmd->ClearRenderTargetView(rtv, g_ClearC, 0, nullptr);
    cmd->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

    D3D12_VIEWPORT mvp{};
    mvp.Width    = static_cast<float>(g_Rw);
    mvp.Height   = static_cast<float>(g_Rh);
    mvp.MaxDepth = 1.f;
    cmd->RSSetViewports(1, &mvp);
    D3D12_RECT scr{ 0, 0, (LONG)g_Rw, (LONG)g_Rh };
    cmd->RSSetScissorRects(1, &scr);

    if (hasGeom)
    {
        ID3D12DescriptorHeap* shSrv = DirectX::GetD3D12SrvHeap();
        D3D12_GPU_DESCRIPTOR_HANDLE tbl = shSrv->GetGPUDescriptorHandleForHeapStart();
        tbl.ptr += UINT64(DirectX::kGpuRendererShadowSrvHeapIndex) * DirectX::GetD3D12SrvDescriptorStride();

        cmd->SetPipelineState(g_PsoMain.Get());
        cmd->SetGraphicsRootSignature(g_RootSig.Get());
        cmd->SetGraphicsRootConstantBufferView(0, cbAddr);
        cmd->SetGraphicsRootDescriptorTable(1, tbl);
        D3D12_VERTEX_BUFFER_VIEW vbv{ vbGpu, UINT(g_Verts.size() * sizeof(Vertex)), sizeof(Vertex) };
        cmd->IASetVertexBuffers(0, 1, &vbv);
        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmd->DrawInstanced(static_cast<UINT>(g_Verts.size()), 1, 0, 0);
    }

    auto bcolPs = CD3DX12_RESOURCE_BARRIER::Transition(g_ColorTex.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                      D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmd->ResourceBarrier(1, &bcolPs);
    g_ColorState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    auto bdmComm = CD3DX12_RESOURCE_BARRIER::Transition(g_DepthMain.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                                        D3D12_RESOURCE_STATE_COMMON);
    cmd->ResourceBarrier(1, &bdmComm);
    g_DepthMainState = D3D12_RESOURCE_STATE_COMMON;

    if (shadowPass && g_ShadowMap)
    {
        auto bshRw = CD3DX12_RESOURCE_BARRIER::Transition(g_ShadowMap.Get(),
                                                           D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                                           D3D12_RESOURCE_STATE_DEPTH_WRITE);
        cmd->ResourceBarrier(1, &bshRw);
        g_ShadowState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }

    cmd->Close();
    ID3D12CommandList* const cls[] = { cmd };
    queue->ExecuteCommandLists(1, cls);
    GpuWait(queue);
}

} // namespace GpuRenderer
