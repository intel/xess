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
#include "XeSSDebug.h"
#include "XeSSProcess.h"
#include "XeSSRuntime.h"
#include "DemoExtraBuffers.h"
#include "XeSSJitter.h"
#include "GraphicsCommon.h"
#include "RootSignature.h"
#include "PipelineState.h"
#include "CompiledShaders/ScreenQuadPresentVS.h"
#include "CompiledShaders/XeSSDebugSimpleUpsamplePS.h"
#include "CompiledShaders/XeSSDebugSimpleUpsampleRGBAPS.h"
#include "ColorBuffer.h"
#include "Display.h"
#include "CommandContext.h"
#include "Utility.h"
#include "GameInput.h"
#include "BufferManager.h"
#include "Camera.h"
#include "Math/Random.h"

#include "xess/xess_d3d12_debug.h"
#include "xess/xess_debug.h"

#include <fstream>
#include <time.h>
#include "Log.h"

using namespace Graphics;
using namespace Math;
using namespace XeSS;
using namespace DirectX;

namespace GameCore
{
    extern HWND g_hWnd;
}

namespace Graphics
{
    extern RootSignature s_PresentRS;
    extern DXGI_FORMAT DefaultHdrColorFormat;
} // namespace Graphics

namespace XeSSDebug
{
    static const uint32_t TOTAL_DUMP_COUNT = 32;

    bool BypassXeSS = false;

    bool s_IsBufferDebugEnabled = false;

    ColorBuffer g_DebugBufferOutput;

    int32_t s_NetworkModel = XESS_NETWORK_MODEL_KPSS;

    int32_t g_JitterScaleConfig = 0;
    int32_t g_VelocityScaleConfig = 0;

    bool s_DumpEnabled = false;
    bool s_DumpDynamic = false;
    uint32_t s_DumpFrameIndex = 0;

    std::wstring s_DumpRootFolder;
    std::string s_DumpRootFolderAnsi;

    RandomNumberGenerator s_RNG;

    bool s_OldDrawFrameRate = false;
    bool s_OldTuningIsVisible = false;

    GraphicsPSO s_SimpleUpsamplePS(L"Simple Upsample PSO for XeSS Debugging");
    GraphicsPSO s_SimpleUpsamplePS_RGBA(L"Simple Upsample PSO for XeSS Debugging - RGBA");

    bool CreateFolderIfNotExist(const std::wstring& folder);
    std::string GetTimeStampString();
    bool PrepareDumpFolder();

    bool StartCurrentDump();
    void EndCurrentDump();
} // namespace XeSSDebug

void XeSSDebug::Initialize()
{
    uint32_t bypassXeSS = false;
    if (CommandLineArgs::GetInteger(L"bypassxess", bypassXeSS) && bypassXeSS)
    {
        BypassXeSS = true;
    }

    s_SimpleUpsamplePS.SetRootSignature(s_PresentRS);
    s_SimpleUpsamplePS.SetRasterizerState(RasterizerTwoSided);
    s_SimpleUpsamplePS.SetBlendState(BlendDisable);
    s_SimpleUpsamplePS.SetDepthStencilState(DepthStateDisabled);
    s_SimpleUpsamplePS.SetSampleMask(0xFFFFFFFF);
    s_SimpleUpsamplePS.SetInputLayout(0, nullptr);
    s_SimpleUpsamplePS.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    s_SimpleUpsamplePS.SetVertexShader(g_pScreenQuadPresentVS, sizeof(g_pScreenQuadPresentVS));
    s_SimpleUpsamplePS.SetPixelShader(g_pXeSSDebugSimpleUpsamplePS, sizeof(g_pXeSSDebugSimpleUpsamplePS));
    s_SimpleUpsamplePS.SetRenderTargetFormat(DefaultHdrColorFormat, DXGI_FORMAT_UNKNOWN);
    s_SimpleUpsamplePS.Finalize();

    s_SimpleUpsamplePS_RGBA.SetRootSignature(s_PresentRS);
    s_SimpleUpsamplePS_RGBA.SetRasterizerState(RasterizerTwoSided);
    s_SimpleUpsamplePS_RGBA.SetBlendState(BlendDisable);
    s_SimpleUpsamplePS_RGBA.SetDepthStencilState(DepthStateDisabled);
    s_SimpleUpsamplePS_RGBA.SetSampleMask(0xFFFFFFFF);
    s_SimpleUpsamplePS_RGBA.SetInputLayout(0, nullptr);
    s_SimpleUpsamplePS_RGBA.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    s_SimpleUpsamplePS_RGBA.SetVertexShader(g_pScreenQuadPresentVS, sizeof(g_pScreenQuadPresentVS));
    s_SimpleUpsamplePS_RGBA.SetPixelShader(g_pXeSSDebugSimpleUpsampleRGBAPS, sizeof(g_pXeSSDebugSimpleUpsampleRGBAPS));
    s_SimpleUpsamplePS_RGBA.SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN);
    s_SimpleUpsamplePS_RGBA.Finalize();
}

