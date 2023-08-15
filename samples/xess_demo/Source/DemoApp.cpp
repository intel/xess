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
#include "DemoApp.h"
#include "GameCore.h"
#include "CommandContext.h"
#include "BufferManager.h"
#include "DemoCameraController.h"
#include "ImGuiModule.h"
#include "MotionBlur.h"
#include "TemporalEffects.h"
#include "PostEffects.h"
#include "Renderer.h"
#include "SponzaRenderer.h"
#include "SSAO.h"
#include "FXAA.h"
#include "GameInput.h"
#include "ParticleEffectManager.h"
#include "DepthOfField.h"
#include "Display.h"
#include "EngineProfiling.h"
#include "Utility.h"
#include "DemoGui.h"
#include "XeSS/XeSSJitter.h"
#include "XeSS/XeSSProcess.h"
#include "XeSS/XeSSRuntime.h"
#include "ModelLoader.h"
#include "LightManager.h"
#include "ParticleEffects.h"

#include "xess/xess_d3d12_debug.h"


CREATE_APPLICATION(DemoApp)

using namespace GameCore;
using namespace Graphics;
using namespace Utility;
using namespace Renderer;

namespace GameInput
{
    extern bool g_bMouseExclusive;
}

namespace GameCore
{
    extern HWND g_hWnd;
}

namespace Graphics
{
    extern EnumVar DebugZoom;
    extern EnumVar NativeResolution;
    extern EnumVar DisplayResolution;
} // namespace Graphics

namespace EngineProfiling
{
    extern BoolVar DrawFrameRate;
}

ExpVar g_SunLightIntensity("Viewer/Lighting/Sun Light Intensity", 4.0f, 0.0f, 16.0f, 0.1f);
NumVar g_SunOrientation("Viewer/Lighting/Sun Orientation", -0.5f, -100.0f, 100.0f, 0.1f);
NumVar g_SunInclination("Viewer/Lighting/Sun Inclination", 0.75f, 0.0f, 1.0f, 0.01f);

void ChangeIBLBias(EngineVar::ActionType);
NumVar g_IBLBias("Viewer/Lighting/EnvironmentMap Blur", 0.0f, 0.0f, 10.0f, 0.1f, ChangeIBLBias);

NumVar g_EnvRotX("Viewer/Lighting/Environment Rotation X", 3, 0, 3, 1);
NumVar g_EnvRotY("Viewer/Lighting/Environment Rotation Y", 0, 0, 3, 1);


void ChangeIBLBias(EngineVar::ActionType)
{
    Renderer::SetIBLBias(g_IBLBias);
}

DemoApp::DemoApp()
    : m_ShowUI(true)
    , m_Technique(kDemoTech_XeSS)
{
    // We register extra buffers handler before graphics initialization.
    Graphics::SetExtraRenderingBuffersHandler(&m_ExtraBuffersHandler);

    // Use non-exclusive mouse input.
    GameInput::g_bMouseExclusive = false;
}

DemoApp::~DemoApp()
{
    Graphics::SetExtraRenderingBuffersHandler(nullptr);
}

