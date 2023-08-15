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
#include "XeSSProcess.h"
#include "XeSSRuntime.h"
#include "XeSSJitter.h"
#include "XeSSDebug.h"
#include "BufferManager.h"
#include "Camera.h"
#include "CommandContext.h"
#include "Display.h"
#include "TemporalEffects.h"
#include "ImageScaling.h"
#include "DemoExtraBuffers.h"
#include "VectorMath.h"
#include "Log.h"
#include "CompiledShaders/XeSSConvertLowResVelocityCS.h"
#include "CompiledShaders/XeSSGenerateHiResVelocityCS.h"
#include "CompiledShaders/XeSSConvertLowResVelocityNDCCS.h"
#include "CompiledShaders/XeSSGenerateHiResVelocityNDCCS.h"
#include "CompiledShaders/SharpenCS.h"

#include <iostream>

using namespace Graphics;
using namespace Math;

namespace XeSS
{
    bool s_Enabled = true;

    bool s_IsSupported = true;

    XeSSRuntime g_XeSSRuntime;

    ColorBuffer g_ConvertedVelocityBuffer;
    ColorBuffer g_SharpenColorBuffer;

    NumVar Sharpness("XeSS/Sharpness", 0.0f, 0.0f, 1.0f, 0.02f);

    eMotionVectorsMode s_MotionVectorsMode = kMotionVectorsHighRes;
    bool s_MotionVectorsJittered = false;
    bool s_MotionVectorsInNDC = false;
    eQualityLevel s_Quality = kQualityQuality;
    bool s_ResponsiveMaskEnabled = false;
    bool s_AutoExposureEnabled = false;
    eMipBiasMode s_MipBiasMode = kMipBiasAutomatic;
    float s_CustomizedMipBias = FLT_MAX;
    bool s_ResetHistory = false;
    bool s_DynResEnabled = false;
    float s_UpscaleFactor = 2.0f;

    uint32_t s_InputWidth = 0;
    uint32_t s_InputHeight = 0;
    uint32_t s_OutputWidth = 0;
    uint32_t s_OutputHeight = 0;

    bool s_RuntimeProfilingEnabled = false;

    bool s_RuntimeDirty = true;
    bool s_InputResolutionDirty = true;

    ComputePSO s_XeSSConvertLowResVelocityCS(L"ConvertLowResVelocityCS");
    ComputePSO s_XeSSGenerateHiResVelocityCS(L"GenerateHiResVelocityCS");
    ComputePSO s_XeSSConvertLowResVelocityNDCCS(L"ConvertLowResVelocityNDCCS");
    ComputePSO s_XeSSGenerateHiResVelocityNDCCS(L"GenerateHiResVelocityNDCCS");
    ComputePSO s_SharpenImageCS(L"Sharpen Image CS");

    void ConvertLowResVelocity(ComputeContext& Context);
    void GenerateHighResVelocity(ComputeContext& Context, const Matrix4& CurToPrevXForm);
    void ExecuteXeSS(ComputeContext& Context, ColorBuffer& OutputColorBuffer);
    void GetRuntimeInputResolution(uint32_t& Width, uint32_t& Height);

    float GetDefaultMipBiasValue();

    void SharpenImage(ComputeContext& Context, ColorBuffer& SourceColorBuffer, ColorBuffer& DestColorBuffer);
} // namespace XeSS

bool XeSS::IsEnabled()
{
    return s_Enabled;
}

void XeSS::SetEnabled(bool Enabled)
{
    if (s_Enabled == Enabled)
        return;

    s_Enabled = Enabled;

    if (Enabled)
    {
        s_ResetHistory = true;
    }
}

