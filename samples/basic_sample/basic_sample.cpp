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

#include "basic_sample.h"

#include <iostream>

#include "stdafx.h"
#include "utils.h"
#include "xess/xess_d3d12.h"
#include "xess/xess_debug.h"

//#define USE_LOWRES_MV 1

void BasicSample::OnKeyUp(UINT8 key)
{
    switch (key)
    {
    case 0x31:  // Key 1
        m_outputIndex = DHI_Color;
        break;
    case 0x32:  // Key 2
        m_outputIndex = DHI_Velocity;
        break;
    case 0x33:  // Key 3
        m_outputIndex = DHI_XeSSOutputSRV;
        break;
    case 0x34:  // Key 4
    {
        xess_dump_parameters_t dump_params{"dump", 0, 200, XESS_DUMP_ALL_INPUTS};
        xessStartDump(m_xessContext, &dump_params);
        break;
    }
    case VK_SPACE:
        m_pause = !m_pause;
        break;
    }
}

BasicSample::BasicSample(UINT width, UINT height, std::wstring name)
    : DXSample(width, height, name)
    , m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height))
    , m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height))
    , m_rtvDescriptorSize(0)
    , m_constantBufferData{}
    , m_pCbvDataBegin(nullptr)
    , m_frameIndex(0)
    , m_desiredOutputResolution{width, height}

{
    m_haltonPointSet = Utils::GenerateHalton(2, 3, 1, 32);
}

void BasicSample::OnInit()
{
    InitDx();
    InitXess();
    LoadPipeline();
    LoadAssets();
    CreateFSQPipeline();
    PopulateDescriptorHeap();
}

void BasicSample::InitDx()
{
    UINT dxgiFactoryFlags = 0;

#ifdef ENABLE_DX_DEBUG_LAYER
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            // Enable additional debug layers.
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    if (m_useWarpDevice)
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

        ThrowIfFailed(
            D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));
    } else
    {
        ComPtr<IDXGIAdapter1> hardwareAdapter;
        GetHardwareAdapter(factory.Get(), &hardwareAdapter, true);

        ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));
    }

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),  // Swap chain needs the queue so that it can force a flush on it.
        Win32Application::GetHwnd(), &swapChainDesc, nullptr, nullptr, &swapChain));

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(
        factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain.As(&m_swapChain));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        AppDescriptorCount, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0};

    m_device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&m_appDescriptorHeap));
    m_appDescriptorHeap->SetName(L"app_descriptor_heap");
}

void BasicSample::InitXess()
{
    auto status = xessD3D12CreateContext(m_device.Get(), &m_xessContext);
    if (status != XESS_RESULT_SUCCESS)
    {
        throw std::runtime_error("Unable to create XeSS context");
    }

    if (XESS_RESULT_WARNING_OLD_DRIVER == xessIsOptimalDriver(m_xessContext))
    {
        MessageBox(NULL, L"Please install the latest graphics driver from your vendor for optimal Intel(R) XeSS performance and visual quality", L"Important notice", MB_OK | MB_TOPMOST);
    }

    xess_properties_t props;
    status = xessGetProperties(m_xessContext, &m_desiredOutputResolution, &props);
    if (status != XESS_RESULT_SUCCESS)
    {
        throw std::runtime_error("Unable to get XeSS props");
    }

    xess_version_t xefx_version;
    status = xessGetIntelXeFXVersion(m_xessContext, &xefx_version);
    if (status != XESS_RESULT_SUCCESS)
    {
        throw std::runtime_error("Unable to get XeFX version");
    }

    D3D12_HEAP_DESC textures_heap_desc{props.tempTextureHeapSize,
        {D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0},
        0, D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES};

    m_device->CreateHeap(&textures_heap_desc, IID_PPV_ARGS(&m_texturesHeap));
    m_texturesHeap->SetName(L"xess_textures_heap");

    m_uavDescriptorSize =
        m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    xess_d3d12_init_params_t params = {
        /* Output width and height. */
        m_desiredOutputResolution,
        /* Quality setting */
        m_quality,
        /* Initialization flags. */
        #if defined(USE_LOWRES_MV)
        XESS_INIT_FLAG_NONE,
        #else
        XESS_INIT_FLAG_HIGH_RES_MV,
        #endif
        /* Specfies the node mask for internally created resources on
         * multi-adapter systems. */
        0,
        /* Specfies the node visibility mask for internally created resources
         * on multi-adapter systems. */
        0,
        /* Optional externally allocated buffers storage for X<sup>e</sup>SS. If NULL the
         * storage is allocated internally. If allocated, the heap type must be
         * D3D12_HEAP_TYPE_DEFAULT. This heap is not accessed by the CPU. */
        nullptr,
        /* Offset in the externally allocated heap for temporary buffers storage. */
        0,
        /* Optional externally allocated textures storage for X<sup>e</sup>SS. If NULL the
         * storage is allocated internally. If allocated, the heap type must be
         * D3D12_HEAP_TYPE_DEFAULT. This heap is not accessed by the CPU. */
        m_texturesHeap.Get(),
        /* Offset in the externally allocated heap for temporary textures storage. */
        0,
        /* No pipeline library */
        NULL
    };

    status = xessD3D12Init(m_xessContext, &params);
    if (status != XESS_RESULT_SUCCESS)
    {
        throw std::runtime_error("Unable to get XeSS props");
    }

    // Get optimal input resolution
    status = xessGetInputResolution(
        m_xessContext, &m_desiredOutputResolution, m_quality, &m_renderResolution);
    if (status != XESS_RESULT_SUCCESS)
    {
        throw std::runtime_error("Unable to get XeSS props");
    }
}

