//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

// Modified 2022, Intel Corporation.
// Added support for super sampling.

#pragma once

#include <cstdint>

namespace Display
{
    extern BoolVar s_EnableVSync;

    extern bool g_SupportTearing;

    void Initialize(void);
    void Shutdown(void);
    void Resize(uint32_t width, uint32_t height);
    void Present(void);

    bool IsFullscreen();
    void SetFullscreen(bool fullscreen);

    void UpdateDPIScale();

    float GetDPIScale();
}

namespace Graphics
{
    extern uint32_t g_DisplayWidth;
    extern uint32_t g_DisplayHeight;
    extern uint32_t g_NativeWidth;
    extern uint32_t g_NativeHeight;
    extern bool g_bEnableHDROutput;

    extern bool g_bArbitraryResolution;

    enum eResolution { k720p, k900p, k1080p, k1440p, k1800p, k2160p };

    void ResolutionToUINT(eResolution res, uint32_t& width, uint32_t& height);

    // Returns the number of elapsed frames since application start
    uint64_t GetFrameCount(void);

    // The amount of time elapsed during the last completed frame.  The CPU and/or
    // GPU may be idle during parts of the frame.  The frame time measures the time
    // between calls to present each frame.
    float GetFrameTime(void);

    // The total number of frames per second
    float GetFrameRate(void);

    extern bool g_bEnableHDROutput;

    // Returns if upscaling is enabled.
    bool IsUpscalingEnabled();

    // Set native resolution and if upscaling in the pipeline. 
    void SetNativeResolutionAndUpscaling(uint32_t Width, uint32_t Height, bool UpscalingEnabled);
}
