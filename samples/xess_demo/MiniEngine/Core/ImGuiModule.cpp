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

#include "ImGuiModule.h"
#include "ColorBuffer.h"
#include "BufferManager.h"
#include "CommandContext.h"
#include "GraphicsCore.h"
#include "Utility.h"
#include "Display.h"
#include "PipelineState.h"
#include "Math/Matrix4.h"
#include "CompiledShaders/ImGuiVS.h"
#include "CompiledShaders/ImGuiPS.h"

#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_win32.h"
#include "imgui.h"

namespace GameCore
{
    extern HWND g_hWnd;
}

using namespace Math;
using namespace Graphics;
using namespace GameCore;

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace ImGuiModule
{
    bool g_Initialized = false;

    // Image type to display in ImGui
    enum ImImageType
    {
        ImImage_Texture = 0,
        ImImage_ColorBuffer
    };

    struct FontInfo
    {
        std::string Path;
        float Size;
        bool Merge;
        ImFont* Font;
    };

    // This lookup table is used to tell which type of GpuResource the image is.
    std::map<GpuResource*, ImImageType> g_AllImageLUT;

    SamplerDesc s_SamplerDesc;
    RootSignature s_RootSignature;
    GraphicsPSO s_PSO(L"ImGui PSO");
    Texture g_FontTexture;
    ByteAddressBuffer g_VertexBuffer;
    ByteAddressBuffer g_IndexBuffer;

    const uint32_t VERTEX_BUFFER_INCREMENT = 5000 * sizeof(ImDrawVert);
    const uint32_t INDEX_BUFFER_INCREMENT = 10000 * sizeof(ImDrawIdx);

    std::vector<uint8_t> g_VertexBufferShadowData;
    std::vector<uint8_t> g_IndexBufferShadowData;

    Matrix4 g_ProjMatrix;

    std::vector<FontInfo> g_AllFont;

    bool g_FontsDirty = false;
    float g_DPIScale = 1.0f;

    void CreatePipeline();
    void PrepareFontTexture();
    void PrepareProjection(const ImDrawData& DrawData);
    void PrepareGeometry(GraphicsContext& Context, const ImDrawData& DrawData);
    void ApplyFontsChange();
} // namespace ImGuiModule

bool ImGuiModule::Initialize()
{
    ASSERT(g_hWnd);
    if (!g_hWnd)
        return false;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    if (!ImGui_ImplWin32_Init(g_hWnd))
    {
        LOG_ERROR("Could not initialize ImGui Renderer.");
        return false;
    }

    CreatePipeline();

    LOG_INFO("ImGui Renderer Initialized.");

    g_Initialized = true;

    return true;
}

bool ImGuiModule::IsIntialized()
{
    return g_Initialized;
}

void ImGuiModule::Shutdown()
{
    if (!g_Initialized)
        return;

    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    g_FontTexture.Destroy();
    g_VertexBuffer.Destroy();
    g_IndexBuffer.Destroy();

    g_Initialized = false;

    LOG_INFO("ImGui Renderer Shutdown.");
}

uint32_t ImGuiModule::AddFont(const std::wstring& FontPath, float Size, bool Merge)
{
    std::string utf8Path = Utility::WideStringToUTF8(FontPath);

    FontInfo info = { utf8Path, Size, Merge, nullptr };
    g_AllFont.push_back(info);

    g_FontsDirty = true;

    return static_cast<uint32_t>(g_AllFont.size() - 1);
}

ImFont* ImGuiModule::GetFont(uint32_t Index)
{
    ASSERT(Index < g_AllFont.size());
    if (Index >= g_AllFont.size())
        return nullptr;

    return g_AllFont[Index].Font;
}

void ImGuiModule::PrepareFontTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    UINT pitchBytes = (width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);

    g_FontTexture.Create2D(pitchBytes, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, pixels);

    io.Fonts->SetTexID(&g_FontTexture);
    io.Fonts->ClearTexData();
}