// Load the rendering pipeline dependencies.
void BasicSample::LoadPipeline()
{
    // Create descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = FrameCount * RTCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

        m_rtvDescriptorSize =
            m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }
    {
        // Describe and create a depth stencil view (DSV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = FrameCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

        m_dsvDescriptorSize =
            m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    }

    constexpr int depth_case = 2; // used to test various depth formats with xess*Dump calls.
    switch (depth_case)
    {
    case 0:
        m_dsvFormat = DXGI_FORMAT_D32_FLOAT;
        m_dsvTypedFormat = DXGI_FORMAT_D32_FLOAT;
        break;
    case 1:
        m_dsvFormat = DXGI_FORMAT_R32_TYPELESS;
        m_dsvTypedFormat = DXGI_FORMAT_D32_FLOAT;
        break;
    case 2:
        m_dsvFormat = DXGI_FORMAT_R32G8X24_TYPELESS;
        m_dsvTypedFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        break;
    case 3:
        m_dsvFormat = DXGI_FORMAT_R24G8_TYPELESS;
        m_dsvTypedFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        break;
    case 4:
        m_dsvFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        m_dsvTypedFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        break;
    case 5:
        m_dsvFormat = DXGI_FORMAT_R16_TYPELESS;
        m_dsvTypedFormat = DXGI_FORMAT_D16_UNORM;
        break;
    case 6:
        m_dsvFormat = DXGI_FORMAT_R16_UINT;
        m_dsvTypedFormat = DXGI_FORMAT_D16_UNORM;
        break;
    default:
        throw std::runtime_error("Please select depth format case!");
        break;
    }

    // Create frame resources.
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

        // Create render targets and xes output for each frame
        for (UINT n = 0; n < FrameCount; n++)
        {
            D3D12_HEAP_PROPERTIES heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            DXGI_FORMAT fmt = DXGI_FORMAT_R16G16B16A16_FLOAT;
            D3D12_RESOURCE_DESC tex_desc =
                CD3DX12_RESOURCE_DESC::Tex2D(fmt, m_renderResolution.x, m_renderResolution.y);
            tex_desc.MipLevels = 1;

            // Get swapchain
            ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_presentRenderTargets[n])));

            D3D12_CLEAR_VALUE clear_value;
            clear_value.Color[0] = m_clearColor[0];
            clear_value.Color[1] = m_clearColor[1];
            clear_value.Color[2] = m_clearColor[2];
            clear_value.Color[3] = m_clearColor[3];
            clear_value.Format = fmt;

            // Render target
            tex_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            ThrowIfFailed(m_device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE,
                &tex_desc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, &clear_value,
                IID_PPV_ARGS(&m_renderTargets[n])));

            tex_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            tex_desc.Format = m_dsvFormat;
            ThrowIfFailed(m_device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE,
                &tex_desc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, nullptr,
                IID_PPV_ARGS(&m_depthTargets[n])));

            // XeSS output
            tex_desc = CD3DX12_RESOURCE_DESC::Tex2D(
                fmt, m_desiredOutputResolution.x, m_desiredOutputResolution.y);
            tex_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            ThrowIfFailed(m_device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE,
                &tex_desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
                IID_PPV_ARGS(&m_xessOutput[n])));

            // Velocity
            fmt = DXGI_FORMAT_R16G16_FLOAT;
            tex_desc = CD3DX12_RESOURCE_DESC::Tex2D(
                fmt, m_desiredOutputResolution.x, m_desiredOutputResolution.y);
            tex_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            clear_value.Format = fmt;
            ThrowIfFailed(m_device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE,
                &tex_desc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, &clear_value,
                IID_PPV_ARGS(&m_renderTargetsVelocity[n])));

            // Create RTVs
            // RT_Present
            m_device->CreateRenderTargetView(m_presentRenderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescriptorSize);

            // RT_Color
            m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescriptorSize);

            // RT_Velocity
            m_device->CreateRenderTargetView(m_renderTargetsVelocity[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescriptorSize);

            // RT_Depth
            auto dsv_desc = D3D12_DEPTH_STENCIL_VIEW_DESC{};
            dsv_desc.Format = m_dsvTypedFormat;
            dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
            dsv_desc.Texture2D = D3D12_TEX2D_DSV{};
            dsv_desc.Texture2D.MipSlice = 0;
            m_device->CreateDepthStencilView(m_depthTargets[n].Get(), &dsv_desc, dsvHandle);
            dsvHandle.Offset(1, m_dsvDescriptorSize);

            m_presentRenderTargets[n]->SetName(
                (std::wstring(L"Present") + std::to_wstring(n)).c_str());
            m_renderTargets[n]->SetName((std::wstring(L"Color") + std::to_wstring(n)).c_str());
            m_depthTargets[n]->SetName((std::wstring(L"Depth") + std::to_wstring(n)).c_str());
            m_xessOutput[n]->SetName((std::wstring(L"XeSSOutput") + std::to_wstring(n)).c_str());
            m_renderTargetsVelocity[n]->SetName(
                (std::wstring(L"Velocity") + std::to_wstring(n)).c_str());
        }
    }

    ThrowIfFailed(m_device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
}