void DemoApp::Startup()
{
    SetWindowText(GameCore::g_hWnd, L"XeSS Demo");

    // Enable console for debugging if needed.
    uint32_t console = 0;
    if (CommandLineArgs::GetInteger(L"console", console) && console)
    {
        if (AllocConsole())
        {
            FILE* stream;
            freopen_s(&stream, "CONOUT$", "w", stderr);
            freopen_s(&stream, "CONOUT$", "w", stdout);
        }
    }

    std::wstring cwd = GetWorkingDirectory();
    SetWorkingDirectory(GetProgramDirectory());

    FindAssetsDir();

    Graphics::g_bArbitraryResolution = true;
    Display::s_EnableVSync = false;
    EngineProfiling::DrawFrameRate = false;
    MotionBlur::Enable = true;
    TemporalEffects::EnableTAA = true;
    TemporalEffects::Sharpness = 0.0f;
    FXAA::Enable = false;
    PostEffects::EnableHDR = true;
    PostEffects::EnableAdaptation = true;
    SSAO::Enable = false;
    SSAO::g_QualityLevel = SSAO::kSsaoQualityVeryHigh;

    Renderer::Initialize();

    ImGuiModule::Initialize();

    DemoGui::Initialize();

    XeSS::Initialize();
    XeSS::SetEnabled(true);
    XeSS::SetQuality(XeSS::kQualityPerformance);
    XeSS::SetMotionVectorsMode(XeSS::kMotionVectorsHighRes);
    XeSS::SetMipBiasMode(XeSS::kMipBiasAutomatic);

    LoadIBLTextures();

    bool forceRebuild = false;
    uint32_t rebuildValue;
    if (CommandLineArgs::GetInteger(L"rebuild", rebuildValue))
        forceRebuild = rebuildValue != 0;

    std::wstring gltfFileName;
    if (CommandLineArgs::GetString(L"model", gltfFileName) == false)
    {
        m_ModeInstance = Renderer::LoadModel(m_AssetRootDir + L"/Sponza/pbr/sponza2.gltf", forceRebuild);
    }
    else
    {
        m_ModeInstance = Renderer::LoadModel(gltfFileName, forceRebuild);
    }

    if (!m_ModeInstance.IsNull())
    {
        m_ModeInstance.LoopAllAnimations();

        // Scale to match the size of old version of Sponza.
        float modelScale = 4637.0f / Math::Max((float)(Length(m_ModeInstance.GetModel()->m_BoundingBox.GetDimensions())), 0.1f);
        m_ModeInstance.Resize(m_ModeInstance.GetRadius() * modelScale);

        OrientedBox obb = m_ModeInstance.GetBoundingBox();
        float modelRadius = Length(obb.GetDimensions()) * 0.5f;
        const Vector3 eye = obb.GetCenter() + Vector3(modelRadius * 0.5f, 0.0f, 0.0f);
        m_Camera.SetEyeAtUp(eye, Vector3(kZero), Vector3(kYUnitVector));
        m_Camera.SetZRange(1.0f, 10000.0f);

        m_CameraController.reset(new DemoCameraController(m_Camera, Vector3(kYUnitVector)));
        m_Camera.SetPerspectiveMatrix(XM_PIDIV4, g_DisplayHeight / static_cast<float>(g_DisplayWidth), 1.0f, 10000.0f);

        Lighting::CreateRandomLights(m_ModeInstance.GetModel()->m_BoundingBox.GetMin() * modelScale, m_ModeInstance.GetModel()->m_BoundingBox.GetMax() * modelScale);
    }

    ParticleEffects::InitFromJSON(m_AssetRootDir + L"/Particle/particles.json", m_AssetRootDir);

    SetWorkingDirectory(cwd);
}

void DemoApp::Cleanup(void)
{
    m_Log.Flush();

    DemoGui::Shutdown();

    ImGuiModule::Shutdown();

    XeSS::Shutdown();

    ParticleEffects::ClearTexturePool();

    m_ModeInstance = nullptr;

    Renderer::Shutdown();
}

void DemoApp::Update(float deltaTime)
{
    ScopedTimer _prof(L"Update State");

    m_Log.Flush();

    UpdateResolution();

    ImGuiModule::PreGUI();

    DemoGui::OnGUI(*this);

    XeSS::Update();

    if (GameInput::IsFirstPressed(GameInput::kLShoulder))
        DebugZoom.Decrement();
    else if (GameInput::IsFirstPressed(GameInput::kRShoulder))
        DebugZoom.Increment();

    if (m_Technique == kDemoTech_XeSS)
    {
        // Choose "jittered" or "unjittered" mode for the main camera to work with XeSS.
        m_Camera.SetReprojectMatrixJittered(XeSS::IsMotionVectorsJittered());

        // Apply projection jitter for the main camera.
        float jitterX, jitterY;
        XeSSJitter::GetJitterValues(jitterX, jitterY);
        XeSSJitter::ApplyCameraJitter(m_Camera, jitterX, jitterY);
    }
    else
    {
        // Only "unjittered" mode is supported by TAA.
        m_Camera.SetReprojectMatrixJittered(false);

        float jitterX, jitterY;
        TemporalEffects::GetJitterOffset(jitterX, jitterY);

        // Instead of using viewport offset, we use projection matrix jitter.
        // So here we borrow function from XeSSJitter to apply jitter to the camera.
        XeSSJitter::ApplyCameraJitter(m_Camera, jitterX, jitterY);
    }

    m_CameraController->Update(deltaTime);

    m_MainViewport.TopLeftX = 0.0f;
    m_MainViewport.TopLeftY = 0.0f;
    m_MainViewport.Width = (float)g_SceneColorBuffer.GetWidth();
    m_MainViewport.Height = (float)g_SceneColorBuffer.GetHeight();
    m_MainViewport.MinDepth = 0.0f;
    m_MainViewport.MaxDepth = 1.0f;

    m_MainScissor.left = 0;
    m_MainScissor.top = 0;
    m_MainScissor.right = (LONG)g_SceneColorBuffer.GetWidth();
    m_MainScissor.bottom = (LONG)g_SceneColorBuffer.GetHeight();

    ImGuiModule::PostGUI();

    GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Update");

    m_ModeInstance.Update(gfxContext, deltaTime);

    gfxContext.Finish();
}