void XeSS::Initialize()
{
    XeSSDebug::Initialize();

    if (!XeSSDebug::BypassXeSS)
    {
        s_IsSupported = g_XeSSRuntime.CreateContext();
        if (!s_IsSupported)
        {
            LOG_ERROR("XeSS: Could not create context. XeSS function will not be available.");
        }
        else
        {
            LOG_INFO("XeSS: Context created.");
        }
    }

    XeSSJitter::Initialize();

#define CreatePSO(ObjName, ShaderByteCode) \
    ObjName.SetRootSignature(g_CommonRS); \
    ObjName.SetComputeShader(ShaderByteCode, sizeof(ShaderByteCode)); \
    ObjName.Finalize();

    CreatePSO(s_XeSSConvertLowResVelocityCS, g_pXeSSConvertLowResVelocityCS);
    CreatePSO(s_XeSSGenerateHiResVelocityCS, g_pXeSSGenerateHiResVelocityCS);
    CreatePSO(s_XeSSConvertLowResVelocityNDCCS, g_pXeSSConvertLowResVelocityNDCCS);
    CreatePSO(s_XeSSGenerateHiResVelocityNDCCS, g_pXeSSGenerateHiResVelocityNDCCS);
    CreatePSO(s_SharpenImageCS, g_pSharpenCS);
#undef CreatePSO

    SetOutputResolution(g_DisplayWidth, g_DisplayHeight);
    XeSSDebug::SelectNetworkModel(0);
}

void XeSS::Shutdown()
{
    if (g_XeSSRuntime.IsInitialzed())
    {
        g_XeSSRuntime.Shutdown();

        LOG_INFO("XeSS: Finalized.");
    }

    XeSSDebug::Shutdown();
}

void XeSS::GetInputResolution(uint32_t& Width, uint32_t& Height)
{
    if (s_DynResEnabled)
    {
        Width = ceil(s_OutputWidth / s_UpscaleFactor);
        Height = ceil(s_OutputHeight / s_UpscaleFactor);
    }
    else
    {
        Width = s_OutputWidth;
        Height = s_OutputHeight;

        GetRuntimeInputResolution(Width, Height);
    }
}

void XeSS::SetOutputResolution(uint32_t Width, uint32_t Height)
{
    if (s_OutputWidth == Width && s_OutputHeight == Height)
        return;

    s_OutputWidth = Width;
    s_OutputHeight = Height;

    s_InputResolutionDirty = true;
    s_RuntimeDirty = true;
}

XeSS::eMotionVectorsMode XeSS::GetMotionVectorsMode()
{
    return s_MotionVectorsMode;
}

void XeSS::SetMotionVectorsMode(eMotionVectorsMode Mode)
{
    if (s_MotionVectorsMode == Mode)
        return;

    s_MotionVectorsMode = Mode;

    s_RuntimeDirty = true;
}

bool XeSS::IsMotionVectorsJittered()
{
    return s_MotionVectorsJittered;
}

void XeSS::SetMotionVectorsJittered(bool Jittered)
{
    if (s_MotionVectorsJittered == Jittered)
        return;

    s_MotionVectorsJittered = Jittered;

    s_RuntimeDirty = true;
}

bool XeSS::IsMotionVectorsInNDC()
{
    return s_MotionVectorsInNDC;
}

void XeSS::SetMotionVectorsInNDC(bool NDC)
{
    if (s_MotionVectorsInNDC == NDC)
        return;

    s_MotionVectorsInNDC = NDC;

    s_RuntimeDirty = true;
}

bool XeSS::IsResponsiveMaskEnabled()
{
    return s_ResponsiveMaskEnabled;
}

void XeSS::SetResponsiveMaskEnabled(bool Enabled)
{
    if (s_ResponsiveMaskEnabled == Enabled)
        return;

    s_ResponsiveMaskEnabled = Enabled;

    s_RuntimeDirty = true;
}

bool XeSS::IsAutoExposureEnabled()
{
    return s_AutoExposureEnabled;
}

void XeSS::SetAutoExposureEnabled(bool Enabled)
{
    if (s_AutoExposureEnabled == Enabled)
        return;

    s_AutoExposureEnabled = Enabled;

    s_RuntimeDirty = true;
}

void XeSS::UpdateInputResolution()
{
    uint32_t width, height;
    GetInputResolution(width, height);

    Graphics::SetNativeResolutionAndUpscaling(width, height, true);

    s_InputWidth = width;
    s_InputHeight = height;

    s_InputResolutionDirty = false;
}

void XeSS::SetDynResEnabled(bool Enabled)
{
    if (s_DynResEnabled == Enabled)
        return;

    s_DynResEnabled = Enabled;
    s_InputResolutionDirty = true;
}

bool XeSS::IsDynResEnabled()
{
    return s_DynResEnabled;
}

float XeSS::GetUpscaleFactor()
{
    return s_UpscaleFactor;
}

void XeSS::SetUpscaleFactor(float Upscale)
{
    if (s_UpscaleFactor == Upscale)
        return;

    s_UpscaleFactor = Upscale;
    s_InputResolutionDirty = true;
}