// Load the sample assets.
void BasicSample::LoadAssets()
{
    // Create a root signature consisting of a descriptor table with a single CBV.
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

        // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the
        // HighestVersion returned will not be greater than this.
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if (FAILED(m_device->CheckFeatureSupport(
                D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
        CD3DX12_ROOT_PARAMETER1 rootParameters[1];

        ranges[0].Init(
            D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);

        // Allow input layout and deny uneccessary access to certain pipeline stages.
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(
            _countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3DX12SerializeVersionedRootSignature(
            &rootSignatureDesc, featureData.HighestVersion, &signature, &error));
        ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(),
            signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
    }

    // Create the pipeline state, which includes compiling and loading shaders.
    {
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif

        ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr,
            nullptr, "VSMainColor", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
        ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr,
            nullptr, "PSMainColor", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

        // Define the vertex input layout.
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = {inputElementDescs, _countof(inputElementDescs)};
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        psoDesc.DepthStencilState.StencilEnable = TRUE;
        psoDesc.DepthStencilState.FrontFace = psoDesc.DepthStencilState.BackFace =
            D3D12_DEPTH_STENCILOP_DESC{D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP,
                D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS};
        psoDesc.DepthStencilState.StencilReadMask = (UINT8)0xff;
        psoDesc.DepthStencilState.StencilWriteMask = (UINT8)0xff;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
        psoDesc.DSVFormat = m_dsvTypedFormat;
        psoDesc.SampleDesc.Count = 1;

        ThrowIfFailed(m_device->CreateGraphicsPipelineState(
            &psoDesc, IID_PPV_ARGS(&m_pipelineStateColorPass)));
    }

    // Create velocity pass
    {
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif

        ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr,
            nullptr, "VSMainVelocity", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
        ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr,
            nullptr, "PSMainVelocity", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

        // Define the vertex input layout.
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = {inputElementDescs, _countof(inputElementDescs)};
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16_FLOAT;
        psoDesc.SampleDesc.Count = 1;

        ThrowIfFailed(m_device->CreateGraphicsPipelineState(
            &psoDesc, IID_PPV_ARGS(&m_pipelineStateVelocityPass)));
    }

    // Create the command list.
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_commandAllocator.Get(), m_pipelineStateColorPass.Get(), IID_PPV_ARGS(&m_commandList)));

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    ThrowIfFailed(m_commandList->Close());

    // Create the vertex buffer.
    {
        // Define the geometry for a triangle.
        Vertex triangleVertices[] = {
            {{0.0f, 0.25f * m_aspectRatio, 0.01f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{0.25f, -0.25f * m_aspectRatio, 0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
            {{-0.25f, -0.25f * m_aspectRatio, 0.2f}, {0.0f, 0.0f, 1.0f, 1.0f}}};

        const UINT vertexBufferSize = sizeof(triangleVertices);

        // Note: using upload heaps to transfer static data like vert buffers is not
        // recommended. Every time the GPU needs it, the upload heap will be marshalled
        // over. Please read up on Default Heap usage. An upload heap is used here for
        // code simplicity and because there are very few verts to actually transfer.
        auto heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto resource_desc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
        ThrowIfFailed(
            m_device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &resource_desc,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_vertexBuffer)));

        // Copy the triangle data to the vertex buffer.
        UINT8* pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0);  // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(
            m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
        m_vertexBuffer->Unmap(0, nullptr);

        // Initialize the vertex buffer view.
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(Vertex);
        m_vertexBufferView.SizeInBytes = vertexBufferSize;
    }

    // Create the constant buffer.
    {
        const UINT constantBufferSize =
            sizeof(SceneConstantBuffer);  // CB size is required to be 256-byte aligned.

        auto heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto resource_desc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);
        ThrowIfFailed(
            m_device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &resource_desc,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_constantBuffer)));

        // Describe and create a constant buffer view.
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = constantBufferSize;
        m_device->CreateConstantBufferView(
            &cbvDesc, m_appDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

        // Map and initialize the constant buffer. We don't unmap this until the
        // app closes. Keeping things mapped for the lifetime of the resource is okay.
        CD3DX12_RANGE readRange(0, 0);  // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(
            m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin)));
        memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));
    }

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fenceValue = 1;

        // Create an event handle to use for frame synchronization.
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        // Wait for the command list to execute; we are reusing the same command
        // list in our main loop but for now, we just want to wait for setup to
        // complete before continuing.
        WaitForPreviousFrame();
    }
}

