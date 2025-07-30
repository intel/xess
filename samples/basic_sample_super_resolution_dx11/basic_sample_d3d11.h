//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// Copyright (C) 2024 Intel Corporation
//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

#include "DXSample.h"
#include "xess/xess_d3d11.h"

#include <chrono>
#include <vector>

using namespace DirectX;

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

class BasicSampleD3D11 : public DXSample
{
public:
    BasicSampleD3D11(UINT width, UINT height, std::wstring name);

    virtual void OnInit();
    virtual void OnUpdate();
    virtual void OnRender();
    virtual void OnDestroy();
    virtual void OnKeyUp(UINT8 key);

private:
    enum RTIndices
    {
        RT_Present = 0,
        RT_Color = 1,
        RT_Velocity = 2,
        RT_Depth = 3,
    };

    enum ViewIndices
    {
        VI_RTV = 0,
        VI_DSV = 1,
        VI_SRV = 2,
        VI_UAV = 3,
        MaxViewIndices = 4
    };

    static const UINT FrameCount = 2;
    static const UINT RTCount = 3;

    virtual void InitDx();
    virtual void InitXess();
    virtual void CreateFSQPipeline();

    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT4 color;
    };

    struct Resource
    {
        ComPtr<ID3D11Resource> resource;
        ComPtr<ID3D11View> view[MaxViewIndices];
    };

    struct D3D11PipelineState
    {
        void CreatePipelineState();
        void SetPipelineState(ID3D11DeviceContext * devContext)
        {
            devContext->VSSetShader(vertexShader.Get(), nullptr, 0);
            devContext->PSSetShader(pixelShader.Get(), nullptr, 0);
            devContext->IASetInputLayout(inputLayout.Get());
            devContext->RSSetState(rasterizerState.Get());
            devContext->OMSetBlendState(blendState.Get(), nullptr, 0xFFFFFFFF);
            devContext->OMSetDepthStencilState(depthStencilState.Get(), 0);
            devContext->PSSetSamplers(0, 1, samplerState.GetAddressOf());
        }

        ComPtr<ID3D11VertexShader> vertexShader;
        ComPtr<ID3D11PixelShader> pixelShader;
        ComPtr<ID3D11InputLayout> inputLayout;
        ComPtr<ID3D11RasterizerState> rasterizerState;
        ComPtr<ID3D11BlendState> blendState;
        ComPtr<ID3D11DepthStencilState> depthStencilState;
        ComPtr<ID3D11SamplerState> samplerState;
    };

    struct SceneConstantBuffer
    {
        XMFLOAT4 offset;  // X, Y - offset, Z, W - jitter
        XMFLOAT4 velocity;
        XMFLOAT4 resolution;
        float padding[52];  // Padding so the constant buffer is 256-byte aligned.
    };
    static_assert((sizeof(SceneConstantBuffer) % 256) == 0,
        "Constant Buffer size must be 256-byte aligned");

    // Pipeline objects.
    CD3D11_VIEWPORT m_viewport;
    CD3D11_RECT m_scissorRect;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D11Device> m_device;
    Resource m_renderTargets[FrameCount];
    Resource m_depthTargets[FrameCount];
    Resource m_renderTargetsVelocity[FrameCount];
    Resource m_presentRenderTargets[FrameCount];
    D3D11PipelineState m_pipelineStateColorPass_d3d11;
    D3D11PipelineState m_pipelineStateVelocityPass_d3d11;
    ComPtr<ID3D11DeviceContext> m_immediateContext;
    DXGI_FORMAT m_dsvFormat = DXGI_FORMAT_UNKNOWN;
    DXGI_FORMAT m_dsvTypedFormat = DXGI_FORMAT_UNKNOWN;

    D3D11PipelineState m_pipelineStateFSQPass_d3d11;

    const float m_clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    // App resources.
    ComPtr<ID3D11Buffer> m_vertexBuffer_d3d11;
    ComPtr<ID3D11Buffer> m_constantBuffer_d3d11;
    SceneConstantBuffer m_constantBufferData;
    UINT8* m_pCbvDataBegin;

    // Synchronization objects.
    UINT m_frameIndex;
    HANDLE m_fenceEvent = nullptr;
    UINT64 m_fenceValues[FrameCount];

    bool m_pause = false;

    // Jitter
    std::vector<std::pair<float, float>> m_haltonPointSet;
    std::size_t m_haltonIndex = 0;
    float jitter[2];

    // XeSS
    xess_context_handle_t m_xessContext = nullptr;
    xess_2d_t m_desiredOutputResolution;
    xess_2d_t m_renderResolution = {};
    Resource m_xessOutput[FrameCount];

    const xess_quality_settings_t m_quality = XESS_QUALITY_SETTING_PERFORMANCE;

    std::chrono::time_point<std::chrono::high_resolution_clock> last_time;
    std::chrono::time_point<std::chrono::high_resolution_clock> last_fps_time;

    uint32_t frame_counter = 0;

    void LoadPipeline();
    void LoadAssets();
    void PopulateCommandList();
    void ExecuteCommandList();
    void WaitForGpu();
    void MoveToNextFrame();

    void CompileFromFile(LPCWSTR path, const char* entryPoint, const char* shaderModel, UINT compileFlags, ComPtr<ID3DBlob>& shader);
};
