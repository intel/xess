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
// Added support for super sampling. Fullscreen, tearing, DPI scale, arbitrary resolution supported.

#include "pch.h"
#include "Display.h"
#include "GraphicsCore.h"
#include "BufferManager.h"
#include "ColorBuffer.h"
#include "SystemTime.h"
#include "CommandContext.h"
#include "RootSignature.h"
#include "ImageScaling.h"
#include "TemporalEffects.h"
#include "shellscalingapi.h"

#pragma comment(lib, "Shcore.lib")
#pragma comment(lib, "dxgi.lib") 

// This macro determines whether to detect if there is an HDR display and enable HDR10 output.
// Currently, with HDR display enabled, the pixel magnfication functionality is broken.
#define CONDITIONALLY_ENABLE_HDR_OUTPUT 1

namespace GameCore { extern HWND g_hWnd; }

#include "CompiledShaders/ScreenQuadPresentVS.h"
#include "CompiledShaders/BufferCopyPS.h"
#include "CompiledShaders/PresentSDRPS.h"
#include "CompiledShaders/PresentHDRPS.h"
#include "CompiledShaders/CompositeSDRPS.h"
#include "CompiledShaders/ScaleAndCompositeSDRPS.h"
#include "CompiledShaders/CompositeHDRPS.h"
#include "CompiledShaders/BlendUIHDRPS.h"
#include "CompiledShaders/ScaleAndCompositeHDRPS.h"
#include "CompiledShaders/MagnifyPixelsPS.h"
#include "CompiledShaders/GenerateMipsLinearCS.h"
#include "CompiledShaders/GenerateMipsLinearOddCS.h"
#include "CompiledShaders/GenerateMipsLinearOddXCS.h"
#include "CompiledShaders/GenerateMipsLinearOddYCS.h"
#include "CompiledShaders/GenerateMipsGammaCS.h"
#include "CompiledShaders/GenerateMipsGammaOddCS.h"
#include "CompiledShaders/GenerateMipsGammaOddXCS.h"
#include "CompiledShaders/GenerateMipsGammaOddYCS.h"

#define SWAP_CHAIN_BUFFER_COUNT 3

DXGI_FORMAT SwapChainFormat = DXGI_FORMAT_R10G10B10A2_UNORM;

using namespace Math;
using namespace ImageScaling;
using namespace Graphics;

namespace Display
{
    float s_FrameTime = 0.0f;
    uint64_t s_FrameIndex = 0;
    int64_t s_FrameStartTick = 0;
    float s_DPIScale = 1.0f;

    BoolVar s_EnableVSync("Timing/VSync", true);

    bool g_SupportTearing = false;
    bool s_Fullscreen = false;
    DWORD s_WindowStyle = 0;
    RECT s_WindowRect = { 0, 0, 100, 100 };
    //BoolVar s_LimitTo30Hz("Timing/Limit To 30Hz", false);
    //BoolVar s_DropRandomFrames("Timing/Drop Random Frames", false);

    void CheckTearingSupport();
}

namespace Graphics
{
    void PreparePresentSDR();
    void PreparePresentHDR();
    void CompositeOverlays( GraphicsContext& Context );

    enum eEQAAQuality { kEQAA1x1, kEQAA1x8, kEQAA1x16 };

    const uint32_t kNumPredefinedResolutions = 6;

    const char* ResolutionLabels[] = { "1280x720", "1600x900", "1920x1080", "2560x1440", "3200x1800", "3840x2160" };

    EnumVar NativeResolution("Graphics/Display/Native Resolution", k1080p, kNumPredefinedResolutions, ResolutionLabels);

#ifdef _GAMING_DESKTOP
    // This can set the window size to common dimensions.  It's also possible for the window to take on other dimensions
    // through resizing or going full-screen.
    EnumVar DisplayResolution("Graphics/Display/Display Resolution", k1080p, kNumPredefinedResolutions, ResolutionLabels);
#endif

    bool g_bEnableHDROutput = false;