void XeSSDebug::Shutdown()
{
    // Nothing to do.
}

bool XeSSDebug::IsBufferDebugEnabled()
{
    return s_IsBufferDebugEnabled;
}

void XeSSDebug::SetBufferDebugEnabled(bool Enabled)
{
    if (s_IsBufferDebugEnabled == Enabled)
        return;

    if (Enabled)
    {
        RecreateDebugBuffers();
    }
    else
    {
        DestroyDebugBuffers();
    }

    s_IsBufferDebugEnabled = Enabled;
}

void XeSSDebug::RecreateDebugBuffers()
{
    g_CommandManager.IdleGPU();
    g_DebugBufferOutput.Create(L"XeSS Output for Debugging", Graphics::g_DisplayWidth, Graphics::g_DisplayHeight, 1, Graphics::DefaultHdrColorFormat);

    LOG_DEBUG("XeSS: Debug buffers recreated.");
}

void XeSSDebug::DestroyDebugBuffers()
{
    g_CommandManager.IdleGPU();
    g_DebugBufferOutput.Destroy();

    LOG_DEBUG("XeSS: Debug buffers destroyed.");
}

void XeSSDebug::UpscaleRGB(GraphicsContext& Context, ColorBuffer& source, ColorBuffer& destination)
{
    ScopedTimer _prof(L"XeSS Debug Upscale", Context);

    Context.SetRootSignature(s_PresentRS);
    Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Context.SetDynamicDescriptor(0, 0, source.GetSRV());
    Context.SetPipelineState(s_SimpleUpsamplePS);
    Context.TransitionResource(source, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    Context.TransitionResource(destination, D3D12_RESOURCE_STATE_RENDER_TARGET);
    Context.SetRenderTarget(destination.GetRTV());
    Context.SetViewportAndScissor(0, 0, destination.GetWidth(), destination.GetHeight());
    Context.SetDynamicDescriptor(0, 0, source.GetSRV());
    Context.Draw(3);
}

void XeSSDebug::UpscaleRGBA(GraphicsContext& Context, ColorBuffer& source, ColorBuffer& destination)
{
    ScopedTimer _prof(L"XeSS Debug Upscale", Context);

    Context.SetRootSignature(s_PresentRS);
    Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Context.SetDynamicDescriptor(0, 0, source.GetSRV());
    Context.SetPipelineState(s_SimpleUpsamplePS_RGBA);
    Context.TransitionResource(source, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    Context.TransitionResource(destination, D3D12_RESOURCE_STATE_RENDER_TARGET);
    Context.SetRenderTarget(destination.GetRTV());
    Context.SetViewportAndScissor(0, 0, destination.GetWidth(), destination.GetHeight());
    Context.SetDynamicDescriptor(0, 0, source.GetSRV());
    Context.Draw(3);
}

bool XeSSDebug::BeginFrameDump(bool DynamicCamera)
{
    if (!XeSS::IsEnabled() || BypassXeSS)
        return false;

    if (s_DumpEnabled)
        return true;

    bool ret = PrepareDumpFolder();
    ASSERT(ret);
    if (!ret)
    {
        LOG_ERROR("XeSS Debug: Could not prepare folder for frame dump.");
        return false;
    }

    s_DumpEnabled = true;
    s_DumpFrameIndex = 0;
    s_DumpDynamic = DynamicCamera;

    s_RNG.SetSeed(static_cast<uint32_t>(time(nullptr)));

    s_DumpRootFolderAnsi = Utility::WideStringToAnsi(s_DumpRootFolder);

    if (!StartCurrentDump())
    {
        s_DumpEnabled = false;
        return false;
    }

    return true;
}

bool XeSSDebug::IsFrameDumpOn()
{
    return s_DumpEnabled;
}

uint32_t XeSSDebug::GetDumpFrameIndex()
{
    return s_DumpFrameIndex;
}

bool XeSSDebug::IsDumpDynamic()
{
    return s_DumpDynamic;
}

bool XeSSDebug::CreateFolderIfNotExist(const std::wstring& folder)
{
    DWORD attr = GetFileAttributesW(folder.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES)
    {
        if (!CreateDirectoryW(folder.c_str(), nullptr))
        {
            LOG_ERRORF("XeSS Debug: Could not create folder \"%s\"", Utility::WideStringToUTF8(folder).c_str());
            return false;
        }
    }

    return true;
}

std::string XeSSDebug::GetTimeStampString()
{
    time_t t;
    time(&t);
    tm localTime;
    errno_t err = localtime_s(&localTime, &t);
    if (err)
    {
        LOG_ERROR("XeSS Debug: Could not get local time.");
        return "00";
    }

    char timeBuf[128];
    strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d_%H_%M_%S", &localTime);

    return timeBuf;
}

bool XeSSDebug::PrepareDumpFolder()
{
    wchar_t modulePath[MAX_PATH];
    modulePath[0] = 0;
    GetModuleFileNameW(nullptr, modulePath, MAX_PATH);

    s_DumpRootFolder = modulePath;
    std::wstring::size_type pos = s_DumpRootFolder.find_last_of(L"\\/");
    s_DumpRootFolder = s_DumpRootFolder.substr(0, pos);
    s_DumpRootFolder.append(L"\\frame_dump");

    if (!CreateFolderIfNotExist(s_DumpRootFolder))
        return false;

    std::string timeStamp = GetTimeStampString();

    s_DumpRootFolder.append(L"\\dump_");
    s_DumpRootFolder.append(Utility::AnsiToWideString(timeStamp));

    if (!CreateFolderIfNotExist(s_DumpRootFolder))
        return false;

    return true;
}

bool XeSSDebug::StartCurrentDump()
{
    ASSERT(s_DumpEnabled);
    if (!s_DumpEnabled)
        return false;

    xess_dump_parameters_t param = { s_DumpRootFolderAnsi.c_str(), s_DumpFrameIndex, TOTAL_DUMP_COUNT, XESS_DUMP_ALL_INPUTS };
    xess_result_t ret = xessStartDump(g_XeSSRuntime.GetContext(), &param);

    ASSERT(ret == XESS_RESULT_SUCCESS);
    if (ret != XESS_RESULT_SUCCESS)
    {
        LOG_ERRORF("XeSS: Could not start to dump. Result - %s.", ResultToString(ret));
        return false;
    }

    LOG_INFO("XeSS: Frame dump started.");

    return true;
}

void XeSSDebug::Update()
{
    if (s_DumpEnabled)
    {
        if (s_DumpFrameIndex >= TOTAL_DUMP_COUNT)
        {
            EndCurrentDump();
            return;
        }

        s_DumpFrameIndex++;
    }
}

int32_t XeSSDebug::GetNetworkModel()
{
    return s_NetworkModel;
}

bool XeSSDebug::SelectNetworkModel(int32_t model)
{
    if (s_NetworkModel == model)
        return true;

    // We make sure the previous work is done.
    g_CommandManager.IdleGPU();

    s_NetworkModel = model;

    // Update the runtime after new network model set.
    XeSS::UpdateRuntime();

    if (model == XESS_NETWORK_MODEL_KPSS)
    {
        LOG_INFO("XeSS: Network model set to KPSS.");
    }

    return true;
}

void XeSSDebug::EndCurrentDump()
{
    s_DumpEnabled = false;
    s_DumpFrameIndex = 0;
    s_DumpDynamic = false;
}

void XeSSDebug::UpdateCameraYaw(float& yaw)
{
    if (!s_DumpEnabled || !s_DumpDynamic)
        return;

    // We add some random camera rotation here.
    yaw -= s_RNG.NextFloat(0.02f, 0.1f);
}