void DemoApp::UpdateResolution()
{
    RECT rect;
    if (!GetClientRect(g_hWnd, &rect))
        return;

    uint32_t width = rect.right - rect.left;
    uint32_t height = rect.bottom - rect.top;

    if (!width || !height)
        return;

    if (g_DisplayWidth != width || g_DisplayHeight != height)
    {
        Display::Resize(width, height);

        XeSS::SetOutputResolution(width, height);
        XeSS::UpdateRuntime();

        if (m_Technique == kDemoTech_XeSS)
        {
            XeSS::UpdateInputResolution();
        }
        else if (m_Technique == kDemoTech_TAANative)
        {
            Graphics::SetNativeResolutionAndUpscaling(g_DisplayWidth, g_DisplayHeight, false);
        }
        else if (m_Technique == kDemoTech_TAAScaled)
        {
            uint32_t nativeWidth, nativeHeight;
            XeSS::GetInputResolution(nativeWidth, nativeHeight);
            Graphics::SetNativeResolutionAndUpscaling(nativeWidth, nativeHeight, false);
        }

        g_DisplayWidth = width;
        g_DisplayHeight = height;

        m_Camera.SetPerspectiveMatrix(XM_PIDIV4, height / static_cast<float>(width),
            m_Camera.GetNearClip(), m_Camera.GetFarClip());
    }
}

void DemoApp::RenderScene(void)
{
    GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Render");
    RenderSceneImpl(gfxContext);
    gfxContext.Finish();

    if (XeSS::IsProfilingEnabled())
    {
        XeSS::g_XeSSRuntime.DoGPUProfile();
    }
}