void ImGuiModule::PrepareGeometry(GraphicsContext& Context, const ImDrawData& DrawData)
{
    ScopedTimer _prof(L"ImGui Prepare Geometry", Context);

    const size_t totalVertexSize = Math::AlignUp(DrawData.TotalVtxCount * sizeof(ImDrawVert), 16);
    const size_t totalIndexSize = Math::AlignUp(DrawData.TotalIdxCount * sizeof(ImDrawIdx), 16);

    if (g_VertexBuffer.GetBufferSize() < totalVertexSize
        || g_IndexBuffer.GetBufferSize() < totalIndexSize)
    {
        g_CommandManager.IdleGPU();
    }

    if (g_VertexBuffer.GetBufferSize() < totalVertexSize)
    {
        g_VertexBuffer.Create(L"ImGui Vertex Buffer",
            static_cast<uint32_t>(Math::AlignUp(totalVertexSize + VERTEX_BUFFER_INCREMENT, 16)), 1);
    }

    if (g_IndexBuffer.GetBufferSize() < totalIndexSize)
    {
        g_IndexBuffer.Create(L"ImGui Index Buffer",
            static_cast<uint32_t>(Math::AlignUp(totalIndexSize + INDEX_BUFFER_INCREMENT, 16)), 1);
    }

    g_VertexBufferShadowData.resize(totalVertexSize);
    g_IndexBufferShadowData.resize(totalIndexSize);

    ImDrawVert* vbPtr = reinterpret_cast<ImDrawVert*>(g_VertexBufferShadowData.data());
    ImDrawIdx* ibPtr = reinterpret_cast<ImDrawIdx*>(g_IndexBufferShadowData.data());

    for (int i = 0; i < DrawData.CmdListsCount; i++)
    {
        const ImDrawList* drawList = DrawData.CmdLists[i];

        memcpy(vbPtr, drawList->VtxBuffer.Data, drawList->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(ibPtr, drawList->IdxBuffer.Data, drawList->IdxBuffer.Size * sizeof(ImDrawIdx));

        vbPtr += drawList->VtxBuffer.Size;
        ibPtr += drawList->IdxBuffer.Size;
    }

    Context.WriteBuffer(g_VertexBuffer, 0, g_VertexBufferShadowData.data(), totalVertexSize);
    Context.WriteBuffer(g_IndexBuffer, 0, g_IndexBufferShadowData.data(), totalIndexSize);
}

void ImGuiModule::CreatePipeline()
{
    // ImGui should be extended to support different texture samplings.
    //s_SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    s_SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    s_SamplerDesc.MaxAnisotropy = 0;
    s_SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    s_SamplerDesc.SetBorderColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
    s_SamplerDesc.MaxLOD = 0.0f;

    s_RootSignature.Reset(2, 1);
    s_RootSignature.InitStaticSampler(0, s_SamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);
    s_RootSignature[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX);
    s_RootSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
    s_RootSignature.Finalize(L"ImGui Renderer", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    D3D12_INPUT_ELEMENT_DESC vertElem[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, (UINT)IM_OFFSETOF(ImDrawVert, pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, (UINT)IM_OFFSETOF(ImDrawVert, uv), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)IM_OFFSETOF(ImDrawVert, col), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    s_PSO.SetRootSignature(s_RootSignature);
    s_PSO.SetRasterizerState(Graphics::RasterizerTwoSided);
    s_PSO.SetBlendState(Graphics::BlendTraditional);
    s_PSO.SetDepthStencilState(Graphics::DepthStateDisabled);
    s_PSO.SetInputLayout(_countof(vertElem), vertElem);
    s_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    s_PSO.SetVertexShader(g_pImGuiVS, sizeof(g_pImGuiVS));
    s_PSO.SetPixelShader(g_pImGuiPS, sizeof(g_pImGuiPS));
    s_PSO.SetRenderTargetFormats(1, &g_OverlayBuffer.GetFormat(), DXGI_FORMAT_UNKNOWN);
    s_PSO.Finalize();
}

void ImGuiModule::ApplyFontsChange()
{
    float scale = Display::GetDPIScale();
    LOG_DEBUGF("ImGuiModule: Update font, DPI scale=%f", scale);

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(scale / g_DPIScale);

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    for (unsigned i = 0; i < g_AllFont.size(); ++i)
    {
        FontInfo& fontInfo = g_AllFont[i];

        ImFontConfig config;
        config.MergeMode = fontInfo.Merge;
        config.FontDataOwnedByAtlas = false;

        ImFont* font = io.Fonts->AddFontFromFileTTF(fontInfo.Path.c_str(), fontInfo.Size * scale, &config, nullptr);
        ASSERT(font);
        if (!font)
            continue;

        g_AllFont[i].Font = font;
    }

    io.Fonts->Build();

    PrepareFontTexture();
    g_DPIScale = scale;
    g_FontsDirty = false;
}

void ImGuiModule::PrepareProjection(const ImDrawData& DrawData)
{
    float L = DrawData.DisplayPos.x;
    float R = DrawData.DisplayPos.x + DrawData.DisplaySize.x;
    float T = DrawData.DisplayPos.y;
    float B = DrawData.DisplayPos.y + DrawData.DisplaySize.y;

    g_ProjMatrix = Matrix4(
        Vector4(2.0f / (R - L), 0.0f, 0.0f, 0.0f),
        Vector4(0.0f, 2.0f / (T - B), 0.0f, 0.0f),
        Vector4(0.0f, 0.0f, 0.5f, 0.0f),
        Vector4((R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f));
}

void ImGuiModule::PreGUI()
{
    if (!g_Initialized)
        return;

    if (g_DPIScale != Display::GetDPIScale())
    {
        g_FontsDirty = true; // if display DPI changed, mark font dirty.
    }

    if (g_FontsDirty)
    {
        ApplyFontsChange();
    }

    ImGui_ImplWin32_NewFrame();

    g_AllImageLUT.clear();
    g_AllImageLUT[&g_FontTexture] = ImImage_Texture;

    ImGui::NewFrame();
}

void ImGuiModule::PostGUI()
{
    if (!g_Initialized)
        return;

    ImGui::Render();
}

void ImGuiModule::Render(GraphicsContext& Context)
{
    if (!g_Initialized)
        return;

    ScopedTimer _prof(L"ImGui Render", Context);

    ImDrawData* drawData = ImGui::GetDrawData();
    if (!drawData || !drawData->CmdListsCount)
        return;

    if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)
        return;

    PrepareGeometry(Context, *drawData);

    PrepareProjection(*drawData);

    {
        ScopedTimer _prof_draw(L"ImGui Draw", Context);

        Context.SetRootSignature(s_RootSignature);
        Context.SetPipelineState(s_PSO);
        Context.TransitionResource(g_VertexBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        Context.TransitionResource(g_IndexBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER);
        Context.SetVertexBuffer(0, g_VertexBuffer.VertexBufferView(0, drawData->TotalVtxCount * sizeof(ImDrawVert), sizeof(ImDrawVert)));
        Context.SetIndexBuffer(g_IndexBuffer.IndexBufferView());

        Context.SetDynamicConstantBufferView(0, sizeof(g_ProjMatrix), &g_ProjMatrix);
        Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        int vertexOffset = 0;
        int indexOffset = 0;
        ImVec2 clip_off = drawData->DisplayPos;
        for (int i = 0; i < drawData->CmdListsCount; i++)
        {
            const ImDrawList* drawList = drawData->CmdLists[i];
            for (int j = 0; j < drawList->CmdBuffer.Size; j++)
            {
                const ImDrawCmd* cmd = &drawList->CmdBuffer[j];
                if (cmd->UserCallback)
                {
                    cmd->UserCallback(drawList, cmd);
                }
                else
                {
                    ImVec2 minClip(cmd->ClipRect.x - clip_off.x, cmd->ClipRect.y - clip_off.y);
                    ImVec2 maxClip(cmd->ClipRect.z - clip_off.x, cmd->ClipRect.w - clip_off.y);
                    if (maxClip.x <= minClip.x || maxClip.y <= minClip.y)
                        continue;

                    const D3D12_RECT r = { (LONG)minClip.x, (LONG)minClip.y, (LONG)maxClip.x, (LONG)maxClip.y };
                    Context.SetScissor(r);

                    ImTextureID texId = cmd->GetTexID();
                    GpuResource* resource = static_cast<GpuResource*>(texId);
                    ASSERT(resource);
                    if (resource)
                    {
                        // Texture and ColorBuffer need a common GetSRV interface.
                        // Here, a lookup table is used to to tell the type of the resource.
                        auto iter = g_AllImageLUT.find(resource);
                        if (iter == g_AllImageLUT.end())
                            continue;

                        if (iter->second == ImImage_Texture)
                        {
                            Context.SetDynamicDescriptor(1, 0, static_cast<Texture*>(resource)->GetSRV());
                        }
                        else
                        {
                            Context.SetDynamicDescriptor(1, 0, static_cast<ColorBuffer*>(resource)->GetSRV());
                        }

                        Context.TransitionResource(*resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                        Context.DrawIndexedInstanced(cmd->ElemCount, 1, cmd->IdxOffset + indexOffset, cmd->VtxOffset + vertexOffset, 0);
                    }
                }
            }

            indexOffset += drawList->IdxBuffer.Size;
            vertexOffset += drawList->VtxBuffer.Size;
        }
    }
}

void ImGuiModule::RegisterImage(ColorBuffer& image)
{
    g_AllImageLUT[&image] = ImImage_ColorBuffer;
}

bool ImGuiModule::WantCaptureKeyboard()
{
    if (!g_Initialized)
        return false;

    return ImGui::GetIO().WantCaptureKeyboard;
}

bool ImGuiModule::WantCaptureMouse()
{
    if (!g_Initialized)
        return false;

    return ImGui::GetIO().WantCaptureMouse;
}

void ImGuiModule::RegisterImage(Texture& image)
{
    g_AllImageLUT[&image] = ImImage_Texture;
}

LRESULT ImGuiModule::WinProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (!g_Initialized)
        return 0;

    return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
}
