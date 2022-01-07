//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

#include "DXSampleHelper.h"
#include "Win32Application.h"

class DXSample
{
public:
    DXSample(){}
    DXSample(UINT width, UINT height, std::wstring name);
    virtual ~DXSample();

    virtual void OnInit() = 0;
    virtual void OnUpdate(float DeltaTime) = 0;
    virtual void OnRender(float DeltaTime) = 0;
    virtual void OnDestroy() = 0;
    virtual void OnGUI(float DeltaTime) = 0;
    virtual void OnPostGUI(float DeltaTime) = 0;

    // Samples override the event handlers to handle specific messages.
    virtual void OnKeyDown(UINT8 /*key*/)   {}
    virtual void OnKeyUp(UINT8 /*key*/)     {}

    // Accessors.
    UINT GetWidth() const           { return m_width; }
    UINT GetHeight() const          { return m_height; }
    const WCHAR* GetTitle() const   { return m_title.c_str(); }

    void ParseCommandLineArgs(_In_reads_(argc) WCHAR* argv[], int argc);

    inline ID3D12Device* GetDirectXDevice() { return mDevice.Get(); }
    inline ID3D12DescriptorHeap* GetDescriptorHeap() { return mRtvHeap.Get(); }
    inline ID3D12GraphicsCommandList* GetCommandList() { return mCommandList.Get(); }

protected:
    ComPtr<ID3D12Device> mDevice;
    ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;

protected:
    std::wstring GetAssetFullPath(LPCWSTR assetName);

    void GetHardwareAdapter(
        _In_ IDXGIFactory1* pFactory,
        _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter,
        bool requestHighPerformanceAdapter = false);

    void SetCustomWindowText(LPCWSTR text);

    // Viewport dimensions.
    UINT m_width;
    UINT m_height;
    float m_aspectRatio;

    // Adapter info.
    bool m_useWarpDevice;

private:
    // Root assets path.
    std::wstring m_assetsPath;

    // Window title.
    std::wstring m_title;
};
