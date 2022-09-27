//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

#include "DXSample.h"
#include "xess/xess_d3d12.h"

#include <chrono>

using namespace DirectX;

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

class BasicSample : public DXSample
{
public:
    BasicSample(UINT width, UINT height, std::wstring name);

    virtual void OnInit();
    virtual void OnUpdate();
    virtual void OnRender();
    virtual void OnDestroy();

    virtual void OnKeyUp(UINT8 key);

private:
    enum DescriptorHeapIndices
    {
        DHI_Scene = 0,
        DHI_Color = 1,
        DHI_Velocity = 2,
        DHI_Depth = 3,
        DHI_XeSSOutputUAV = 4,
        DHI_XeSSOutputSRV = 5
    };

    enum RTIndices
    {
        RT_Present = 0,
        RT_Color = 1,
        RT_Velocity = 2,
        RT_Depth = 3,
    };

    static const UINT DescriptorsPerFrame = 5;
    static const UINT FrameCount = 2;
    static const uint32_t AppDescriptorCount = 64;
    static const UINT RTCount = 3;

    virtual void InitDx();
    virtual void InitXess();
    virtual void CreateFSQPipeline();
    virtual void PopulateDescriptorHeap();

    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT4 color;
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
    CD3DX12_VIEWPORT m_viewport;
    CD3DX12_RECT m_scissorRect;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    ComPtr<ID3D12Resource> m_depthTargets[FrameCount];
    ComPtr<ID3D12Resource> m_renderTargetsVelocity[FrameCount];
    ComPtr<ID3D12Resource> m_presentRenderTargets[FrameCount];
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
    ComPtr<ID3D12DescriptorHeap> m_appDescriptorHeap;
    ComPtr<ID3D12PipelineState> m_pipelineStateColorPass;
    ComPtr<ID3D12PipelineState> m_pipelineStateVelocityPass;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    DXGI_FORMAT m_dsvFormat;
    DXGI_FORMAT m_dsvTypedFormat;

    std::uint32_t m_outputIndex = DHI_XeSSOutputSRV;
    UINT m_rtvDescriptorSize;
    UINT m_dsvDescriptorSize;
    UINT m_uavDescriptorSize;

    ComPtr<ID3D12RootSignature> m_rootSignatureFSQ;
    ComPtr<ID3D12PipelineState> m_pipelineStateFSQPass;
    ComPtr<ID3D12Resource> m_sapmlerFSQ;

    const float m_clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    // App resources.
    ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    ComPtr<ID3D12Resource> m_constantBuffer;
    SceneConstantBuffer m_constantBufferData;
    UINT8* m_pCbvDataBegin;

    // Synchronization objects.
    UINT m_frameIndex;
    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue;

    bool m_pause = false;

    // Jitter
    std::vector<std::pair<float, float>> m_haltonPointSet;
    std::size_t m_haltonIndex = 0;
    float jitter[2];

    // XeSS
    xess_context_handle_t m_xessContext = nullptr;
    xess_2d_t m_desiredOutputResolution;
    xess_2d_t m_renderResolution;
    ComPtr<ID3D12Heap> m_texturesHeap;
    ComPtr<ID3D12Resource> m_xessOutput[FrameCount];
    const xess_quality_settings_t m_quality = XESS_QUALITY_SETTING_PERFORMANCE;

    std::chrono::time_point<std::chrono::high_resolution_clock> last_time;

    void LoadPipeline();
    void LoadAssets();
    void PopulateCommandList();
    void WaitForPreviousFrame();
};