void BasicSample::PopulateDescriptorHeap()
{
    auto addTexture =
        [&](std::uint32_t index, ID3D12Resource* resource, DXGI_FORMAT fmt, bool is_uav = false)
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescHandle(
            m_appDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), (INT)index, m_uavDescriptorSize);

        if (is_uav)
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};

            uavDesc.Format = fmt;
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice = 0;
            uavDesc.Texture2D.PlaneSlice = 0;

            m_device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, cpuDescHandle);
        } else
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Format = fmt;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.PlaneSlice = 0;
            m_device->CreateShaderResourceView(resource, &srvDesc, cpuDescHandle);
        }
    };

    for (UINT n = 0; n < FrameCount; ++n)
    {
        addTexture(DescriptorsPerFrame * n + DHI_Color, m_renderTargets[n].Get(),
            DXGI_FORMAT_R16G16B16A16_FLOAT);
        addTexture(DescriptorsPerFrame * n + DHI_Velocity,
            m_renderTargetsVelocity[n].Get(), DXGI_FORMAT_R16G16_FLOAT);
        addTexture(DescriptorsPerFrame * n + DHI_XeSSOutputUAV, m_xessOutput[n].Get(),
            DXGI_FORMAT_R16G16B16A16_FLOAT, true);
        addTexture(DescriptorsPerFrame * n + DHI_XeSSOutputSRV, m_xessOutput[n].Get(),
            DXGI_FORMAT_R16G16B16A16_FLOAT);
    }
}

void BasicSample::CreateFSQPipeline()
{
    // Create a root signature
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

        // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the
        // HighestVersion returned will not be greater than this.
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if (FAILED(m_device->CheckFeatureSupport(
                D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
        ranges[0].Init(
            D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

        CD3DX12_ROOT_PARAMETER1 rootParameters[1];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.MipLODBias = 0;
        sampler.MaxAnisotropy = 0;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler,
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3DX12SerializeVersionedRootSignature(
            &rootSignatureDesc, featureData.HighestVersion, &signature, &error));
        ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(),
            signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignatureFSQ)));
    }

    // Create the pipeline state, which includes compiling and loading shaders.
    {
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif

        ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr,
            nullptr, "VSMainFSQ", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
        ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr,
            nullptr, "PSMainFSQ", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = {};
        psoDesc.pRootSignature = m_rootSignatureFSQ.Get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;

        ThrowIfFailed(
            m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateFSQPass)));
    }
}

