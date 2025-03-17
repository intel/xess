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

#include <chrono>

#define ENABLE_XEFG_SWAPCHAIN 1

#ifdef ENABLE_XEFG_SWAPCHAIN
#include "xefg_swapchain_d3d12.h"
#include "xell_d3d12.h"
#endif

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
    virtual void SetViewPort(UINT width, UINT height);
    virtual void OnMouseWheel(WORD wValue);
    virtual void OnKeyUp(UINT8 key);
    virtual void OnSleep();

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
    ComPtr<ID3D12Resource> m_sapmlerFSQ;
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
    const FLOAT clearColor[4] = { 0.0, 0.2, 0.4, 1.0 };

    D3D12_SHADER_RESOURCE_VIEW_DESC blankResDesc;
    ComPtr<ID3D12Resource> blankRes = nullptr;
        
    // App resources.
    ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    ComPtr<ID3D12Resource> m_constantBuffer;
    SceneConstantBuffer m_constantBufferData;
    UINT8* m_pCbvDataBegin;
    float m_verticalOffset = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> last_time;
    bool m_pause = false;

    // Synchronization objects.
    UINT m_frameIndex;
    UINT m_backBufferIndex;
    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue;
    //That format is used for creating backbuffers, interpolation resources and views, so please check compliance before use
    //DXGI_FORMAT m_format = DXGI_FORMAT_R10G10B10A2_UNORM;
    DXGI_FORMAT m_format = DXGI_FORMAT_R8G8B8A8_UNORM; 

#define INTERPOLATION_COUNT 1

#ifdef ENABLE_XEFG_SWAPCHAIN
    // XeFG
    bool m_enableXeFG = true;
    xefg_swapchain_handle_t m_xefgSwapChain;
    xefg_swapchain_present_status_t lastPresentStatus;
    xell_context_handle_t m_xellContext;

    bool m_enableExternalDH =
#ifdef USE_EXTERNAL_DESCRIPTOR
        true;
#else
        false;
#endif
    xefg_swapchain_properties_t props;
    ComPtr<ID3D12DescriptorHeap> m_pDH;
    uint32_t m_descriptorCount = 0;
    uint32_t requiredDescriptorCount = 0;
#endif
    UINT m_frameCounter = 0;

    void LoadDX12();
    void CreateFrameResources();
    void LoadPipeline();
    void CreateFSQPipeline();
    void PopulateDescriptorHeap();
    void LoadAssets();
    void PopulateCommandList();
    void PopulateRenderTargetCommandList(uint32_t srcIndex);
    void WaitForExec();
    void WaitForPreviousFrame();
#ifdef ENABLE_XEFG_SWAPCHAIN
    static void LogCallback(const char* message, xefg_swapchain_logging_level_t level, void* userData)
    {
        OutputDebugStringA(("[XeFG Runtime][" + std::to_string(level) + "]: " + message + "\n").c_str());
    }
#endif

    // Busy wait for a specified duration
    void BusyWait(std::chrono::microseconds duration)
    {
        auto start = std::chrono::high_resolution_clock::now();
        while (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start) < duration)
        {
            // Do nothing
        }
    }
};
