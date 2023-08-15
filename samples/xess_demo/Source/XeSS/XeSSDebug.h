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

class ColorBuffer;
class GraphicsContext;
class ComputeContext;

namespace Math
{
    class Camera;
}

namespace XeSSDebug
{
    /// If we should bypass XeSS. Used for debugging.
    extern bool BypassXeSS;

    /// Intermediate buffer used for XeSS output debug.
    extern ColorBuffer g_DebugBufferOutput;

    /// Initialization interface.
    void Initialize();
    /// Destruction interface.
    void Shutdown();
    /// Update interface, called very frame.
    void Update();

    /// Get if output buffer debugging is enabled.
    bool IsBufferDebugEnabled();
    /// Set if output buffer debugging is enabled.
    void SetBufferDebugEnabled(bool Enabled);

    /// Recreate buffers for debugging.
    void RecreateDebugBuffers();
    /// Destroy buffers for debugging.
    void DestroyDebugBuffers();

    /// Returns network model index.
    int32_t GetNetworkModel();
    /// Select network model of XeSS.
    bool SelectNetworkModel(int32_t model);

    /// Upscale a RGB color buffer.
    void UpscaleRGB(GraphicsContext& Context, ColorBuffer& source, ColorBuffer& destination);
    /// Upscale a RGBA color buffer.
    void UpscaleRGBA(GraphicsContext& Context, ColorBuffer& source, ColorBuffer& destination);

    /// Begin frame dumping.
    bool BeginFrameDump(bool DynamicCamera = true);
    /// Is frame dumping is in process.
    bool IsFrameDumpOn();
    /// Get index of the frame being dumped.
    uint32_t GetDumpFrameIndex();
    /// Return true if camera is moving when dumping, false when static camera is used.
    bool IsDumpDynamic();

    /// Update camera yaw. (Used in dynamic camera mode.)
    void UpdateCameraYaw(float& yaw);

} // namespace XeSSDebug
