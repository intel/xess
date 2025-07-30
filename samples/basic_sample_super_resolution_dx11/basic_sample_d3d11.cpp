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

#include "basic_sample_d3d11.h"

#include <iostream>
#include <filesystem>
#include <locale>
#include <codecvt>

#include "stdafx.h"
#include "utils.h"
#include "xess/xess_d3d11.h"

//#define USE_LOWRES_MV 1

inline std::string XeSSResultToString(xess_result_t result)
{
    switch (result)
    {
    case XESS_RESULT_WARNING_NONEXISTING_FOLDER: return " XESS_RESULT_WARNING_NONEXISTING_FOLDER";
    case XESS_RESULT_WARNING_OLD_DRIVER: return "XESS_RESULT_WARNING_OLD_DRIVER";
    case XESS_RESULT_SUCCESS: return "XESS_RESULT_SUCCESS";
    case XESS_RESULT_ERROR_UNSUPPORTED_DEVICE: return "XESS_RESULT_ERROR_UNSUPPORTED_DEVICE";
    case XESS_RESULT_ERROR_UNSUPPORTED_DRIVER: return "XESS_RESULT_ERROR_UNSUPPORTED_DRIVER";
    case XESS_RESULT_ERROR_UNINITIALIZED: return "XESS_RESULT_ERROR_UNINITIALIZED";
    case XESS_RESULT_ERROR_INVALID_ARGUMENT: return "XESS_RESULT_ERROR_INVALID_ARGUMENT";
    case XESS_RESULT_ERROR_DEVICE_OUT_OF_MEMORY: return "XESS_RESULT_ERROR_DEVICE_OUT_OF_MEMORY";
    case XESS_RESULT_ERROR_DEVICE: return "XESS_RESULT_ERROR_DEVICE";
    case XESS_RESULT_ERROR_NOT_IMPLEMENTED: return "XESS_RESULT_ERROR_NOT_IMPLEMENTED";
    case XESS_RESULT_ERROR_INVALID_CONTEXT: return "XESS_RESULT_ERROR_INVALID_CONTEXT";
    case XESS_RESULT_ERROR_OPERATION_IN_PROGRESS: return "XESS_RESULT_ERROR_OPERATION_IN_PROGRESS";
    case XESS_RESULT_ERROR_UNSUPPORTED: return "XESS_RESULT_ERROR_UNSUPPORTED";
    case XESS_RESULT_ERROR_CANT_LOAD_LIBRARY: return "XESS_RESULT_ERROR_CANT_LOAD_LIBRARY";
    case XESS_RESULT_ERROR_WRONG_CALL_ORDER: return "XESS_RESULT_ERROR_WRONG_CALL_ORDER";
    case XESS_RESULT_ERROR_UNKNOWN: return "XESS_RESULT_ERROR_UNKNOWN";
    default: return "Unknown error code";
    }
}

inline void ThrowIfFailed(xess_result_t result, const std::string& err)
{
    if (result > XESS_RESULT_SUCCESS) // warnings
    {
        OutputDebugStringA(("XeSS-SR warning: " + XeSSResultToString(result)).c_str());
    }
    else if (result != XESS_RESULT_SUCCESS)
    {
        throw std::runtime_error(err + ". Error code: " + XeSSResultToString(result) + ".");
    }
}

inline void ThrowIfFailed(HRESULT result, const std::string& err)
{
    if (result != S_OK)
    {
        throw std::runtime_error(err);
    }
}

BasicSampleD3D11::BasicSampleD3D11(UINT width, UINT height, std::wstring name)
    : DXSample(width, height, name)
    , m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height))
    , m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height))
    , m_constantBufferData{}
    , m_pCbvDataBegin(nullptr)
    , m_frameIndex(0)
    , m_fenceValues{}
    , m_desiredOutputResolution{width, height}

{
    m_haltonPointSet = Utils::GenerateHalton(2, 3, 1, 32);
}

