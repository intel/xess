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

#include "pch.h"
#include "XeSSJitter.h"
#include "Camera.h"
#include "BufferManager.h"

using namespace Math;
using namespace Graphics;

namespace XeSSJitter
{
    std::vector<std::pair<float, float>> g_HaltonSamples;
    std::vector<std::pair<float, float>>& haltonSamples = g_HaltonSamples;

    size_t s_JitterIndex = 0;

    float GetCorput(uint32_t index, uint32_t base)
    {
        float result = 0;
        float bk = 1.f;

        while (index > 0)
        {
            bk /= (float)base;
            result += (float)(index % base) * bk;
            index /= base;
        }

        return result;
    }

    void GenerateHalton(std::vector<std::pair<float, float>>& HaltonSamples,
        std::uint32_t base1, std::uint32_t base2,
        std::uint32_t start_index, std::uint32_t count, float offset1, float offset2)
    {
        HaltonSamples.reserve(count);

        for (std::uint32_t a = start_index; a < count + start_index; ++a)
        {
            HaltonSamples.emplace_back(GetCorput(a, base1) + offset1, GetCorput(a, base2) + offset2);
        }
    }

    void Initialize()
    {
        s_JitterIndex = 0;
        g_HaltonSamples.clear();

        GenerateHalton(g_HaltonSamples, 2, 3, 1, 32, -0.5f, -0.5f);

        haltonSamples = g_HaltonSamples;
    }

    void Reset()
    {
        s_JitterIndex = 0;

#if _XESS_DEBUG_JITTER_
        LOG_DEBUG("XeSS Jitter: Reset.");
#endif
    }

    void FrameMove()
    {
        s_JitterIndex = (s_JitterIndex + 1) % haltonSamples.size();

#if _XESS_DEBUG_JITTER_
        LOG_DEBUG("XeSS Jitter: Frame Move.");
#endif
    }

    void GetJitterValues(float& JitterX, float& JitterY)
    {
        ASSERT(!haltonSamples.empty());
        auto haltonValue = haltonSamples[s_JitterIndex];
        JitterX = haltonValue.first;
        JitterY = haltonValue.second;
    }

    void ApplyCameraJitter(Camera& Camera_, float JitterX, float JitterY)
    {
        uint32_t nativeWidth = g_SceneColorBuffer.GetWidth();
        uint32_t nativeHeight = g_SceneColorBuffer.GetHeight();
        if (nativeWidth && nativeHeight) // Make sure g_SceneColorBuffer is ready.
        {
            // MiniEngine uses right-handed coordinate system.
            float projJitterX = -(JitterX * 2.0f) / nativeWidth;
            float projJitterY = (JitterY * 2.0f) / nativeHeight;

            Camera_.SetProjectMatrixJitter(projJitterX, projJitterY);

#if _XESS_DEBUG_JITTER_
            LOG_DEBUGF("XeSS Jitter: Camera Projection Jitter Applied = %f, %f.", JitterX, JitterY);
#endif
        }
    }

    void ClearCameraJitter(Camera& Camera_)
    {
        Camera_.SetProjectMatrixJitter(0.0f, 0.0f);

#if _XESS_DEBUG_JITTER_
        LOG_DEBUG("XeSS Jitter: Camera Projection Jitter Removed.");
#endif
    }
} // namespace XeSSJitter