// Update frame-based values.
void BasicSample::OnUpdate()
{
    if (last_time.time_since_epoch().count() == 0)
    {
        last_time = std::chrono::high_resolution_clock::now();
    }

    auto current_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = current_time - last_time;
    last_time = current_time;

    const double speed = 1. / 2; //4 second for screen
    float translationSpeed = m_pause ? 0.f : (float)(speed * elapsed_seconds.count());
    const float offsetBounds = 1.25f;

    m_constantBufferData.offset.x += translationSpeed;
    if (m_constantBufferData.offset.x > offsetBounds)
    {
        m_constantBufferData.offset.x = -offsetBounds;
    }

    auto haltonValue = m_haltonPointSet[m_haltonIndex];
    m_haltonIndex = (m_haltonIndex + 1) % m_haltonPointSet.size();

    jitter[0] = haltonValue.first;
    jitter[1] = haltonValue.second;

    m_constantBufferData.offset.z = jitter[0];
    m_constantBufferData.offset.w = jitter[1];

    m_constantBufferData.resolution.x = (float)m_renderResolution.x;
    m_constantBufferData.resolution.y = (float)m_renderResolution.y;

    m_constantBufferData.velocity.x = -translationSpeed * ((float)m_desiredOutputResolution.x / 2.f);

    memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));
}

// Render the scene.
void BasicSample::OnRender()
{
    // Record all the commands we need to render the scene into the command list.
    PopulateCommandList();

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = {m_commandList.Get()};
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present the frame.
    ThrowIfFailed(m_swapChain->Present(1, 0));

    WaitForPreviousFrame();
}

void BasicSample::OnDestroy()
{
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    WaitForPreviousFrame();

    auto status = xessDestroyContext(m_xessContext);
    if (status != XESS_RESULT_SUCCESS)
    {
        throw std::runtime_error("Unable to destroy XeSS context");
    }

    CloseHandle(m_fenceEvent);
}