void BasicSampleD3D11::OnInit()
{
    InitDx();
    InitXess();
    LoadPipeline();
    LoadAssets();
    CreateFSQPipeline();
}

void BasicSampleD3D11::InitDx()
{
    UINT createDeviceFlags = 0;

#if defined(_DEBUG)
    // Enable the debug layer 
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ZeroMemory( &swapChainDesc, sizeof( swapChainDesc ) );
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.BufferDesc.Width = m_width;
    swapChainDesc.BufferDesc.Height = m_height;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    //swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    //swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = Win32Application::GetHwnd();
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.Windowed = TRUE;

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE( featureLevels );
    ComPtr<IDXGISwapChain> swapChain;
    if (m_useWarpDevice)
    {
        auto driverType = D3D_DRIVER_TYPE_WARP;
        D3D_FEATURE_LEVEL m_featureLevel;
        D3D11CreateDeviceAndSwapChain( NULL, driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
            D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &m_device, &m_featureLevel, m_immediateContext.GetAddressOf() );
    } else
    {
        auto driverType = D3D_DRIVER_TYPE_HARDWARE;
        D3D_FEATURE_LEVEL m_featureLevel;

        ComPtr<IDXGIAdapter> hardwareAdapter;
        if (m_hardwareAdapterId != -1)
        {
            ComPtr<IDXGIFactory1> factory;
            ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

            ThrowIfFailed(factory->EnumAdapters((UINT)m_hardwareAdapterId, &hardwareAdapter), "Unabe to use hardware adapter with id " + std::to_string(m_hardwareAdapterId));
            driverType = D3D_DRIVER_TYPE_UNKNOWN;
        }

        ThrowIfFailed(D3D11CreateDeviceAndSwapChain( hardwareAdapter.Get(), driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
            D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &m_device, &m_featureLevel, m_immediateContext.GetAddressOf() ));
    }

    ThrowIfFailed(swapChain.As(&m_swapChain));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void BasicSampleD3D11::InitXess()
{
    ThrowIfFailed(xessD3D11CreateContext(m_device.Get(), &m_xessContext), "Unable to create XeSS context");

    if (XESS_RESULT_WARNING_OLD_DRIVER == xessIsOptimalDriver(m_xessContext))
    {
        MessageBox(NULL, L"Please install the latest graphics driver from your vendor for optimal Intel(R) XeSS performance and visual quality", L"Important notice", MB_OK | MB_TOPMOST | MB_ICONINFORMATION);
    }

    xess_d3d11_init_params_t params = {
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
    };

    ThrowIfFailed(xessD3D11Init(m_xessContext, &params), "Unable to initialize XeSS context");

    // Get optimal input resolution
    ThrowIfFailed(xessGetInputResolution(
        m_xessContext, &m_desiredOutputResolution, m_quality, &m_renderResolution), "Unable to get input resolution");
}

// Load the rendering pipeline dependencies.
void BasicSampleD3D11::LoadPipeline()
{
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

    // Get swapchain
    auto hr = m_swapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ),
        reinterpret_cast<void**>( m_presentRenderTargets[ 0 ].resource.GetAddressOf() ) );
    ThrowIfFailed( hr );

    // RT_Present
    {
        auto rtv_desc = D3D11_RENDER_TARGET_VIEW_DESC{};
        rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        rtv_desc.Texture2D.MipSlice = 0;

        hr = m_device->CreateRenderTargetView( m_presentRenderTargets[ 0 ].resource.Get(), &rtv_desc,
            reinterpret_cast<ID3D11RenderTargetView**>( m_presentRenderTargets[ 0 ].view[ VI_RTV ].GetAddressOf() ) );

        ThrowIfFailed( hr );
    }

    // Create frame resources.
    // Create render targets and xes output for each frame
    for( UINT n = 0; n < FrameCount; n++ )
    {
        DXGI_FORMAT fmt = DXGI_FORMAT_R16G16B16A16_FLOAT;
       
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory( &desc, sizeof( desc ) );

        // Render target
        {
            desc.Width = m_renderResolution.x;
            desc.Height = m_renderResolution.y;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.SampleDesc.Count = 1;
            desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

            hr = m_device->CreateTexture2D( &desc, NULL, 
                reinterpret_cast<ID3D11Texture2D**>( m_renderTargets[ n ].resource.GetAddressOf() ) );
            ThrowIfFailed( hr );
        }

        // Depth resource
        {
            desc.Format = m_dsvFormat;
            desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

            ThrowIfFailed( m_device->CreateTexture2D( &desc, NULL,
                reinterpret_cast<ID3D11Texture2D**>( m_depthTargets[ n ].resource.GetAddressOf() ) ) );
        }

        // XeSS output
        {
            desc.Width = m_desiredOutputResolution.x;
            desc.Height = m_desiredOutputResolution.y;
            desc.Format = fmt;
            desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;

            ThrowIfFailed( m_device->CreateTexture2D( &desc, NULL,
                reinterpret_cast<ID3D11Texture2D**>( m_xessOutput[ n ].resource.GetAddressOf() ) ) );
        }

        // Velocity
        {
            fmt = DXGI_FORMAT_R16G16_FLOAT;

            desc.Width = m_desiredOutputResolution.x;
            desc.Height = m_desiredOutputResolution.y;
            desc.Format = fmt;
            desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

            ThrowIfFailed( m_device->CreateTexture2D( &desc, NULL,
                reinterpret_cast<ID3D11Texture2D**>( m_renderTargetsVelocity[ n ].resource.GetAddressOf() ) ) );
        }

        // Create RTVs
        // RT_Color
        {
            D3D11_RENDER_TARGET_VIEW_DESC rtv_desc;
            rtv_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            rtv_desc.Texture2D.MipSlice = 0;

            ThrowIfFailed( m_device->CreateRenderTargetView( m_renderTargets[ n ].resource.Get(), &rtv_desc,
                reinterpret_cast<ID3D11RenderTargetView**>( m_renderTargets[ n ].view[ VI_RTV ].GetAddressOf() ) ));
        }

        // RT_Velocity
        {
            D3D11_RENDER_TARGET_VIEW_DESC rtv_desc;
            rtv_desc.Format = DXGI_FORMAT_R16G16_FLOAT;
            rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            rtv_desc.Texture2D.MipSlice = 0;

            ThrowIfFailed( m_device->CreateRenderTargetView( m_renderTargetsVelocity[ n ].resource.Get(), &rtv_desc,
                reinterpret_cast<ID3D11RenderTargetView**>( m_renderTargetsVelocity[ n ].view[ VI_RTV ].GetAddressOf() ) ) );
        }

        // RT_Depth
        {
            auto dsv_desc = D3D11_DEPTH_STENCIL_VIEW_DESC{};
            dsv_desc.Format = m_dsvTypedFormat;
            dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            dsv_desc.Flags = 0;
            dsv_desc.Texture2D = D3D11_TEX2D_DSV{};
            dsv_desc.Texture2D.MipSlice = 0;
            ThrowIfFailed( m_device->CreateDepthStencilView( m_depthTargets[ n ].resource.Get(), &dsv_desc,
                reinterpret_cast<ID3D11DepthStencilView**>( m_depthTargets[ n ].view[ VI_RTV ].GetAddressOf() ) ) );
        }

        // SRV XeSS output
        {
            fmt = DXGI_FORMAT_R16G16B16A16_FLOAT;
            auto srv_desc = D3D11_SHADER_RESOURCE_VIEW_DESC{};
            srv_desc.Format = fmt;
            srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srv_desc.Texture2D.MostDetailedMip = 0;
            srv_desc.Texture2D.MipLevels = 1;
            hr = m_device->CreateShaderResourceView( m_xessOutput[ n ].resource.Get(), &srv_desc,
                reinterpret_cast<ID3D11ShaderResourceView**>( m_xessOutput[ n ].view[ VI_SRV ].GetAddressOf() ) );
            ThrowIfFailed( hr );
        }
    }
}

