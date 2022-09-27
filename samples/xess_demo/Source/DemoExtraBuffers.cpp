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
#include "DemoExtraBuffers.h"
#include "ColorBuffer.h"
#include "XeSS/XeSSProcess.h"
#include "XeSS/XeSSDebug.h"

namespace Graphics
{
    extern DXGI_FORMAT DefaultHdrColorFormat;
}

DXGI_FORMAT VelocityBufferFormat = DXGI_FORMAT_R16G16_FLOAT;

void DemoExtraBuffersHandler::InitializeBuffers(uint32_t NativeWidth, uint32_t NativeHeight, uint32_t UpscaledWidth, uint32_t UpscaledHeight)
{
    (UpscaledWidth);
    (UpscaledHeight);
    XeSS::g_ConvertedVelocityBuffer.Create(L"Motion Vectors - Converted", NativeWidth, NativeHeight, 1, VelocityBufferFormat);
    XeSS::g_SharpenColorBuffer.Create(L"Sharpening Buffer", UpscaledWidth, UpscaledHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);

    if (XeSSDebug::IsBufferDebugEnabled())
    {
        XeSSDebug::RecreateDebugBuffers();
    }
}

void DemoExtraBuffersHandler::DestroyRenderingBuffers()
{
    if (XeSSDebug::IsBufferDebugEnabled())
    {
        XeSSDebug::DestroyDebugBuffers();
    }

    XeSS::g_SharpenColorBuffer.Destroy();
    XeSS::g_ConvertedVelocityBuffer.Destroy();
}