XeSS::eQualityLevel XeSS::GetQuality()
{
    return s_Quality;
}

void XeSS::SetQuality(eQualityLevel Quality)
{
    if (s_Quality == Quality)
        return;

    s_Quality = Quality;

    s_InputResolutionDirty = true;

    s_RuntimeDirty = true;
}

XeSS::eMipBiasMode XeSS::GetMipBiasMode()
{
    return s_MipBiasMode;
}

void XeSS::SetMipBiasMode(eMipBiasMode Mode)
{
    s_MipBiasMode = Mode;
}

float XeSS::GetDefaultMipBiasValue()
{
    return log2f(g_NativeWidth / static_cast<float>(g_DisplayWidth));
}

float XeSS::GetMipBias()
{
    if (s_MipBiasMode == kMipBiasAutomatic)
        return GetDefaultMipBiasValue();
    else
    {
        if (s_CustomizedMipBias > 15.99f)
        {
            s_CustomizedMipBias = GetDefaultMipBiasValue();
        }

        return s_CustomizedMipBias;
    }
}

void XeSS::SetMipBias(float Value)
{
    s_MipBiasMode = kMipBiasCustomized;
    s_CustomizedMipBias = Value;
}

void XeSS::UpdateRuntime()
{
    bool isHiResMode = (s_MotionVectorsMode == kMotionVectorsHighRes);

    InitArguments initArgs = {};
    initArgs.OutputWidth = s_OutputWidth;
    initArgs.OutputHeight = s_OutputHeight;
    initArgs.Quality = s_Quality;
    initArgs.UseHiResMotionVectors = isHiResMode;
    initArgs.UseJitteredMotionVectors = s_MotionVectorsJittered;
    initArgs.UseMotionVectorsInNDC = s_MotionVectorsInNDC;
    initArgs.UseResponsiveMask = s_ResponsiveMaskEnabled;
    initArgs.UseAutoExposure = s_AutoExposureEnabled;
    initArgs.EnableProfiling = s_RuntimeProfilingEnabled;

    g_XeSSRuntime.Initialize(initArgs);

    s_RuntimeDirty = false;

    s_ResetHistory = true;
}

void XeSS::Update()
{
    if (!s_Enabled)
        return;

    if (s_RuntimeDirty)
    {
        UpdateRuntime();
    }

    if (s_InputResolutionDirty)
    {
        UpdateInputResolution();
    }

    XeSSDebug::Update();

    if (s_ResetHistory)
    {
        XeSSJitter::Reset();
    }
    else
    {
        XeSSJitter::FrameMove();
    }
}

uint32_t XeSS::GetFrameIndexMod2()
{
    return TemporalEffects::GetFrameIndexMod2();
}
bool XeSS::IsSupported()
{
    return s_IsSupported;
}

void XeSS::ResetHistory()
{
    s_ResetHistory = true;
}

bool XeSS::IsProfilingEnabled()
{
    return s_RuntimeProfilingEnabled;
}

void XeSS::SetProfilingEnabled(bool Enabled)
{
    if (s_RuntimeProfilingEnabled == Enabled)
        return;

    s_RuntimeProfilingEnabled = Enabled;
    s_RuntimeDirty = true;
}