    bool g_bUpscalingEnabled = false;

    bool g_bArbitraryResolution = false;

    NumVar g_HDRPaperWhite("Graphics/Display/Paper White (nits)", 200.0f, 100.0f, 500.0f, 50.0f);
    NumVar g_MaxDisplayLuminance("Graphics/Display/Peak Brightness (nits)", 1000.0f, 500.0f, 10000.0f, 100.0f);
    const char* HDRModeLabels[] = { "HDR", "SDR", "Side-by-Side" };
    EnumVar HDRDebugMode("Graphics/Display/HDR Debug Mode", 0, 3, HDRModeLabels);

    uint32_t g_NativeWidth = 0;
    uint32_t g_NativeHeight = 0;
    uint32_t g_DisplayWidth = 2560;
    uint32_t g_DisplayHeight = 1440;

    ColorBuffer g_PreDisplayBuffer;

    void ResolutionToUINT(eResolution res, uint32_t& width, uint32_t& height)
    {
        switch (res)
        {
        default:
        case k720p:
            width = 1280;
            height = 720;
            break;
        case k900p:
            width = 1600;
            height = 900;
            break;
        case k1080p:
            width = 1920;
            height = 1080;
            break;
        case k1440p:
            width = 2560;
            height = 1440;
            break;
        case k1800p:
            width = 3200;
            height = 1800;
            break;
        case k2160p:
            width = 3840;
            height = 2160;
            break;
        }
    }

    void SetNativeResolution(void)
    {
        if (g_bArbitraryResolution)
            return;

        uint32_t NativeWidth, NativeHeight;

        ResolutionToUINT(eResolution((int)NativeResolution), NativeWidth, NativeHeight);

        if (g_NativeWidth == NativeWidth && g_NativeHeight == NativeHeight)
            return;
        LOG_DEBUGF("Changing native resolution to %ux%u.", NativeWidth, NativeHeight);

        g_NativeWidth = NativeWidth;
        g_NativeHeight = NativeHeight;

        g_CommandManager.IdleGPU();

        InitializeRenderingBuffers(NativeWidth, NativeHeight);
    }