// Fill the command list with all the render commands and dependent state.
void BasicSample::PopulateCommandList()
{
    // Command list allocators can only be reset when the associated
    // command lists have finished execution on the GPU; apps should use
    // fences to determine GPU execution progress.
    ThrowIfFailed(m_commandAllocator->Reset());

    // However, when ExecuteCommandList() is called on a particular command
    // list, that command list can then be reset at any time and must be before
    // re-recording.
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineStateColorPass.Get()));

    // Run Color pass
    {
        // Set necessary state.
        m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

        ID3D12DescriptorHeap* ppHeaps[] = {m_appDescriptorHeap.Get()};
        m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

        CD3DX12_VIEWPORT render_res_viewport(
            0.f, 0.f, (float)m_renderResolution.x, (float)m_renderResolution.y);
        CD3DX12_RECT render_res_scissors(0, 0, (LONG)m_renderResolution.x, (LONG)m_renderResolution.y);

        m_commandList->SetGraphicsRootDescriptorTable(
            0, m_appDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
        m_commandList->RSSetViewports(1, &render_res_viewport);
        m_commandList->RSSetScissorRects(1, &render_res_scissors);

        // Transition Color buffer to render target
        std::vector<CD3DX12_RESOURCE_BARRIER> transition =
        { CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
        CD3DX12_RESOURCE_BARRIER::Transition(m_depthTargets[m_frameIndex].Get(),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE),

        };
        m_commandList->ResourceBarrier((UINT)transition.size(), transition.data());

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle =
            CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
                (INT)(m_frameIndex * RTCount + RT_Color), m_rtvDescriptorSize);
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle =
            CD3DX12_CPU_DESCRIPTOR_HANDLE(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(),
                (INT)(m_frameIndex), m_dsvDescriptorSize);
        m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

        // Record commands.
        m_commandList->ClearRenderTargetView(rtvHandle, m_clearColor, 0, nullptr);
        m_commandList->ClearDepthStencilView(
            dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 0.0f, 0, 0, nullptr);
        m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        m_commandList->DrawInstanced(3, 1, 0, 0);
    }

    // Run velocity pass
    {
        m_commandList->SetPipelineState(m_pipelineStateVelocityPass.Get());

        m_commandList->RSSetViewports(1, &m_viewport);
        m_commandList->RSSetScissorRects(1, &m_scissorRect);

        // Indicate that the back buffer will be used as a render target.
        CD3DX12_RESOURCE_BARRIER transition =
            CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargetsVelocity[m_frameIndex].Get(),
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_commandList->ResourceBarrier(1, &transition);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle =
            CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
                (INT)(m_frameIndex * RTCount + RT_Velocity), m_rtvDescriptorSize);
        m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        // Record commands.
        m_commandList->ClearRenderTargetView(rtvHandle, m_clearColor, 0, nullptr);
        m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        m_commandList->DrawInstanced(3, 1, 0, 0);
    }

    // Transition render targets D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE for XeSS
    std::vector<CD3DX12_RESOURCE_BARRIER> transitions = {
        CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargetsVelocity[m_frameIndex].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(m_depthTargets[m_frameIndex].Get(),
            D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
    };
    m_commandList->ResourceBarrier((UINT)transitions.size(), transitions.data());

    // Run XeSS
    {
        xess_d3d12_execute_params_t exec_params{};
        exec_params.inputWidth = m_renderResolution.x;
        exec_params.inputHeight = m_renderResolution.y;
        exec_params.jitterOffsetX = jitter[0];
        exec_params.jitterOffsetY = -jitter[1];
        exec_params.exposureScale = 1.0f;

        exec_params.pColorTexture = m_renderTargets[m_frameIndex].Get();
        exec_params.pVelocityTexture = m_renderTargetsVelocity[m_frameIndex].Get();
        exec_params.pOutputTexture = m_xessOutput[m_frameIndex].Get();
        #if defined(USE_LOWRES_MV)
        exec_params.pDepthTexture = m_depthTargets[m_frameIndex].Get();
        #else
        exec_params.pDepthTexture = nullptr;
        #endif
        exec_params.pExposureScaleTexture = 0;
        auto status = xessD3D12Execute(m_xessContext, m_commandList.Get(), &exec_params);
        if (status != XESS_RESULT_SUCCESS)
        {
            throw std::runtime_error("Unable to run XeSS");
        }
    }

    // Render XeSS output using full screen quad
    {
        ID3D12DescriptorHeap* ppHeaps[] = {m_appDescriptorHeap.Get()};
        m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

        // Transition XeSS output is in UAV so we need to transition it to SRV
        CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
            m_xessOutput[m_frameIndex].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        m_commandList->ResourceBarrier(1, &transition);

        m_commandList->SetPipelineState(m_pipelineStateFSQPass.Get());

        m_commandList->SetGraphicsRootSignature(m_rootSignatureFSQ.Get());

        // Use selected output (DHI_XeSSOutputSRV by default)
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescHandle(
            m_appDescriptorHeap->GetGPUDescriptorHandleForHeapStart(),
            (INT)((DescriptorsPerFrame * m_frameIndex) + m_outputIndex), m_uavDescriptorSize);
        m_commandList->SetGraphicsRootDescriptorTable(0, gpuDescHandle);
        m_commandList->RSSetViewports(1, &m_viewport);
        m_commandList->RSSetScissorRects(1, &m_scissorRect);

        // Indicate that the back buffer will be used as a render target.
        transition =
            CD3DX12_RESOURCE_BARRIER::Transition(m_presentRenderTargets[m_frameIndex].Get(),
                D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_commandList->ResourceBarrier(1, &transition);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle =
            CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
                (INT)(m_frameIndex * RTCount), m_rtvDescriptorSize);
        m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        // Record commands.
        m_commandList->ClearRenderTargetView(rtvHandle, m_clearColor, 0, nullptr);
        m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        m_commandList->DrawInstanced(3, 1, 0, 0);

        // Transition RT to present
        transition =
            CD3DX12_RESOURCE_BARRIER::Transition(m_presentRenderTargets[m_frameIndex].Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        m_commandList->ResourceBarrier(1, &transition);

        // Transition XeSS output to UAV for future use
        transition = CD3DX12_RESOURCE_BARRIER::Transition(m_xessOutput[m_frameIndex].Get(),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        m_commandList->ResourceBarrier(1, &transition);
    }

    ThrowIfFailed(m_commandList->Close());
}

void BasicSample::WaitForPreviousFrame()
{
    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
    // sample illustrates how to use fences for efficient resource usage and to
    // maximize GPU utilization.

    // Signal and increment the fence value.
    const UINT64 fence = m_fenceValue;
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
    m_fenceValue++;

    // Wait until the previous frame is finished.
    if (m_fence->GetCompletedValue() < fence)
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}