void XeSS::Process(CommandContext& BaseContext, const Camera& camera)
{
    if (!s_Enabled || !g_XeSSRuntime.IsInitialzed())
        return;

    ScopedTimer _prof(L"XeSS Process", BaseContext);

    ComputeContext& Context = BaseContext.GetComputeContext();

    uint32_t Width = g_UpscaledSceneColorBuffer.GetWidth();
    uint32_t Height = g_UpscaledSceneColorBuffer.GetHeight();

    float RcpHalfDimX = 2.0f / Width;
    float RcpHalfDimY = 2.0f / Height;
    float RcpZMagic = camera.GetNearClip() / (camera.GetFarClip() - camera.GetNearClip());

    Matrix4 preMult = Matrix4(
        Vector4(RcpHalfDimX, 0.0f, 0.0f, 0.0f),
        Vector4(0.0f, -RcpHalfDimY, 0.0f, 0.0f),
        Vector4(0.0f, 0.0f, RcpZMagic, 0.0f),
        Vector4(-1.0f, 1.0f, -RcpZMagic, 1.0f));

    Matrix4 postMult = Matrix4(
        Vector4(1.0f / RcpHalfDimX, 0.0f, 0.0f, 0.0f),
        Vector4(0.0f, -1.0f / RcpHalfDimY, 0.0f, 0.0f),
        Vector4(0.0f, 0.0f, 1.0f, 0.0f),
        Vector4(1.0f / RcpHalfDimX, 1.0f / RcpHalfDimY, 0.0f, 1.0f));

    Matrix4 CurToPrevXForm = postMult * camera.GetReprojectionMatrix() * preMult;

    if (s_MotionVectorsMode == XeSS::kMotionVectorsHighRes)
    {
        GenerateHighResVelocity(Context, CurToPrevXForm);
    }
    else
    {
        ConvertLowResVelocity(Context);
    }

    if (!XeSSDebug::BypassXeSS)
    {
        bool sharpenEnabled = Sharpness > 0.001f;

        ExecuteXeSS(Context, sharpenEnabled ? g_SharpenColorBuffer : g_UpscaledSceneColorBuffer);

        if (XeSSDebug::IsBufferDebugEnabled())
        {
            // If XeSS output debug is enabled, before drawing anything further we store the output to the debug buffer.
            XeSSDebug::UpscaleRGB(BaseContext.GetGraphicsContext(), sharpenEnabled ? g_SharpenColorBuffer : g_UpscaledSceneColorBuffer, XeSSDebug::g_DebugBufferOutput);
        }

        if (sharpenEnabled)
        {
            SharpenImage(Context, g_SharpenColorBuffer, g_UpscaledSceneColorBuffer);
        }
    }
    else
    {
        XeSSDebug::UpscaleRGB(BaseContext.GetGraphicsContext(), g_SceneColorBuffer, g_UpscaledSceneColorBuffer);
    }

    s_ResetHistory = false;
}

void XeSS::GetRuntimeInputResolution(uint32_t& Width, uint32_t& Height)
{
    if (!g_XeSSRuntime.IsInitialzed())
        return;

    if (!g_XeSSRuntime.GetInputResolution(Width, Height))
    {
        Width = 512;
        Height = 512;
    }
}