// Compile the shader from given hlsl file (path).
void BasicSampleD3D11::CompileFromFile(LPCWSTR path, const char* entryPoint, const char* shaderModel, UINT compileFlags, ComPtr<ID3DBlob>& shader)
{
    std::wstring assetFullPath = GetAssetFullPath(path).c_str();

    std::filesystem::path fsPath(assetFullPath);

    if (!std::filesystem::exists(fsPath) || !std::filesystem::is_regular_file(fsPath))
    {

        int count = WideCharToMultiByte(CP_ACP, 0, assetFullPath.c_str(), static_cast<int>(assetFullPath.length()), NULL, 0, NULL, NULL);
        std::string narrowPath(count, 0);
        WideCharToMultiByte(CP_ACP, 0, fsPath.c_str(), -1, &narrowPath[0], count, NULL, NULL);

        throw std::runtime_error("Unable to find shader: " + narrowPath);
    }

    ThrowIfFailed(D3DCompileFromFile(assetFullPath.c_str(), nullptr, nullptr, entryPoint, shaderModel, compileFlags, 0, &shader, nullptr));
};

// Load the sample assets.
void BasicSampleD3D11::LoadAssets()
{
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

        CompileFromFile(L"basic_sample_shaders/hlsl/shader_xess_sr_d3d11.hlsl", "VSMainColor", "vs_5_0", compileFlags, vertexShader);
        CompileFromFile(L"basic_sample_shaders/hlsl/shader_xess_sr_d3d11.hlsl", "PSMainColor", "ps_5_0", compileFlags, pixelShader);

        //State creation for d3d11 pipeline

        auto hr = m_device->CreateVertexShader( vertexShader->GetBufferPointer(), 
            vertexShader->GetBufferSize(), NULL, m_pipelineStateColorPass_d3d11.vertexShader.GetAddressOf() );
        ThrowIfFailed( hr );

        hr = m_device->CreatePixelShader( pixelShader->GetBufferPointer(), 
            pixelShader->GetBufferSize(), NULL, m_pipelineStateColorPass_d3d11.pixelShader.GetAddressOf() );
        ThrowIfFailed(hr);

        D3D11_INPUT_ELEMENT_DESC inputElementDescsD3D11[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}};

        hr = m_device->CreateInputLayout( inputElementDescsD3D11, ARRAYSIZE( inputElementDescsD3D11 ), 
            vertexShader->GetBufferPointer(), vertexShader->GetBufferSize(),
            m_pipelineStateColorPass_d3d11.inputLayout.GetAddressOf() );
        ThrowIfFailed( hr );

        CD3D11_DEFAULT defaultDesc;
        D3D11_RASTERIZER_DESC rasterizerDesc = CD3D11_RASTERIZER_DESC( defaultDesc );
        hr = m_device->CreateRasterizerState( &rasterizerDesc, m_pipelineStateColorPass_d3d11.rasterizerState.GetAddressOf() );
        ThrowIfFailed( hr );

        D3D11_BLEND_DESC blendDesc = CD3D11_BLEND_DESC( defaultDesc );
        hr = m_device->CreateBlendState( &blendDesc, m_pipelineStateColorPass_d3d11.blendState.GetAddressOf() );
        ThrowIfFailed( hr );

        D3D11_SAMPLER_DESC samplerDesc = CD3D11_SAMPLER_DESC( defaultDesc );
        hr = m_device->CreateSamplerState( &samplerDesc, m_pipelineStateColorPass_d3d11.samplerState.GetAddressOf() );
        ThrowIfFailed( hr );

        D3D11_DEPTH_STENCIL_DESC depthStencilDesc = CD3D11_DEPTH_STENCIL_DESC( defaultDesc );
        depthStencilDesc.DepthEnable = TRUE;
        depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        depthStencilDesc.StencilEnable = TRUE;
        depthStencilDesc.FrontFace = depthStencilDesc.BackFace =
            D3D11_DEPTH_STENCILOP_DESC{ D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP,
                D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS };
        depthStencilDesc.StencilReadMask = (UINT8)0xff;
        depthStencilDesc.StencilWriteMask = (UINT8)0xff;
        hr = m_device->CreateDepthStencilState( &depthStencilDesc, m_pipelineStateColorPass_d3d11.depthStencilState.GetAddressOf() );
        ThrowIfFailed( hr );

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

        CompileFromFile(L"basic_sample_shaders/hlsl/shader_xess_sr_d3d11.hlsl", "VSMainVelocity", "vs_5_0", compileFlags, vertexShader);
        CompileFromFile(L"basic_sample_shaders/hlsl/shader_xess_sr_d3d11.hlsl", "PSMainVelocity", "ps_5_0", compileFlags, pixelShader);

        //State creation for d3d11 pipeline
        auto hr = m_device->CreateVertexShader( vertexShader->GetBufferPointer(),
            vertexShader->GetBufferSize(), NULL, m_pipelineStateVelocityPass_d3d11.vertexShader.GetAddressOf() );
        ThrowIfFailed( hr );

        hr = m_device->CreatePixelShader( pixelShader->GetBufferPointer(),
            pixelShader->GetBufferSize(), NULL, m_pipelineStateVelocityPass_d3d11.pixelShader.GetAddressOf() );
        ThrowIfFailed( hr );

        D3D11_INPUT_ELEMENT_DESC inputElementDescsD3D11[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0} };

        hr = m_device->CreateInputLayout( inputElementDescsD3D11, ARRAYSIZE( inputElementDescsD3D11 ),
            vertexShader->GetBufferPointer(), vertexShader->GetBufferSize(),
            m_pipelineStateVelocityPass_d3d11.inputLayout.GetAddressOf() );
        ThrowIfFailed( hr );

        CD3D11_DEFAULT defaultDesc;
        D3D11_RASTERIZER_DESC rasterizerDesc = CD3D11_RASTERIZER_DESC( defaultDesc );
        hr = m_device->CreateRasterizerState( &rasterizerDesc, m_pipelineStateVelocityPass_d3d11.rasterizerState.GetAddressOf() );
        ThrowIfFailed( hr );

        D3D11_BLEND_DESC blendDesc = CD3D11_BLEND_DESC( defaultDesc );
        hr = m_device->CreateBlendState( &blendDesc, m_pipelineStateVelocityPass_d3d11.blendState.GetAddressOf() );
        ThrowIfFailed( hr );

        D3D11_SAMPLER_DESC samplerDesc = CD3D11_SAMPLER_DESC( defaultDesc );
        hr = m_device->CreateSamplerState( &samplerDesc, m_pipelineStateVelocityPass_d3d11.samplerState.GetAddressOf() );
        ThrowIfFailed( hr );

        D3D11_DEPTH_STENCIL_DESC depthStencilDesc = CD3D11_DEPTH_STENCIL_DESC( defaultDesc );
        depthStencilDesc.DepthEnable = FALSE;
        depthStencilDesc.StencilEnable = FALSE;
        hr = m_device->CreateDepthStencilState( &depthStencilDesc, m_pipelineStateVelocityPass_d3d11.depthStencilState.GetAddressOf() );
        ThrowIfFailed( hr );
    }

    {
        // Define the geometry for a triangle.
        Vertex triangleVertices[] = {
            {{0.0f, 0.25f * m_aspectRatio, 0.01f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{0.25f, -0.25f * m_aspectRatio, 0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
            {{-0.25f, -0.25f * m_aspectRatio, 0.2f}, {0.0f, 0.0f, 1.0f, 1.0f}}};

        const UINT vertexBufferSize = sizeof(triangleVertices);

        //m_vertexBuffer_d3d11
        D3D11_BUFFER_DESC bufferDesc;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.ByteWidth = vertexBufferSize;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0;
        bufferDesc.MiscFlags = 0;
        D3D11_SUBRESOURCE_DATA initData;
        initData.pSysMem = triangleVertices;
        auto hr = m_device->CreateBuffer( &bufferDesc, &initData, m_vertexBuffer_d3d11.GetAddressOf() );
        ThrowIfFailed( hr );
    }

    // Create the constant buffer.
    {
        const UINT constantBufferSize =
            sizeof(SceneConstantBuffer);  // CB size is required to be 256-byte aligned.

        D3D11_BUFFER_DESC cbDesc;
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.ByteWidth = constantBufferSize;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        cbDesc.MiscFlags = 0;
        auto hr = m_device->CreateBuffer( &cbDesc, nullptr, m_constantBuffer_d3d11.GetAddressOf() );
        ThrowIfFailed( hr );
    }
}

void BasicSampleD3D11::CreateFSQPipeline()
{

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

        CompileFromFile(L"basic_sample_shaders/hlsl/shader_xess_sr_d3d11.hlsl", "VSMainFSQ", "vs_5_0", compileFlags, vertexShader);
        CompileFromFile(L"basic_sample_shaders/hlsl/shader_xess_sr_d3d11.hlsl", "PSMainFSQ", "ps_5_0", compileFlags, pixelShader);

        //State creation for d3d11 pipeline
        auto hr = m_device->CreateVertexShader( vertexShader->GetBufferPointer(),
            vertexShader->GetBufferSize(), NULL, m_pipelineStateFSQPass_d3d11.vertexShader.GetAddressOf() );
        ThrowIfFailed( hr );

        hr = m_device->CreatePixelShader( pixelShader->GetBufferPointer(),
            pixelShader->GetBufferSize(), NULL, m_pipelineStateFSQPass_d3d11.pixelShader.GetAddressOf() );
        ThrowIfFailed( hr );

        CD3D11_DEFAULT defaultDesc;
        D3D11_RASTERIZER_DESC rasterizerDesc = CD3D11_RASTERIZER_DESC( defaultDesc );
        hr = m_device->CreateRasterizerState( &rasterizerDesc, m_pipelineStateFSQPass_d3d11.rasterizerState.GetAddressOf() );
        ThrowIfFailed( hr );

        D3D11_BLEND_DESC blendDesc = CD3D11_BLEND_DESC( defaultDesc );
        hr = m_device->CreateBlendState( &blendDesc, m_pipelineStateFSQPass_d3d11.blendState.GetAddressOf() );
        ThrowIfFailed( hr );

        D3D11_SAMPLER_DESC samplerDesc = CD3D11_SAMPLER_DESC( defaultDesc );
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
        samplerDesc.MipLODBias = 0;
        samplerDesc.MaxAnisotropy = 0;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        samplerDesc.BorderColor[0] = 0.f;
        samplerDesc.BorderColor[ 1 ] = 0.f;
        samplerDesc.BorderColor[ 2 ] = 0.f;
        samplerDesc.BorderColor[ 3 ] = 0.f;
        samplerDesc.MinLOD = 0.0f;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        hr = m_device->CreateSamplerState( &samplerDesc, m_pipelineStateFSQPass_d3d11.samplerState.GetAddressOf() );
        ThrowIfFailed( hr );

        D3D11_DEPTH_STENCIL_DESC depthStencilDesc = CD3D11_DEPTH_STENCIL_DESC( defaultDesc );
        depthStencilDesc.DepthEnable = FALSE;
        depthStencilDesc.StencilEnable = FALSE;
        hr = m_device->CreateDepthStencilState( &depthStencilDesc, m_pipelineStateFSQPass_d3d11.depthStencilState.GetAddressOf() );
        ThrowIfFailed( hr );
    }
}

// Update frame-based values.
void BasicSampleD3D11::OnUpdate()
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

    //memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));
    {
        // update CB per frame D3D11
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        auto hr = m_immediateContext->Map( m_constantBuffer_d3d11.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
        ThrowIfFailed( hr );
        memcpy( mappedResource.pData, &m_constantBufferData, sizeof( m_constantBufferData ) );
        m_immediateContext->Unmap( m_constantBuffer_d3d11.Get(), 0 );
    }

    if (last_fps_time.time_since_epoch().count() == 0)
    {
        last_fps_time = std::chrono::high_resolution_clock::now();
    }
    float fps_timer = (float)(std::chrono::duration<double, std::milli>(current_time - last_fps_time).count());
    if (fps_timer > 1000.0f)
    {
        SetCustomWindowText((std::to_wstring(frame_counter) + L" fps").c_str());
        frame_counter = 0;
        last_fps_time = current_time;
    }
    frame_counter++;
}

// Render the scene.
void BasicSampleD3D11::OnRender()
{
    // Record all the commands we need to render the scene into the command list.
    ExecuteCommandList();

    // Present the frame.
    ThrowIfFailed(m_swapChain->Present(0, 0));

    MoveToNextFrame();
}

void BasicSampleD3D11::OnDestroy()
{
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    WaitForGpu();

    ThrowIfFailed(xessDestroyContext(m_xessContext), "Unable to destroy XeSS context");

    //CloseHandle(m_fenceEvent);
}

void BasicSampleD3D11::OnKeyUp(UINT8 key)
{
    switch (key)
    {
    case VK_SPACE:
        m_pause = !m_pause;
        break;
    }
}

// Fill the command list with all the render commands and dependent state.
void BasicSampleD3D11::ExecuteCommandList()
{
    //ID3D11RenderTargetView* const pNullrtv[ 8 ] = { 0 };
    ID3D11ShaderResourceView* const pNullsrv[ 128 ] = { 0 };
    ID3D11Buffer* const pNullBuffer[ 16 ] = { 0 };


    // Run Color pass
    {
        // Set necessary state.
        CD3D11_VIEWPORT render_res_viewport(
            0.f, 0.f, (float)m_renderResolution.x, (float)m_renderResolution.y);
        CD3D11_RECT render_res_scissors(0, 0, (LONG)m_renderResolution.x, (LONG)m_renderResolution.y);

        m_immediateContext->RSSetViewports(1, &render_res_viewport);
        m_immediateContext->RSSetScissorRects(1, &render_res_scissors);

        auto rtvHandle = reinterpret_cast<ID3D11RenderTargetView*>( m_renderTargets[ m_frameIndex ].view[ VI_RTV ].Get() );
        auto dsvHandle = reinterpret_cast<ID3D11DepthStencilView*>( m_depthTargets[ m_frameIndex ].view[ VI_RTV ].Get() );
        m_immediateContext->OMSetRenderTargets( 1, &rtvHandle,dsvHandle );

        m_immediateContext->ClearRenderTargetView( rtvHandle, m_clearColor);
        m_immediateContext->ClearDepthStencilView(
            dsvHandle, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.0f, 0 );
        m_immediateContext->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
        
        auto vbHandle = m_vertexBuffer_d3d11.Get();
        UINT stride = sizeof( Vertex );
        UINT offset = 0;
        m_immediateContext->IASetVertexBuffers( 0, 1, &vbHandle, &stride, &offset );
        m_pipelineStateColorPass_d3d11.SetPipelineState(m_immediateContext.Get());
        m_immediateContext->VSSetConstantBuffers( 0, 1, m_constantBuffer_d3d11.GetAddressOf() );
        m_immediateContext->DrawInstanced( 3, 1, 0, 0 );

    }

    // Run velocity pass
    {
        m_pipelineStateVelocityPass_d3d11.SetPipelineState(m_immediateContext.Get());
        m_immediateContext->RSSetViewports( 1, &m_viewport );
        m_immediateContext->RSSetScissorRects( 1, &m_scissorRect );

        auto rtvHandle = reinterpret_cast<ID3D11RenderTargetView*>( m_renderTargetsVelocity[ m_frameIndex ].view[ VI_RTV ].Get() );
        m_immediateContext->OMSetRenderTargets( 1, &rtvHandle, nullptr );

        // Record commands.
        m_immediateContext->ClearRenderTargetView( rtvHandle, m_clearColor );
        m_immediateContext->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
        auto vbHandle = m_vertexBuffer_d3d11.Get();
        UINT stride = sizeof( Vertex );
        UINT offset = 0;
        m_immediateContext->IASetVertexBuffers( 0, 1, &vbHandle, &stride, &offset );
        m_immediateContext->VSSetConstantBuffers( 0, 1, m_constantBuffer_d3d11.GetAddressOf() );
        m_immediateContext->PSSetConstantBuffers( 0, 1, m_constantBuffer_d3d11.GetAddressOf() );
        m_immediateContext->DrawInstanced( 3, 1, 0, 0 );

        m_immediateContext->Flush();

        m_immediateContext->VSSetConstantBuffers( 0, 1, pNullBuffer );
        m_immediateContext->PSSetConstantBuffers( 0, 1, pNullBuffer );

    }

    // Run XeSS
    {
        xess_d3d11_execute_params_t exec_params{};
        exec_params.inputWidth = m_renderResolution.x;
        exec_params.inputHeight = m_renderResolution.y;
        exec_params.jitterOffsetX = jitter[0];
        exec_params.jitterOffsetY = -jitter[1];
        exec_params.exposureScale = 1.0f;

        exec_params.pColorTexture = m_renderTargets[m_frameIndex].resource.Get();
        exec_params.pVelocityTexture = m_renderTargetsVelocity[m_frameIndex].resource.Get();
        exec_params.pOutputTexture = m_xessOutput[m_frameIndex].resource.Get();
        #if defined(USE_LOWRES_MV)
        exec_params.pDepthTexture = m_depthTargets[m_frameIndex].resource.Get();
        #else
        exec_params.pDepthTexture = nullptr;
        #endif
        exec_params.pExposureScaleTexture = 0;
        ThrowIfFailed(xessD3D11Execute(m_xessContext, &exec_params), "Unable to run XeSS");
    }

    // Render XeSS output using full screen quad
    {
        //m_immediateContext->Flush();
        m_pipelineStateFSQPass_d3d11.SetPipelineState(m_immediateContext.Get());

        //m_commandList->SetGraphicsRootSignature(m_rootSignatureFSQ.Get());

        m_immediateContext->RSSetViewports( 1, &m_viewport );
        m_immediateContext->RSSetScissorRects( 1, &m_scissorRect );

        auto rtvHandle = reinterpret_cast<ID3D11RenderTargetView*>( m_presentRenderTargets[ 0 ].view[ VI_RTV ].Get() );
        m_immediateContext->OMSetRenderTargets(1, &rtvHandle, nullptr);

        // Record commands.
        m_immediateContext->ClearRenderTargetView( rtvHandle, m_clearColor );
        m_immediateContext->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
        auto vbHandle = m_vertexBuffer_d3d11.Get();
        UINT stride = sizeof( Vertex );
        UINT offset = 0;
        m_immediateContext->IASetVertexBuffers( 0, 1, &vbHandle, &stride, &offset );
        auto srvHandle = reinterpret_cast<ID3D11ShaderResourceView*>( m_xessOutput[ m_frameIndex ].view[ VI_SRV ].Get() );
        m_immediateContext->PSSetShaderResources( 0, 1, &srvHandle );
        m_immediateContext->DrawInstanced( 3, 1, 0, 0 );

        m_immediateContext->Flush();
        m_immediateContext->PSSetShaderResources( 0, 1, pNullsrv );
    }
}

// Wait for pending GPU work to complete.
void BasicSampleD3D11::WaitForGpu()
{
    m_immediateContext->Flush();
}

// Prepare to render the next frame.
void BasicSampleD3D11::MoveToNextFrame()
{
    // Update the frame index.
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}
