//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// Copyright (c) 2023 Intel Corporation
// 
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
    DXSample(UINT width, UINT height, std::wstring name);
    virtual ~DXSample();

    virtual void OnInit() = 0;
    virtual void OnUpdate() = 0;
    virtual void OnRender() = 0;
    virtual void OnDestroy() = 0;
    virtual void SetViewPort(UINT width, UINT height) = 0;

    // Samples override the event handlers to handle specific messages.
    virtual void OnKeyDown(UINT8 /*key*/)   {}
    virtual void OnKeyUp(UINT8 /*key*/)     {}
    virtual void OnMouseWheel(WORD wValue) {}
    virtual void OnSleep() = 0;

    // Accessors.
    UINT GetWidth() const           { return m_width; }
    UINT GetHeight() const          { return m_height; }
    const WCHAR* GetTitle() const   { return m_title.c_str(); }
    void SetFrameTime(float duration) { m_last_frameMS = duration; };

    void ParseCommandLineArgs(_In_reads_(argc) WCHAR* argv[], int argc);

    // Flip mode
    bool m_useAsyncFlip = false;

    // Target FPS
    float m_frameMS = 0; // as fast as possible

    // Use Full Screen
    bool m_fullScreen = false;
    bool m_switchFullScreen = false;
    bool m_switchResolution = false;
    bool m_switchEnablement = false;

    bool m_showOnlyInterpolation = false;
    bool m_tagInterpolatedFrames = true;
    bool m_maximized = false;
    bool m_borderless = false;
    bool m_topmost = false;
    float m_last_frameMS = 0.0;

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
    bool m_useDebugDevice;
    INT m_hardwareAdapterId;

private:
    // Root assets path.
    std::wstring m_assetsPath;

    // Window title.
    std::wstring m_title;
};
