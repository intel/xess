/*******************************************************************************
 * Copyright 2021 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files(the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions :
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#pragma once

#include "EngineTuning.h"
#include "XeSSRuntime.h"

class ColorBuffer;
class CommandContext;

namespace XeSS
{
    class XeSSRuntime;
}

namespace Math
{
    class Camera;
}

namespace XeSS
{
    /// Motion vectors mode
    enum eMotionVectorsMode
    {
        kMotionVectorsHighRes = 0,
        kMotionVectorsLowRes,
        kMotionVectorsModeNum
    };

    /// Mip-bias mode
    enum eMipBiasMode
    {
        kMipBiasAutomatic = 0,
        kMipBiasCustomized,
        kMipBiasNum
    };

    extern NumVar Sharpness;

    /// XeSS runtime wrapper object
    extern XeSSRuntime g_XeSSRuntime;

    extern ColorBuffer g_SharpenColorBuffer;

    /// Velocity buffer used in low-res motion vectors mode, converted from MiniEngine's packed velocity.
    extern ColorBuffer g_ConvertedVelocityBuffer; // DXGI_FORMAT_R16G16_FLOAT

    /// Return if XeSS processing is enabled.
    bool IsEnabled();
    /// Set if XeSS processing enabled.
    void SetEnabled(bool Enabled);

    /// Initialization interface.
    void Initialize();
    /// Destruction interface.
    void Shutdown();
    /// Tick the processing.
    void Update();
    /// Process the rendering content. XeSS execution happens inside this interface.
    void Process(CommandContext& Context, const Math::Camera& camera);

    /// Set output resolution.
    void SetOutputResolution(uint32_t Width, uint32_t Height);
    /// Update XeSS runtime.
    void UpdateRuntime();

    /// Get input resolution.
    void GetInputResolution(uint32_t& Width, uint32_t& Height);
    /// Update input resolution.
    void UpdateInputResolution();
    /// Get motion vectors mode.
    eMotionVectorsMode GetMotionVectorsMode();
    /// Set motion vectors mode.
    void SetMotionVectorsMode(eMotionVectorsMode Mode);
    /// Return if motion vectors are jittered.
    bool IsMotionVectorsJittered();
    /// Set motion jittered.
    void SetMotionVectorsJittered(bool Jittered);
    /// Return if motion vectors are in Normalized Device Coordinate.
    bool IsMotionVectorsInNDC();
    /// Set motion vectors in Normalized Device Coordinate.
    void SetMotionVectorsInNDC(bool NDC);
    /// Get if responsive mask enabled.
    bool IsResponsiveMaskEnabled();
    /// Set responsive mask enabled.
    void SetResponsiveMaskEnabled(bool Enabled);
    /// Get if auto exposure enabled.
    bool IsAutoExposureEnabled();
    /// Set auto exposure enabled.
    void SetAutoExposureEnabled(bool Enabled);
    /// Get quality of XeSS.
    eQualityLevel GetQuality();
    /// Set quality of XeSS.
    void SetQuality(eQualityLevel Quality);
    /// Get mip-bias mode.
    eMipBiasMode GetMipBiasMode();
    /// Set mip-bias mode.
    void SetMipBiasMode(eMipBiasMode Mode);
    /// Get mip-bias value.
    float GetMipBias();
    /// Set mip-bias Value.
    void SetMipBias(float Value);
    /// Returns (frame-index % 2).
    uint32_t GetFrameIndexMod2();
    /// Is XeSS supported.
    bool IsSupported();
    /// Reset XeSS history once for debug purposes.
    void ResetHistory();
    /// Returns if GPU profiling is enabled.
    bool IsProfilingEnabled();
    /// Set GPU profiling enabled.
    void SetProfilingEnabled(bool Enabled);

    /// Return if dynamic resolution is enabled.
    bool IsDynResEnabled();
    /// Set if dynamic resolution is enabled.
    void SetDynResEnabled(bool Enabled);

    float GetUpscaleFactor();
    void SetUpscaleFactor(float Upscale);

} // namespace XeSS
