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

#define ENABLE_XELL 1
#ifdef ENABLE_XELL
#include "xell/xell_d3d12.h"
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
        DHI_Scene = 0,
        DHI_Color = 1,
        DHI_Velocity = 2,
    };

    enum RTIndices
    {
        RT_Present = 0,
        RT_Color = 1,
        RT_Velocity = 2,
        RT_Depth = 3,
    };

    static const UINT FrameCount = 4;
    static const UINT DescriptorsPerFrame = 5;
    static const uint32_t AppDescriptorCount = 64;
    static const UINT RTCount = 3;

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

    const float ClearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };

    // Pipeline objects.
    CD3DX12_VIEWPORT m_viewport;
    CD3DX12_RECT m_scissorRect;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    ComPtr<ID3D12Resource> m_depthTargets[FrameCount];
    ComPtr<ID3D12Resource> m_renderTargetsVelocity[FrameCount];
    ComPtr<ID3D12Resource> m_presentRenderTargets[FrameCount];
    ComPtr<ID3D12Resource> m_sapmlerFSQ;
    ComPtr<ID3D12CommandAllocator> m_commandAllocator[FrameCount];
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
    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValues[FrameCount];

#ifdef ENABLE_XELL
    // Xell
    xell_context_handle_t m_xellContext = nullptr;
    UINT32 m_frameCounter = 0;
    bool m_latencyReductionEnabled = false;
    void InitXell();
    void SetSleepMode();
#endif

    void LoadDX12();
    void LoadPipeline();
    void CreateFSQPipeline();
    void PopulateDescriptorHeap();
    void LoadAssets();
    void PopulateCommandList();
    void WaitForGpu();
    void MoveToNextFrame();
};