void XeSS::ConvertLowResVelocity(ComputeContext& Context)
{
    ASSERT(s_MotionVectorsMode == kMotionVectorsLowRes);
    if (s_MotionVectorsMode != kMotionVectorsLowRes)
        return;

    ScopedTimer _prof(L"Convert Velocity", Context);

    Context.SetRootSignature(g_CommonRS);
    Context.SetPipelineState(s_MotionVectorsInNDC ? s_XeSSConvertLowResVelocityNDCCS : s_XeSSConvertLowResVelocityCS);

    __declspec(align(16)) struct ConstantBuffer
    {
        float InBufferDim[2];
        float RcpInBufferDim[2];
    };

    ConstantBuffer cbv = {
        (float)g_VelocityBuffer.GetWidth(), (float)g_VelocityBuffer.GetHeight(),
        1.0f / g_VelocityBuffer.GetWidth(), 1.0f / g_VelocityBuffer.GetHeight()
    };

    Context.SetDynamicConstantBufferView(3, sizeof(cbv), &cbv);

    Context.TransitionResource(g_VelocityBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    Context.TransitionResource(g_ConvertedVelocityBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    Context.SetDynamicDescriptor(1, 0, g_VelocityBuffer.GetSRV());
    Context.SetDynamicDescriptor(2, 0, g_ConvertedVelocityBuffer.GetUAV());

    Context.Dispatch2D(g_ConvertedVelocityBuffer.GetWidth(), g_ConvertedVelocityBuffer.GetHeight());
}

void XeSS::GenerateHighResVelocity(ComputeContext& Context, const Matrix4& CurToPrevXForm)
{
    ASSERT(s_MotionVectorsMode == kMotionVectorsHighRes);
    if (s_MotionVectorsMode != kMotionVectorsHighRes)
        return;

    ScopedTimer _prof(L"Upscale Velocity", Context);

    Context.SetRootSignature(g_CommonRS);
    Context.SetPipelineState(s_MotionVectorsInNDC ? s_XeSSGenerateHiResVelocityNDCCS : s_XeSSGenerateHiResVelocityCS);

    __declspec(align(16)) struct ConstantBuffer
    {
        float InBufferDim[2];
        float OutBufferDim[2];
        float RcpInBufferDim[2];
        float RcpOutBufferDim[2];
        float ViewportJitter[2];
        Matrix4 CurToPrevXForm;
    };

    ColorBuffer& LinearDepth = g_LinearDepth[XeSS::GetFrameIndexMod2()];

    float jitterX, jitterY;
    XeSSJitter::GetJitterValues(jitterX, jitterY);

    ConstantBuffer cbv = {
        (float)g_VelocityBuffer.GetWidth(), (float)g_VelocityBuffer.GetHeight(),
        (float)g_UpscaledVelocityBuffer.GetWidth(), (float)g_UpscaledVelocityBuffer.GetHeight(),
        1.0f / g_VelocityBuffer.GetWidth(), 1.0f / g_VelocityBuffer.GetHeight(),
        1.0f / g_UpscaledVelocityBuffer.GetWidth(), 1.0f / g_UpscaledVelocityBuffer.GetHeight(),
        jitterX, jitterY,
        CurToPrevXForm
    };

    Context.SetDynamicConstantBufferView(3, sizeof(cbv), &cbv);

    Context.TransitionResource(g_VelocityBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    Context.TransitionResource(LinearDepth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    Context.TransitionResource(g_UpscaledVelocityBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    Context.SetDynamicDescriptor(1, 0, g_VelocityBuffer.GetSRV());
    Context.SetDynamicDescriptor(1, 1, LinearDepth.GetSRV());
    Context.SetDynamicDescriptor(2, 0, g_UpscaledVelocityBuffer.GetUAV());

    Context.Dispatch2D(g_UpscaledVelocityBuffer.GetWidth(), g_UpscaledVelocityBuffer.GetHeight());
}

void XeSS::ExecuteXeSS(ComputeContext& Context, ColorBuffer& OutputColorBuffer)
{
    if (!g_XeSSRuntime.IsInitialzed())
        return;

    ScopedTimer _prof(L"Execute XeSS", Context);

    bool isHiResMode = (s_MotionVectorsMode == kMotionVectorsHighRes);

    ColorBuffer& linearDepth = g_LinearDepth[XeSS::GetFrameIndexMod2()];

    Context.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    Context.TransitionResource(OutputColorBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    if (isHiResMode)
    {
        Context.TransitionResource(g_UpscaledVelocityBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }
    else
    {
        Context.TransitionResource(g_ConvertedVelocityBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        Context.TransitionResource(linearDepth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }

    if (s_ResponsiveMaskEnabled)
    {
        Context.TransitionResource(g_ResponsiveMaskBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }

    Context.FlushResourceBarriers();

    ExecuteArguments exeArgs = {};
    exeArgs.CommandList = Context.GetComputeContext().GetCommandList();
    exeArgs.InputWidth = g_SceneColorBuffer.GetWidth();
    exeArgs.InputHeight = g_SceneColorBuffer.GetHeight();
    exeArgs.ResetHistory = s_ResetHistory;

    exeArgs.ColorTexture = &g_SceneColorBuffer;
    exeArgs.VelocityTexture = isHiResMode ? &g_UpscaledVelocityBuffer : &g_ConvertedVelocityBuffer;
    exeArgs.OutputTexture = &OutputColorBuffer;
    exeArgs.DepthTexture = isHiResMode ? nullptr : &linearDepth; // Only low-res mode needs depth input.
    exeArgs.ResponsiveMask = s_ResponsiveMaskEnabled ? &g_ResponsiveMaskBuffer : nullptr;

    g_XeSSRuntime.Execute(exeArgs);

    Context.GetComputeContext().InvalidateStates();
}

void XeSS::SharpenImage(ComputeContext& Context, ColorBuffer& SourceColorBuffer, ColorBuffer& DestColorBuffer)
{
    ScopedTimer _prof(L"Sharpen Image", Context);

    Context.TransitionResource(SourceColorBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    Context.TransitionResource(DestColorBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    Context.SetRootSignature(g_CommonRS);
    Context.SetPipelineState(s_SharpenImageCS);
    Context.SetConstants(0, 1.0f + Sharpness, 0.25f * Sharpness);
    Context.SetDynamicDescriptor(1, 0, SourceColorBuffer.GetSRV());
    Context.SetDynamicDescriptor(2, 0, DestColorBuffer.GetUAV());
    Context.Dispatch2D(DestColorBuffer.GetWidth(), DestColorBuffer.GetHeight());
}