    void SetDisplayResolution(void)
    {
#ifdef _GAMING_DESKTOP
        if (g_bArbitraryResolution)
            return;

        static int SelectedDisplayRes = DisplayResolution;
        if (SelectedDisplayRes == DisplayResolution)
            return;

        SelectedDisplayRes = DisplayResolution;

        ResolutionToUINT((eResolution)SelectedDisplayRes, g_DisplayWidth, g_DisplayHeight);
        LOG_DEBUGF("Changing display resolution to %ux%u.", g_DisplayWidth, g_DisplayHeight);

        g_CommandManager.IdleGPU();

        Display::Resize(g_DisplayWidth, g_DisplayHeight);

        RECT rc = { 0, 0, (LONG)g_DisplayWidth, (LONG)g_DisplayHeight };
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

        SetWindowPos(GameCore::g_hWnd, nullptr, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
    }

    void SetNativeResolutionAndUpscaling(uint32_t Width, uint32_t Height, bool UpscalingEnabled)
    {
        ASSERT(Width);
        ASSERT(Height);
        if (!Width || !Height)
            return;

        if (!g_bArbitraryResolution)
            return;

        if (g_NativeWidth == Width && g_NativeHeight == Height && g_bUpscalingEnabled == UpscalingEnabled)
            return;

        LOG_DEBUGF("Set native resolution to %ux%u and upscaling=%d.", Width, Height, UpscalingEnabled);

        g_NativeWidth = Width;
        g_NativeHeight = Height;

        g_bUpscalingEnabled = UpscalingEnabled;

        g_CommandManager.IdleGPU();

        InitializeRenderingBuffers(Width, Height);
    }

    ColorBuffer g_DisplayPlane[SWAP_CHAIN_BUFFER_COUNT];
    UINT g_CurrentBuffer = 0;

    IDXGISwapChain1* s_SwapChain1 = nullptr;

    RootSignature s_PresentRS;
    GraphicsPSO s_BlendUIPSO(L"Core: BlendUI");
    GraphicsPSO s_BlendUIHDRPSO(L"Core: BlendUIHDR");
    GraphicsPSO PresentSDRPS(L"Core: PresentSDR");
    GraphicsPSO PresentHDRPS(L"Core: PresentHDR");
    GraphicsPSO CompositeSDRPS(L"Core: CompositeSDR");
    GraphicsPSO ScaleAndCompositeSDRPS(L"Core: ScaleAndCompositeSDR");
    GraphicsPSO CompositeHDRPS(L"Core: CompositeHDR");
    GraphicsPSO ScaleAndCompositeHDRPS(L"Core: ScaleAndCompositeHDR");
    GraphicsPSO MagnifyPixelsPS(L"Core: MagnifyPixels");

    const char* FilterLabels[] = { "Bilinear", "Sharpening", "Bicubic", "Lanczos" };
    EnumVar UpsampleFilter("Graphics/Display/Scaling Filter", kSharpening, kFilterCount, FilterLabels);

    enum DebugZoomLevel { kDebugZoomOff, kDebugZoom2x, kDebugZoom4x, kDebugZoom8x, kDebugZoom16x, kDebugZoomCount };
    const char* DebugZoomLabels[] = { "Off", "2x Zoom", "4x Zoom", "8x Zoom", "16x Zoom" };
    EnumVar DebugZoom("Graphics/Display/Magnify Pixels", kDebugZoomOff, kDebugZoomCount, DebugZoomLabels);
}

void Display::Resize(uint32_t width, uint32_t height)
{
    if (!width || !height)
        return;

    g_CommandManager.IdleGPU();

    g_DisplayWidth = width;
    g_DisplayHeight = height;

    LOG_DEBUGF("Changing display resolution to %ux%u.", width, height);

    g_PreDisplayBuffer.Create(L"PreDisplay Buffer", width, height, 1, SwapChainFormat);

    for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
        g_DisplayPlane[i].Destroy();

    ASSERT(s_SwapChain1 != nullptr);
    ASSERT_SUCCEEDED(s_SwapChain1->ResizeBuffers(SWAP_CHAIN_BUFFER_COUNT, width, height, SwapChainFormat, 
        g_SupportTearing ? (DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) : DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

    for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
    {
        ComPtr<ID3D12Resource> DisplayPlane;
        ASSERT_SUCCEEDED(s_SwapChain1->GetBuffer(i, MY_IID_PPV_ARGS(&DisplayPlane)));
        g_DisplayPlane[i].CreateFromSwapChain(L"Primary SwapChain Buffer", DisplayPlane.Detach());
    }

    g_CurrentBuffer = 0;

    g_CommandManager.IdleGPU();

    ResizeDisplayDependentBuffers(g_NativeWidth, g_NativeHeight);
}

// Initialize the DirectX resources required to run.
void Display::Initialize(void)
{
    ASSERT(s_SwapChain1 == nullptr, "Graphics has already been initialized");

    // Test tearing support.
    CheckTearingSupport();

    ASSERT(GameCore::g_hWnd);

    // Record initial DPI scale.
    UpdateDPIScale();

    // Record window style.
    s_WindowStyle = GetWindowLong(GameCore::g_hWnd, GWL_STYLE);

    // Record window rect.
    GetWindowRect(GameCore::g_hWnd, &s_WindowRect);

    Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
    ASSERT_SUCCEEDED(CreateDXGIFactory2(0, MY_IID_PPV_ARGS(&dxgiFactory)));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = g_DisplayWidth;
    swapChainDesc.Height = g_DisplayHeight;
    swapChainDesc.Format = SwapChainFormat;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    swapChainDesc.Flags = g_SupportTearing ? (DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) : DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
    fsSwapChainDesc.Windowed = TRUE;

    ASSERT_SUCCEEDED(dxgiFactory->CreateSwapChainForHwnd(
        g_CommandManager.GetCommandQueue(),
        GameCore::g_hWnd,
        &swapChainDesc,
        &fsSwapChainDesc,
        nullptr,
        &s_SwapChain1));

    // We will handle ALT+Enter key presses in the window message loop.
    dxgiFactory->MakeWindowAssociation(GameCore::g_hWnd, DXGI_MWA_NO_ALT_ENTER);

#if CONDITIONALLY_ENABLE_HDR_OUTPUT
    {
        IDXGISwapChain4* swapChain = (IDXGISwapChain4*)s_SwapChain1;
        ComPtr<IDXGIOutput> output;
        ComPtr<IDXGIOutput6> output6;
        DXGI_OUTPUT_DESC1 outputDesc;
        UINT colorSpaceSupport;

        // Query support for ST.2084 on the display and set the color space accordingly
        if (SUCCEEDED(swapChain->GetContainingOutput(&output)) && SUCCEEDED(output.As(&output6)) &&
            SUCCEEDED(output6->GetDesc1(&outputDesc)) && outputDesc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 &&
            SUCCEEDED(swapChain->CheckColorSpaceSupport(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, &colorSpaceSupport)) &&
            (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) &&
            SUCCEEDED(swapChain->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)))
        {
            g_bEnableHDROutput = true;
        }
    }
#endif // End CONDITIONALLY_ENABLE_HDR_OUTPUT

    for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
    {
        ComPtr<ID3D12Resource> DisplayPlane;
        ASSERT_SUCCEEDED(s_SwapChain1->GetBuffer(i, MY_IID_PPV_ARGS(&DisplayPlane)));
        g_DisplayPlane[i].CreateFromSwapChain(L"Primary SwapChain Buffer", DisplayPlane.Detach());
    }

    s_PresentRS.Reset(4, 2);
    s_PresentRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
    s_PresentRS[1].InitAsConstants(0, 6, D3D12_SHADER_VISIBILITY_ALL);
    s_PresentRS[2].InitAsBufferSRV(2, D3D12_SHADER_VISIBILITY_PIXEL);
    s_PresentRS[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
    s_PresentRS.InitStaticSampler(0, SamplerLinearClampDesc);
    s_PresentRS.InitStaticSampler(1, SamplerPointClampDesc);
    s_PresentRS.Finalize(L"Present");

    // Initialize PSOs
    s_BlendUIPSO.SetRootSignature(s_PresentRS);
    s_BlendUIPSO.SetRasterizerState( RasterizerTwoSided );
    s_BlendUIPSO.SetBlendState( BlendPreMultiplied );
    s_BlendUIPSO.SetDepthStencilState( DepthStateDisabled );
    s_BlendUIPSO.SetSampleMask(0xFFFFFFFF);
    s_BlendUIPSO.SetInputLayout(0, nullptr);
    s_BlendUIPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    s_BlendUIPSO.SetVertexShader( g_pScreenQuadPresentVS, sizeof(g_pScreenQuadPresentVS) );
    s_BlendUIPSO.SetPixelShader( g_pBufferCopyPS, sizeof(g_pBufferCopyPS) );
    s_BlendUIPSO.SetRenderTargetFormat(SwapChainFormat, DXGI_FORMAT_UNKNOWN);
    s_BlendUIPSO.Finalize();

    s_BlendUIHDRPSO = s_BlendUIPSO;
    s_BlendUIHDRPSO.SetPixelShader(g_pBlendUIHDRPS, sizeof(g_pBlendUIHDRPS));
    s_BlendUIHDRPSO.Finalize();

#define CreatePSO( ObjName, ShaderByteCode ) \
    ObjName = s_BlendUIPSO; \
    ObjName.SetBlendState( BlendDisable ); \
    ObjName.SetPixelShader(ShaderByteCode, sizeof(ShaderByteCode) ); \
    ObjName.Finalize();

    CreatePSO(PresentSDRPS, g_pPresentSDRPS);
    CreatePSO(CompositeSDRPS, g_pCompositeSDRPS);
    CreatePSO(ScaleAndCompositeSDRPS, g_pScaleAndCompositeSDRPS);
    CreatePSO(CompositeHDRPS, g_pCompositeHDRPS);
    CreatePSO(ScaleAndCompositeHDRPS, g_pScaleAndCompositeHDRPS);
    CreatePSO(MagnifyPixelsPS, g_pMagnifyPixelsPS);

    PresentHDRPS = PresentSDRPS;
    PresentHDRPS.SetPixelShader(g_pPresentHDRPS, sizeof(g_pPresentHDRPS));
    DXGI_FORMAT SwapChainFormats[2] = { DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_R10G10B10A2_UNORM };
    PresentHDRPS.SetRenderTargetFormats(2, SwapChainFormats, DXGI_FORMAT_UNKNOWN );
    PresentHDRPS.Finalize();

#undef CreatePSO

    SetNativeResolution();

    g_PreDisplayBuffer.Create(L"PreDisplay Buffer", g_DisplayWidth, g_DisplayHeight, 1, SwapChainFormat);
    ImageScaling::Initialize(g_PreDisplayBuffer.GetFormat());
}

void Display::Shutdown( void )
{
    s_SwapChain1->SetFullscreenState(FALSE, nullptr);
    s_SwapChain1->Release();

    for (UINT i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
        g_DisplayPlane[i].Destroy();

    g_PreDisplayBuffer.Destroy();
}

void Graphics::PreparePresentHDR(void)
{
    GraphicsContext& Context = GraphicsContext::Begin(L"Present");
    Context.SetRootSignature(s_PresentRS);
    Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    bool isUpscalingMode = Graphics::IsUpscalingEnabled();

    bool NeedsScaling = true;
    
    if (isUpscalingMode)
    {
        NeedsScaling = false;
    }
    else
    {
        NeedsScaling = g_NativeWidth != g_DisplayWidth || g_NativeHeight != g_DisplayHeight;
    }

    if (isUpscalingMode)
    {
        Context.TransitionResource(g_UpscaledSceneColorBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        Context.SetDynamicDescriptor(0, 0, g_UpscaledSceneColorBuffer.GetSRV());
    }
    else
    {
        Context.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        Context.SetDynamicDescriptor(0, 0, g_SceneColorBuffer.GetSRV());
    }

    ColorBuffer& Dest = DebugZoom == kDebugZoomOff ? g_DisplayPlane[g_CurrentBuffer] : g_PreDisplayBuffer;

    // On Windows, prefer scaling and compositing in one step via pixel shader
    Context.SetRootSignature(s_PresentRS);
    Context.TransitionResource(g_OverlayBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    if (DebugZoom == kDebugZoomOff)
    {
        Context.SetDynamicDescriptor(0, 1, g_OverlayBuffer.GetSRV());
        Context.SetPipelineState(NeedsScaling ? ScaleAndCompositeHDRPS : CompositeHDRPS);
    }
    else
    {
        Context.SetDynamicDescriptor(0, 1, GetDefaultTexture(kBlackTransparent2D));
        Context.SetPipelineState(NeedsScaling ? ScaleAndCompositeHDRPS : PresentHDRPS);
    }
    Context.SetConstants(1, (float)g_HDRPaperWhite / 10000.0f, (float)g_MaxDisplayLuminance,
        0.7071f / g_NativeWidth, 0.7071f / g_NativeHeight);
    Context.TransitionResource(Dest, D3D12_RESOURCE_STATE_RENDER_TARGET);
    Context.SetRenderTarget(Dest.GetRTV());
    Context.SetViewportAndScissor(0, 0, g_DisplayWidth, g_DisplayHeight);
    Context.Draw(3);

    // Magnify without stretching
    if (DebugZoom != kDebugZoomOff)
    {
        Context.SetPipelineState(MagnifyPixelsPS);
        Context.TransitionResource(g_PreDisplayBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        Context.TransitionResource(g_DisplayPlane[g_CurrentBuffer], D3D12_RESOURCE_STATE_RENDER_TARGET);
        Context.SetRenderTarget(g_DisplayPlane[g_CurrentBuffer].GetRTV());
        Context.SetDynamicDescriptor(0, 0, g_PreDisplayBuffer.GetSRV());
        Context.SetViewportAndScissor(0, 0, g_DisplayWidth, g_DisplayHeight);
        Context.SetConstants(1, 1.0f / ((int)DebugZoom + 1.0f));
        Context.Draw(3);

        CompositeOverlays(Context);
    }

    Context.TransitionResource(g_DisplayPlane[g_CurrentBuffer], D3D12_RESOURCE_STATE_PRESENT);

    // Close the final context to be executed before frame present.
    Context.Finish();
}

void Graphics::CompositeOverlays( GraphicsContext& Context )
{
    // Now blend (or write) the UI overlay
    Context.SetRootSignature(s_PresentRS);
    Context.TransitionResource(g_OverlayBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    Context.SetDynamicDescriptor(0, 0, g_OverlayBuffer.GetSRV());
    Context.SetPipelineState(g_bEnableHDROutput ? s_BlendUIHDRPSO : s_BlendUIPSO);
    Context.SetConstants(1, (float)g_HDRPaperWhite / 10000.0f, (float)g_MaxDisplayLuminance);
    Context.Draw(3);
}

void Graphics::PreparePresentSDR(void)
{
    GraphicsContext& Context = GraphicsContext::Begin(L"Present");

    Context.SetRootSignature(s_PresentRS);
    Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    bool isUpscalingMode = Graphics::IsUpscalingEnabled();

    if (isUpscalingMode)
    {
        Context.TransitionResource(g_UpscaledSceneColorBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        Context.SetDynamicDescriptor(0, 0, g_UpscaledSceneColorBuffer.GetSRV());
    }
    else
    {
        Context.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        Context.SetDynamicDescriptor(0, 0, g_SceneColorBuffer.GetSRV());
    }

    bool NeedsScaling = true;

    if (isUpscalingMode)
    {
        NeedsScaling = false;
    }
    else
    {
        NeedsScaling = g_NativeWidth != g_DisplayWidth || g_NativeHeight != g_DisplayHeight;
    }

    // On Windows, prefer scaling and compositing in one step via pixel shader
    if (DebugZoom == kDebugZoomOff && (UpsampleFilter == kSharpening || !NeedsScaling))
    {
        Context.TransitionResource(g_OverlayBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        Context.SetDynamicDescriptor(0, 1, g_OverlayBuffer.GetSRV());
        Context.SetPipelineState(NeedsScaling ? ScaleAndCompositeSDRPS : CompositeSDRPS);
        Context.SetConstants(1, 0.7071f / g_NativeWidth, 0.7071f / g_NativeHeight);
        Context.TransitionResource(g_DisplayPlane[g_CurrentBuffer], D3D12_RESOURCE_STATE_RENDER_TARGET);
        Context.SetRenderTarget(g_DisplayPlane[g_CurrentBuffer].GetRTV());
        Context.SetViewportAndScissor(0, 0, g_DisplayWidth, g_DisplayHeight);
        Context.Draw(3);
    }
    else
    {
        ColorBuffer& Dest = DebugZoom == kDebugZoomOff ? g_DisplayPlane[g_CurrentBuffer] : g_PreDisplayBuffer;

        // Scale or Copy
        if (NeedsScaling)
        {
            ImageScaling::Upscale(Context, Dest, g_SceneColorBuffer, eScalingFilter((int)UpsampleFilter));
        }
        else
        {
            Context.SetPipelineState(PresentSDRPS);
            Context.TransitionResource(Dest, D3D12_RESOURCE_STATE_RENDER_TARGET);
            Context.SetRenderTarget(Dest.GetRTV());
            Context.SetViewportAndScissor(0, 0, g_DisplayWidth, g_DisplayHeight);
            Context.Draw(3);
        }

        // Magnify without stretching
        if (DebugZoom != kDebugZoomOff)
        {
            Context.SetPipelineState(MagnifyPixelsPS);
            Context.TransitionResource(g_PreDisplayBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            Context.TransitionResource(g_DisplayPlane[g_CurrentBuffer], D3D12_RESOURCE_STATE_RENDER_TARGET);
            Context.SetRenderTarget(g_DisplayPlane[g_CurrentBuffer].GetRTV());
            Context.SetDynamicDescriptor(0, 0, g_PreDisplayBuffer.GetSRV());
            Context.SetViewportAndScissor(0, 0, g_DisplayWidth, g_DisplayHeight);
            Context.SetConstants(1, 1.0f / ((int)DebugZoom + 1.0f));
            Context.Draw(3);
        }

        CompositeOverlays(Context);
    }


    Context.TransitionResource(g_DisplayPlane[g_CurrentBuffer], D3D12_RESOURCE_STATE_PRESENT);

    // Close the final context to be executed before frame present.
    Context.Finish();
}

void Display::Present(void)
{
    if (g_bEnableHDROutput)
        PreparePresentHDR();
    else
        PreparePresentSDR();

    BOOL fullscreenState = FALSE;
    ASSERT_SUCCEEDED(s_SwapChain1->GetFullscreenState(&fullscreenState, nullptr));
    
    //UINT PresentInterval = s_EnableVSync ? std::min(4, (int)Round(s_FrameTime * 60.0f)) : 0;

    //s_SwapChain1->Present(PresentInterval, 0);

    if (s_EnableVSync)
        s_SwapChain1->Present(1, 0);
    else
    {
        s_SwapChain1->Present(0, (g_SupportTearing && !fullscreenState) ? DXGI_PRESENT_ALLOW_TEARING : 0);
    }

    g_CurrentBuffer = (g_CurrentBuffer + 1) % SWAP_CHAIN_BUFFER_COUNT;

    // Test robustness to handle spikes in CPU time
    //if (s_DropRandomFrames)
    //{
    //	if (std::rand() % 25 == 0)
    //		BusyLoopSleep(0.010);
    //}

    int64_t CurrentTick = SystemTime::GetCurrentTick();

    /*
    if (s_EnableVSync)
    {
        // With VSync enabled, the time step between frames becomes a multiple of 16.666 ms.  We need
        // to add logic to vary between 1 and 2 (or 3 fields).  This delta time also determines how
        // long the previous frame should be displayed (i.e. the present interval.)
        s_FrameTime = (s_LimitTo30Hz ? 2.0f : 1.0f) / 60.0f;
        if (s_DropRandomFrames)
        {
            if (std::rand() % 50 == 0)
                s_FrameTime += (1.0f / 60.0f);
        }
    }
    else
    {
        // When running free, keep the most recent total frame time as the time step for
        // the next frame simulation.  This is not super-accurate, but assuming a frame
        // time varies smoothly, it should be close enough.
        s_FrameTime = (float)SystemTime::TimeBetweenTicks(s_FrameStartTick, CurrentTick);
    }
    */

    s_FrameTime = (float)SystemTime::TimeBetweenTicks(s_FrameStartTick, CurrentTick);

    s_FrameStartTick = CurrentTick;

    ++s_FrameIndex;

    TemporalEffects::Update((uint32_t)s_FrameIndex);

    // We need to know the display resolution to create upscaled buffers with correct sizes.
    // So we put SetDisplayResolution first.
    SetDisplayResolution();
    SetNativeResolution();
}

void Display::CheckTearingSupport()
{
    ComPtr<IDXGIFactory6> factory;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    BOOL allowTearing = FALSE;
    if (SUCCEEDED(hr))
    {
        hr = factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
    }

    g_SupportTearing = SUCCEEDED(hr) && allowTearing;

    LOG_INFOF("Present tearing support: %d.", g_SupportTearing);
}

bool Display::IsFullscreen()
{
    return s_Fullscreen;
}

void Display::SetFullscreen(bool fullscreen)
{
    if (s_Fullscreen == fullscreen)
        return;

    // Implementation based on D3D12Fullscreen sample:
    // https://github.com/microsoft/DirectX-Graphics-Samples/tree/master/Samples/Desktop/D3D12Fullscreen
    // In our implementation, if tearing is not supported, it actually falls back to borderless mode.

    if (fullscreen)
    {
        // Save the old window rect so we can restore it when exiting fullscreen mode.
        GetWindowRect(GameCore::g_hWnd, &s_WindowRect);

        // Make the window borderless so that the client area can fill the screen.
        SetWindowLong(GameCore::g_hWnd, GWL_STYLE, Display::s_WindowStyle & ~(WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME));

        RECT fullscreenWindowRect = s_WindowRect;

        bool fullscreenRectGot = false;

        if (Graphics::s_SwapChain1)
        {
            // Get the settings of the display on which the app's window is currently displayed
            ComPtr<IDXGIOutput> pOutput;
            HRESULT hr = Graphics::s_SwapChain1->GetContainingOutput(&pOutput);
            //ASSERT_SUCCEEDED(hr);
            if (SUCCEEDED(hr))
            {
                DXGI_OUTPUT_DESC Desc;
                hr = pOutput->GetDesc(&Desc);
                ASSERT_SUCCEEDED(hr);
                if (SUCCEEDED(hr))
                {
                    fullscreenWindowRect = Desc.DesktopCoordinates;
                    fullscreenRectGot = true;
                }
            }
        }

        if (!fullscreenRectGot)
        {
            MONITORINFO mi = { sizeof(mi) };
            if (GetMonitorInfo(MonitorFromWindow(GameCore::g_hWnd, MONITOR_DEFAULTTOPRIMARY), &mi))
            {
                fullscreenWindowRect = mi.rcMonitor;
            }
        }

        SetWindowPos(
            GameCore::g_hWnd,
            HWND_TOPMOST,
            fullscreenWindowRect.left,
            fullscreenWindowRect.top,
            fullscreenWindowRect.right,
            fullscreenWindowRect.bottom,
            SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ShowWindow(GameCore::g_hWnd, SW_MAXIMIZE);
    }
    else
    {
        // Restore the window's attributes and size.
        SetWindowLong(GameCore::g_hWnd, GWL_STYLE, Display::s_WindowStyle);

        SetWindowPos(
            GameCore::g_hWnd,
            HWND_NOTOPMOST,
            s_WindowRect.left,
            s_WindowRect.top,
            s_WindowRect.right - s_WindowRect.left,
            s_WindowRect.bottom - s_WindowRect.top,
            SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ShowWindow(GameCore::g_hWnd, SW_NORMAL);
    }

    s_Fullscreen = fullscreen;
}

void Display::UpdateDPIScale()
{
    ASSERT(GameCore::g_hWnd);
    if (!GameCore::g_hWnd)
        return;

    HMONITOR monitor = MonitorFromWindow(GameCore::g_hWnd, MONITOR_DEFAULTTONEAREST);
    if (!monitor)
        return;

    unsigned int dpiX;
    unsigned int dpiY;
    GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
    if (dpiX == 0 || dpiY == 0)
        return;

    s_DPIScale = std::max(dpiX / 96.f, dpiY / 96.f);

    LOG_DEBUGF("Display: DPI changed, DPI scale=%f", s_DPIScale);
}

float Display::GetDPIScale()
{
    return s_DPIScale;
}

uint64_t Graphics::GetFrameCount(void)
{
    return Display::s_FrameIndex;
}

float Graphics::GetFrameTime(void)
{
    return Display::s_FrameTime;
}

float Graphics::GetFrameRate(void)
{
    return Display::s_FrameTime == 0.0f ? 0.0f : 1.0f / Display::s_FrameTime;
}

bool Graphics::IsUpscalingEnabled()
{
    return g_bUpscalingEnabled;
}