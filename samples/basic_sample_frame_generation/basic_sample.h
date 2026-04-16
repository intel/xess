//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// Copyright (c) 2023 Intel Corporation
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

#include <xefg_swapchain_d3d12.h>
#include <xell_d3d12.h>

#include <atomic>
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

    void OnInit() override;
    void OnUpdate() override;
    void OnRender() override;
    void OnDestroy() override;
    void SetViewPort(UINT width, UINT height) override;
    void OnMouseWheel(WPARAM wParam) override;
    void OnKeyUp(UINT8 key) override;
    void OnSleep() override;

private:
    enum DescriptorHeapIndices
    {
        DHI_Color = 0,
        DHI_Velocity = 1,
        DHI_Interpolated = 2
    };

    enum RTIndices
    {
        RT_Present = 0,
        RT_Color = 1,
        RT_Velocity = 2,
        RT_Depth = 3,
        RT_Interpolated = 4,
        RT_COUNT
    };

    // Swap chain will have this many back buffers.
    static const UINT FrameCount = 4;
    static const UINT DescriptorsPerFrame = RT_COUNT;
    static const uint32_t AppDescriptorCount = 64;
    static const UINT RTCount = 4;

    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT4 color;
    };

    struct SceneConstantBuffer
    {
        XMFLOAT4 offset;
        XMFLOAT4 velocity;
        float padding[56];  // Padding so the constant buffer is 256-byte aligned.
    };
    static_assert((sizeof(SceneConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

    // Pipeline objects.
    CD3DX12_VIEWPORT m_viewport;
    CD3DX12_RECT m_scissorRect;
    ComPtr<IDXGISwapChain4> m_swapChain;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    ComPtr<ID3D12Resource> m_depthTargets[FrameCount];
    ComPtr<ID3D12Resource> m_renderTargetsVelocity[FrameCount];
    ComPtr<ID3D12Resource> m_presentRenderTargets[FrameCount];
    ComPtr<ID3D12Resource> m_interpolatedTargets[FrameCount];

    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12RootSignature> m_rootSignatureFSQ;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
    ComPtr<ID3D12DescriptorHeap> m_appDescriptorHeap;
    ComPtr<ID3D12PipelineState> m_pipelineStateColorPass;
    ComPtr<ID3D12PipelineState> m_pipelineStateVelocityPass;
    ComPtr<ID3D12PipelineState> m_pipelineStateFSQPass;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    DXGI_FORMAT m_dsvFormat;
    DXGI_FORMAT m_dsvTypedFormat;
    std::uint32_t m_outputIndex = DHI_Color;
    UINT m_rtvDescriptorSize;
    UINT m_dsvDescriptorSize;
    UINT m_uavDescriptorSize;
    const FLOAT m_clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };

    // App resources.
    ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    ComPtr<ID3D12Resource> m_constantBuffer;
    SceneConstantBuffer m_constantBufferData;
    UINT8* m_pCbvDataBegin;
    float m_verticalOffset = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_lastUpdateTime;
    bool m_pause = false;

    // Synchronization objects.
    UINT m_backBufferIndex;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue;

    DXGI_FORMAT m_format = DXGI_FORMAT_R8G8B8A8_UNORM;

    std::atomic<UINT> m_syncInterval = 0;

    bool m_enableXeFG = true;
    xefg_swapchain_handle_t m_xefgSwapChain = nullptr;
    xefg_swapchain_present_status_t m_lastPresentStatus = {};
    xell_context_handle_t m_xellContext = nullptr;

    // If your application uses frame latency waitable object,
    // the logic can remain active when switching to XeSS-FG proxy swap chain.
    HANDLE m_frameLatencyWaitableObject = nullptr;

    xefg_swapchain_properties_t m_xefgProperties = {};
    ComPtr<ID3D12DescriptorHeap> m_xefgDescriptorHeap;
    ComPtr<ID3D12Heap> m_fgTextureHeap;
    ComPtr<ID3D12Heap> m_fgBufferHeap;
    uint32_t m_xefgDescriptorCount = 0;

    UINT m_frameCounter = 0;

    // Currently selected number of interpolated frames.
    uint32_t m_numInterpolatedFrames = 0;

    xefg_swapchain_ui_composition_state_t m_uiCompositionState
        = XEFG_SWAPCHAIN_UI_COMPOSITION_STATE_DISABLED;

    void LoadDX12();
    void CreateFrameResources();
    void LoadPipeline();
    void CreateFSQPipeline();
    void PopulateDescriptorHeap();
    void LoadAssets();
    void PopulateCommandList();
    void PopulateRenderTargetCommandList();
    void WaitForExec();
    void WaitForPreviousFrame();

    void CycleNumInterpolatedFrames();
    void ToggleUiCompositionState();
    void ToggleVerticalSynchronization();

    static void LogCallback(const char* message, xefg_swapchain_logging_level_t /*level*/, void* userData)
    {
        const char* prefix = static_cast<const char*>(userData);
        DebugPrintf("%s %s\n", prefix, message);
    }
};