void DemoApp::RenderSceneImpl(GraphicsContext& gfxContext)
{
    uint32_t FrameIndex = TemporalEffects::GetFrameIndexMod2();
    const D3D12_VIEWPORT& viewport = m_MainViewport;
    const D3D12_RECT& scissor = m_MainScissor;

    if (ParticleEffectManager::Enable)
    {
        ParticleEffectManager::Update(gfxContext.GetComputeContext(), Graphics::GetFrameTime());
    }

    float mipBias = (m_Technique == kDemoTech_XeSS || m_Technique == kDemoTech_TAAScaled) ? XeSS::GetMipBias() : 0.0f;
    {
        // Update global constants
        float costheta = cosf(g_SunOrientation);
        float sintheta = sinf(g_SunOrientation);
        float cosphi = cosf(g_SunInclination * 3.14159f * 0.5f);
        float sinphi = sinf(g_SunInclination * 3.14159f * 0.5f);

        Vector3 SunDirection = Normalize(Vector3(costheta * cosphi, sinphi, sintheta * cosphi));
        Vector3 ShadowBounds = Vector3(m_ModeInstance.GetRadius());
        m_SunShadowCamera.UpdateMatrix(-SunDirection, m_ModeInstance.GetCenter(), ShadowBounds,
            (uint32_t)g_ShadowBuffer.GetWidth(), (uint32_t)g_ShadowBuffer.GetHeight(), 16);

        GlobalConstants globals;
        globals.ViewProjMatrix = m_Camera.GetViewProjMatrix();
        globals.SunShadowMatrix = m_SunShadowCamera.GetShadowMatrix();
        globals.EnvRotation = Matrix3::MakeYRotation(XM_PIDIV2 * float(g_EnvRotX)) * Matrix3::MakeXRotation(XM_PIDIV2 * float(g_EnvRotY));
        globals.CameraPos = m_Camera.GetPosition();
        globals.SunDirection = SunDirection;
        globals.SunIntensity = Scalar(g_SunLightIntensity);
        globals.ViewMipBias = mipBias;

        // Lights shadow
        {
            using namespace Lighting;

            static uint32_t LightIndex = 0;
            if (LightIndex < MaxLights)
            {
                ScopedTimer _prof(L"Generate lights shadow", gfxContext);

                m_LightShadowTempBuffer.BeginRendering(gfxContext);
                {
                    ShadowCamera lightShadowCamera;
                    lightShadowCamera.SetPosition(m_Camera.GetPosition());
                    lightShadowCamera.SetViewProjMatrix(m_LightShadowMatrix[LightIndex]);

                    MeshSorter shadowSorter(MeshSorter::kShadows);

                    shadowSorter.SetCullEnabled(false);
                    shadowSorter.SetCamera(lightShadowCamera);
                    shadowSorter.SetDepthStencilTarget(m_LightShadowTempBuffer);

                    m_ModeInstance.Render(shadowSorter);

                    shadowSorter.Sort();
                    shadowSorter.RenderMeshes(MeshSorter::kZPass, gfxContext, globals);
                }

                gfxContext.TransitionResource(m_LightShadowTempBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
                gfxContext.TransitionResource(m_LightShadowArray, D3D12_RESOURCE_STATE_COPY_DEST);

                gfxContext.CopySubresource(m_LightShadowArray, LightIndex, m_LightShadowTempBuffer, 0);

                gfxContext.TransitionResource(m_LightShadowArray, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

                ++LightIndex;
            }
        }

        // Begin rendering depth
        gfxContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
        gfxContext.ClearDepth(g_SceneDepthBuffer);

        MeshSorter sorter(MeshSorter::kDefault);
        sorter.SetCamera(m_Camera);
        sorter.SetViewport(viewport);
        sorter.SetScissor(scissor);
        sorter.SetDepthStencilTarget(g_SceneDepthBuffer);
        sorter.AddRenderTarget(g_SceneColorBuffer);

        m_ModeInstance.Render(sorter);

        sorter.Sort();

        {
            ScopedTimer _prof(L"Depth Pre-Pass", gfxContext);
            sorter.RenderMeshes(MeshSorter::kZPass, gfxContext, globals);
        }

        SSAO::Render(gfxContext, m_Camera);

        // Fill light grid for transparent objects.
        Lighting::FillLightGrid(gfxContext, m_Camera, true);
        // Fill light grid for solid objects.
        Lighting::FillLightGrid(gfxContext, m_Camera, false);

        if (!SSAO::DebugDraw)
        {
            ScopedTimer _outerprof(L"Main Render", gfxContext);

            {
                ScopedTimer _prof(L"Sun Shadow Map", gfxContext);

                MeshSorter shadowSorter(MeshSorter::kShadows);
                shadowSorter.SetCamera(m_SunShadowCamera);
                shadowSorter.SetDepthStencilTarget(g_ShadowBuffer);
                shadowSorter.SetCullEnabled(false);

                m_ModeInstance.Render(shadowSorter);

                shadowSorter.Sort();
                shadowSorter.RenderMeshes(MeshSorter::kZPass, gfxContext, globals);
            }

            gfxContext.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
            gfxContext.ClearColor(g_SceneColorBuffer);

            {
                ScopedTimer _prof(L"Render Opaque", gfxContext);

                gfxContext.TransitionResource(g_SSAOFullScreen, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                gfxContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_READ);
                gfxContext.SetRenderTarget(g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV_DepthReadOnly());
                gfxContext.SetViewportAndScissor(viewport, scissor);

                sorter.RenderMeshes(MeshSorter::kOpaque, gfxContext, globals);
            }

            Renderer::DrawSkybox(gfxContext, m_Camera, viewport, scissor, globals.EnvRotation);

            {
                ScopedTimer _prof(L"Render Transparent", gfxContext);

                sorter.RenderMeshes(MeshSorter::kTransparent, gfxContext, globals);
            }
        }
    }

    // Only update responsive when XeSS, may become a common requirement.
    const bool isUpdateResponsiveMask = (m_Technique == kDemoTech_XeSS && XeSS::IsResponsiveMaskEnabled());

    // clear responsive mask if necessary
    if (isUpdateResponsiveMask)
    {
        gfxContext.TransitionResource(g_ResponsiveMaskBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
        gfxContext.ClearColor(g_ResponsiveMaskBuffer);
    }

    if (ParticleEffectManager::Enable)
    {
        ParticleEffectManager::Render(gfxContext, m_Camera, g_SceneColorBuffer, g_SceneDepthBuffer, g_ResponsiveMaskBuffer, g_LinearDepth[FrameIndex], isUpdateResponsiveMask);
    }

    if (m_Technique == kDemoTech_XeSS)
    {
        if (XeSS::GetMotionVectorsMode() == XeSS::kMotionVectorsLowRes)
        {
            MotionBlur::GenerateCameraVelocityBuffer(gfxContext, m_Camera, true);
        }

        XeSS::Process(gfxContext, m_Camera);
    }
    else
    {
        MotionBlur::GenerateCameraVelocityBuffer(gfxContext, m_Camera, true);
        TemporalEffects::ResolveImage(gfxContext);
    }

    MotionBlur::RenderObjectBlur(gfxContext, g_VelocityBuffer);
}

void DemoApp::RenderUI(GraphicsContext& Context)
{
    ImGuiModule::Render(Context);
}

eDemoTechnique DemoApp::GetTechnique() const
{
    return m_Technique;
}

void DemoApp::SetTechnique(eDemoTechnique tech)
{
    if (m_Technique == tech)
        return;

    if (tech == kDemoTech_XeSS)
    {
        XeSS::SetEnabled(true);
        TemporalEffects::EnableTAA = false;
        XeSS::UpdateInputResolution();
    }
    else if (tech == kDemoTech_TAANative)
    {
        XeSS::SetEnabled(false);
        TemporalEffects::EnableTAA = true;
        Graphics::SetNativeResolutionAndUpscaling(g_DisplayWidth, g_DisplayHeight, false);
    }
    else if (tech == kDemoTech_TAAScaled)
    {
        XeSS::SetEnabled(false);
        TemporalEffects::EnableTAA = true;

        // We use same resolution as XeSS.
        uint32_t width, height;
        XeSS::GetInputResolution(width, height);
        Graphics::SetNativeResolutionAndUpscaling(width, height, false);
    }

    XeSSJitter::Reset();

    m_Technique = tech;
}

void DemoApp::LoadIBLTextures()
{
    TextureRef BRDFLUTTexture = TextureManager::LoadDDSFromFile(m_AssetRootDir + L"/Textures/BRDF_LUT.dds");
    Renderer::SetBRDFLUTTexture(BRDFLUTTexture);

    TextureRef diffuseTexture = TextureManager::LoadDDSFromFile(m_AssetRootDir + L"/Textures/Blouberg_diffuseIBL.dds");
    TextureRef specularTexture = TextureManager::LoadDDSFromFile(m_AssetRootDir + L"/Textures/Blouberg_specularIBL.dds");
    Renderer::SetIBLTextures(diffuseTexture, specularTexture);
}

CameraController* DemoApp::GetCameraController() const
{
    return m_CameraController.get();
}

DemoLog& DemoApp::GetLog()
{
    return m_Log;
}

bool DemoApp::FindAssetsDir()
{
    m_AssetRootDir = L"";

    const std::wstring& cwd = GetWorkingDirectory();
    
    // Search under current working directory.
    std::wstring assetDir = cwd + L"\\Assets";

    struct _stat64 dirStat;
    bool dirMissing = _wstat64(assetDir.c_str(), &dirStat) == -1;
    if (!dirMissing)
    {
        m_AssetRootDir = assetDir;
        return true;
    }

    // Search under SDK demo folder
    assetDir = cwd + L"\\..\\samples\\xess_demo\\Assets";
    dirMissing = _wstat64(assetDir.c_str(), &dirStat) == -1;
    if (!dirMissing)
    {
        m_AssetRootDir = assetDir;
        return true;
    }

    LOG_ERROR("Could not find Assets folder, please copy it from \"{SDK_DIR}/samples/xess_demo\".");
    MessageBox(GameCore::g_hWnd, L"Could not find Assets folder, please copy it from \"{SDK_DIR}/samples/xess_demo\".", 
        L"XeSS Demo: Assets missing", MB_OK);

    return false;
}
