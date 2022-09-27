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

/// Debugging output of jitter when enabled.
#define _XESS_DEBUG_JITTER_ 0

namespace Math
{
    class Camera;
}

namespace XeSSJitter
{
    /// Initialization interface for jitter.
    void Initialize();
    /// Reset jitter sequence.
    void Reset();
    /// Move index of jitter sequence forward.
    void FrameMove();
    /// Get jitter values. Sequence index does not change after this call.
    void GetJitterValues(float& JitterX, float& JitterY);
    /// Apply current jitter values to the camera.
    void ApplyCameraJitter(Math::Camera& Camera_, float JitterX, float JitterY);
    /// Clear jitter values from the camera.
    void ClearCameraJitter(Math::Camera& Camera_);
} // namespace XeSSJitter
