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

#include "stdafx.h"
#include "basic_sample.h"

#include <xefg_swapchain_debug.h>

#include <algorithm>
#include <mutex>
#include <sstream>

#define USE_APP_SWAPCHAIN_OBJECT

inline const char* ToString(xefg_swapchain_result_t result)
{
    switch (result)
    {
    case XEFG_SWAPCHAIN_RESULT_WARNING_OLD_DRIVER: return "XEFG_SWAPCHAIN_RESULT_WARNING_OLD_DRIVER";
    case XEFG_SWAPCHAIN_RESULT_WARNING_TOO_FEW_FRAMES: return "XEFG_SWAPCHAIN_RESULT_WARNING_TOO_FEW_FRAMES";
    case XEFG_SWAPCHAIN_RESULT_WARNING_FRAMES_ID_MISMATCH: return "XEFG_SWAPCHAIN_RESULT_WARNING_FRAMES_ID_MISMATCH";
    case XEFG_SWAPCHAIN_RESULT_WARNING_MISSING_PRESENT_STATUS: return "XEFG_SWAPCHAIN_RESULT_WARNING_MISSING_PRESENT_STATUS";
    case XEFG_SWAPCHAIN_RESULT_WARNING_RESOURCE_SIZES_MISMATCH: return "XEFG_SWAPCHAIN_RESULT_WARNING_RESOURCE_SIZES_MISMATCH";
    case XEFG_SWAPCHAIN_RESULT_SUCCESS: return "XEFG_SWAPCHAIN_RESULT_SUCCESS";
    case XEFG_SWAPCHAIN_RESULT_ERROR_UNSUPPORTED_DEVICE: return "XEFG_SWAPCHAIN_RESULT_ERROR_UNSUPPORTED_DEVICE";
    case XEFG_SWAPCHAIN_RESULT_ERROR_UNSUPPORTED_DRIVER: return "XEFG_SWAPCHAIN_RESULT_ERROR_UNSUPPORTED_DRIVER";
    case XEFG_SWAPCHAIN_RESULT_ERROR_UNINITIALIZED: return "XEFG_SWAPCHAIN_RESULT_ERROR_UNINITIALIZED";
    case XEFG_SWAPCHAIN_RESULT_ERROR_INVALID_ARGUMENT: return "XEFG_SWAPCHAIN_RESULT_ERROR_INVALID_ARGUMENT";
    case XEFG_SWAPCHAIN_RESULT_ERROR_DEVICE_OUT_OF_MEMORY: return "XEFG_SWAPCHAIN_RESULT_ERROR_DEVICE_OUT_OF_MEMORY";
    case XEFG_SWAPCHAIN_RESULT_ERROR_DEVICE: return "XEFG_SWAPCHAIN_RESULT_ERROR_DEVICE";
    case XEFG_SWAPCHAIN_RESULT_ERROR_NOT_IMPLEMENTED: return "XEFG_SWAPCHAIN_RESULT_ERROR_NOT_IMPLEMENTED";
    case XEFG_SWAPCHAIN_RESULT_ERROR_INVALID_CONTEXT: return "XEFG_SWAPCHAIN_RESULT_ERROR_INVALID_CONTEXT";
    case XEFG_SWAPCHAIN_RESULT_ERROR_OPERATION_IN_PROGRESS: return "XEFG_SWAPCHAIN_RESULT_ERROR_OPERATION_IN_PROGRESS";
    case XEFG_SWAPCHAIN_RESULT_ERROR_UNSUPPORTED: return "XEFG_SWAPCHAIN_RESULT_ERROR_UNSUPPORTED";
    case XEFG_SWAPCHAIN_RESULT_ERROR_CANT_LOAD_LIBRARY: return "XEFG_SWAPCHAIN_RESULT_ERROR_CANT_LOAD_LIBRARY";
    case XEFG_SWAPCHAIN_RESULT_ERROR_MISMATCH_INPUT_RESOURCES: return "XEFG_SWAPCHAIN_RESULT_ERROR_MISMATCH_INPUT_RESOURCES";
    case XEFG_SWAPCHAIN_RESULT_ERROR_INCORRECT_OUTPUT_RESOURCES: return "XEFG_SWAPCHAIN_RESULT_ERROR_INCORRECT_OUTPUT_RESOURCES";
    case XEFG_SWAPCHAIN_RESULT_ERROR_INCORRECT_INPUT_RESOURCES: return "XEFG_SWAPCHAIN_RESULT_ERROR_INCORRECT_INPUT_RESOURCES";
    case XEFG_SWAPCHAIN_RESULT_ERROR_LATENCY_REDUCTION_UNSUPPORTED: return "XEFG_SWAPCHAIN_RESULT_ERROR_LATENCY_REDUCTION_UNSUPPORTED";
    case XEFG_SWAPCHAIN_RESULT_ERROR_LATENCY_REDUCTION_FUNCTION_MISSING: return "XEFG_SWAPCHAIN_RESULT_ERROR_LATENCY_REDUCTION_FUNCTION_MISSING";
    case XEFG_SWAPCHAIN_RESULT_ERROR_HRESULT_FAILURE: return "XEFG_SWAPCHAIN_RESULT_ERROR_HRESULT_FAILURE";
    case XEFG_SWAPCHAIN_RESULT_ERROR_DXGI_INVALID_CALL: return "XEFG_SWAPCHAIN_RESULT_ERROR_DXGI_INVALID_CALL";
    case XEFG_SWAPCHAIN_RESULT_ERROR_POINTER_STILL_IN_USE: return "XEFG_SWAPCHAIN_RESULT_ERROR_POINTER_STILL_IN_USE";
    case XEFG_SWAPCHAIN_RESULT_ERROR_INVALID_DESCRIPTOR_HEAP: return "XEFG_SWAPCHAIN_RESULT_ERROR_INVALID_DESCRIPTOR_HEAP";
    case XEFG_SWAPCHAIN_RESULT_ERROR_UNKNOWN: return "XEFG_SWAPCHAIN_RESULT_ERROR_UNKNOWN";
    default: return "Unknown error code";
    }
}

inline const char* ToString(xell_result_t result)
{
    switch (result)
    {
    case XELL_RESULT_SUCCESS: return "XELL_RESULT_SUCCESS";
    case XELL_RESULT_ERROR_UNSUPPORTED_DEVICE: return "XELL_RESULT_ERROR_UNSUPPORTED_DEVICE";
    case XELL_RESULT_ERROR_UNSUPPORTED_DRIVER: return "XELL_RESULT_ERROR_UNSUPPORTED_DRIVER";
    case XELL_RESULT_ERROR_UNINITIALIZED: return "XELL_RESULT_ERROR_UNINITIALIZED";
    case XELL_RESULT_ERROR_INVALID_ARGUMENT: return "XELL_RESULT_ERROR_INVALID_ARGUMENT";
    case XELL_RESULT_ERROR_DEVICE: return "XELL_RESULT_ERROR_DEVICE";
    case XELL_RESULT_ERROR_NOT_IMPLEMENTED: return "XELL_RESULT_ERROR_NOT_IMPLEMENTED";
    case XELL_RESULT_ERROR_INVALID_CONTEXT: return "XELL_RESULT_ERROR_INVALID_CONTEXT";
    case XELL_RESULT_ERROR_UNSUPPORTED: return "XELL_RESULT_ERROR_UNSUPPORTED";
    case XELL_RESULT_ERROR_UNKNOWN: return "XELL_RESULT_ERROR_UNKNOWN";
    default: return "Unknown error code";
    }
}

inline void ThrowIfFailed(xefg_swapchain_result_t result, const std::string &err)
{
    if (result > XEFG_SWAPCHAIN_RESULT_SUCCESS) // warnings
    {
        DebugPrintf("XeSS-FG warning: %s\n", ToString(result));
    }
    else if (result != XEFG_SWAPCHAIN_RESULT_SUCCESS)
    {
        throw std::runtime_error(err + ". Error code: " + ToString(result) + ".");
    }
}

inline void ThrowIfFailed(xell_result_t result, const std::string& err)
{
    if (result != XEFG_SWAPCHAIN_RESULT_SUCCESS)
    {
        throw std::runtime_error(err + ". Error code: " + ToString(result) + ".");
    }
}

inline void ThrowIfFailed(HRESULT result, const std::string& err)
{
    if (result != S_OK)
    {
        throw std::runtime_error(err);
    }
}

static void SetDebugMessageFilter(ID3D12Device* device)
{
#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> infoQueue;
    if (FAILED(device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
    {
        return;
    }

    // Set the message filter to ignore warnings.
    D3D12_MESSAGE_ID hide[] =
    {
        D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,  // benign
        D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,  // benign
    };

    D3D12_INFO_QUEUE_FILTER filter = {};
    filter.DenyList.NumIDs = _countof(hide);
    filter.DenyList.pIDList = hide;

    if (FAILED(infoQueue->AddStorageFilterEntries(&filter)))
    {
        return;
    }
#else
    (void)device;
#endif
}

template<class T>
T Clamp(T value, T lower, T upper)
{
    if (value < lower)
    {
        return lower;
    }

    if (value > upper)
    {
        return upper;
    }

    return value;
}

static ComPtr<ID3D12Heap> CreateExternalBufferHeap(ID3D12Device* device, uint64_t size)
{
    ComPtr<ID3D12Heap> heap;

    D3D12_HEAP_DESC desc{};
    desc.SizeInBytes = size;
    desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
    desc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
    ThrowIfFailed(device->CreateHeap(&desc, IID_PPV_ARGS(&heap)));

    heap->SetName(L"ExternalBufferHeap");

    return heap;
}

static ComPtr<ID3D12Heap> CreateExternalTextureHeap(ID3D12Device* device, uint64_t size)
{
    ComPtr<ID3D12Heap> heap;

    D3D12_HEAP_DESC desc{};
    desc.SizeInBytes = size;
    desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Flags = D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES;
    desc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
    ThrowIfFailed(device->CreateHeap(&desc, IID_PPV_ARGS(&heap)));

    heap->SetName(L"ExternalTextureHeap");

    return heap;
}

BasicSample::BasicSample(UINT width, UINT height, std::wstring name) :
    DXSample(width, height, std::move(name)),
    m_frameIndex(0),
    m_backBufferIndex(0),
    m_fenceValue(0),
    m_pCbvDataBegin(nullptr),
    m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
    m_dsvFormat(),
    m_dsvTypedFormat(),
    m_rtvDescriptorSize(0),
    m_dsvDescriptorSize(0),
    m_uavDescriptorSize(0),
    blankResDesc(),
    m_vertexBufferView(),
    m_constantBufferData{}
{
}

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
    case 0x33: // Key 3
        m_enableXeFG = !m_enableXeFG;
        ThrowIfFailed(xefgSwapChainSetEnabled(m_xefgSwapChain, m_enableXeFG), "Failed to toggle XeSS-FG");
        DebugPrintf("XeSS-FG %s\n", m_enableXeFG ? "ON" : "OFF");
        break;
    case 0x34: // Key 4
        CycleNumInterpolatedFrames();
        break;
    case 0x35:
        CycleUiCompositionState();
        break;
    case VK_F3: // Key F3
        m_switchResolution = true;
        break;
    case VK_F4: // Key F4
        m_fullScreen = !m_fullScreen;
        m_switchFullScreen = true;
        break;
    case VK_F5: // Key F5
        m_showOnlyInterpolation = !m_showOnlyInterpolation;
        ThrowIfFailed(xefgSwapChainEnableDebugFeature(m_xefgSwapChain,
                                                      XEFG_SWAPCHAIN_DEBUG_FEATURE_SHOW_ONLY_INTERPOLATION,
                                                      m_showOnlyInterpolation, nullptr),
                      "Failed to set XEFG_SWAPCHAIN_DEBUG_FEATURE_SHOW_ONLY_INTERPOLATION");
        break;
    case VK_F6: // Key F6
        m_tagInterpolatedFrames = !m_tagInterpolatedFrames;
        ThrowIfFailed(xefgSwapChainEnableDebugFeature(m_xefgSwapChain,
                                                      XEFG_SWAPCHAIN_DEBUG_FEATURE_TAG_INTERPOLATED_FRAMES,
                                                      m_tagInterpolatedFrames, nullptr),
                      "Failed to set XEFG_SWAPCHAIN_DEBUG_FEATURE_TAG_INTERPOLATED_FRAMES");
        break;
    case VK_SPACE:
        m_pause = !m_pause;
        break;
    }
}


void BasicSample::SetViewPort(UINT width, UINT height)
{
    m_width = width;
    m_height = height;

    m_viewport.Width = static_cast<FLOAT>(width);
    m_viewport.Height = static_cast<FLOAT>(height);

    m_scissorRect.right = width;
    m_scissorRect.bottom = height;
}

void BasicSample::OnMouseWheel(WORD wValue)
{
    ThrowIfFailed(xellAddMarkerData(m_xellContext, m_frameCounter, XELL_INPUT_SAMPLE),
                  "Failed to XeLL input sample marker");

    m_verticalOffset += wValue > 1 ? 0.1f : -0.1f;

    m_verticalOffset = Clamp(m_verticalOffset, -1.0f, 1.0f);
}

void BasicSample::OnSleep()
{
    // Pace the application with predicted sleep interval
    xellSleep(m_xellContext, m_frameCounter);
    if (m_frameLatencyWaitableObject)
    {
        (void)WaitForSingleObject(m_frameLatencyWaitableObject, INFINITE);
    }
}

void BasicSample::OnInit()
{
    LoadDX12();
    LoadPipeline();
    LoadAssets();
    CreateFSQPipeline();
    PopulateDescriptorHeap();

    xell_sleep_params_t xellParams = {};
    xellParams.bLowLatencyMode = 1;
    ThrowIfFailed(xellSetSleepMode(m_xellContext, &xellParams), "Failed to set XeLL sleep mode");
    ThrowIfFailed(xefgSwapChainSetEnabled(m_xefgSwapChain, true), "Failed to enable XeSS-FG");
}

// Load the rendering pipeline dependencies.
void BasicSample::LoadDX12()
{
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
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

    auto get_adapter_description_wstrings = [](IDXGIAdapter* adapter, std::wstring& shortDesc, std::wstring& fullDesc)
    {
        DXGI_ADAPTER_DESC desc;
        ThrowIfFailed(adapter->GetDesc(&desc), "Cannot obtain adapter description");
        std::wostringstream sstream;
        sstream << std::hex << std::showbase
            << "VendorId=" << desc.VendorId
            << " DeviceId=" << desc.DeviceId
            << " AdapterLuid.LowPart=" << desc.AdapterLuid.LowPart
            << " AdapterLuid.HighPart=" << desc.AdapterLuid.HighPart
            << std::dec << std::noshowbase
            << " Revision=" << desc.Revision << " SubSysId=" << desc.SubSysId
            << " Desc=" << desc.Description << std::endl;
        shortDesc = desc.Description;
        fullDesc = sstream.str();
    };

    ComPtr<IDXGIFactory4> factory;
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    std::wstring selectedAdapterShortDesc, selectedAdapterFullDesc;

    if (m_useWarpDevice)
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

        get_adapter_description_wstrings(warpAdapter.Get(), selectedAdapterShortDesc, selectedAdapterFullDesc);

        ThrowIfFailed(D3D12CreateDevice(
            warpAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device)
        ));
    }
    else
    {
        ComPtr<IDXGIAdapter1> hardwareAdapter;
        if (m_hardwareAdapterId != -1)
        {
            ThrowIfFailed(
                factory->EnumAdapters1((UINT)m_hardwareAdapterId, &hardwareAdapter),
                "Failed to use hardware adapter with id " + std::to_string(m_hardwareAdapterId));
        }
        else
        {
            GetHardwareAdapter(factory.Get(), &hardwareAdapter);
        }

        if (m_useDebugDevice)
        {
            ComPtr<ID3D12Debug> spDebugController0;
            ComPtr<ID3D12Debug1> spDebugController1;
            SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&spDebugController0)));
            SUCCEEDED(spDebugController0->QueryInterface(IID_PPV_ARGS(&spDebugController1)));
            spDebugController1->SetEnableGPUBasedValidation(true);
        }

        get_adapter_description_wstrings(hardwareAdapter.Get(), selectedAdapterShortDesc, selectedAdapterFullDesc);

        ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device)
        ));
    }

    SetDebugMessageFilter(m_device.Get());

    SetCustomWindowText(selectedAdapterShortDesc.c_str());
    DebugPrintf("Selected adapter: %ls\n", selectedAdapterShortDesc.c_str());

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
    swapChainDesc.Format = m_format;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    if (m_useWaitableSwapChain)
    {
        swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    }

    ComPtr<IDXGISwapChain1> applicationSwapChain;

    if (m_initializeFromApplicationSwapChain)
    {
        ThrowIfFailed(factory->CreateSwapChainForHwnd(
            m_commandQueue.Get(),
            Win32Application::GetHwnd(),
            &swapChainDesc,
            nullptr,
            nullptr,
            &applicationSwapChain));
    }

    // XeLL
    ThrowIfFailed(xellD3D12CreateContext(m_device.Get(), &m_xellContext), "Failed to create XeLL context");

    // XeSS-FG
    ThrowIfFailed(xefgSwapChainD3D12CreateContext(m_device.Get(), &m_xefgSwapChain), "Failed to create XeSS-FG context");

    // We pass a prefix string as the logging callback user data.
    const char* loggingContext = "XeSS-FG: ";
    if (xefgSwapChainSetLoggingCallback(
            m_xefgSwapChain, XEFG_SWAPCHAIN_LOGGING_LEVEL_DEBUG, LogCallback, const_cast<char*>(loggingContext)) !=
        XEFG_SWAPCHAIN_RESULT_SUCCESS)
    {
        DebugPrintf("Failed to set XeSS-FG logging callback.\n");
    }

    ThrowIfFailed(xefgSwapChainSetLatencyReduction(m_xefgSwapChain, m_xellContext), "Failed to set XeLL context");

    xefg_swapchain_d3d12_init_params_t params = {};
    params.maxInterpolatedFrames = m_maxInterpolatedFrames
                                       ? m_maxInterpolatedFrames
                                       : XEFG_SWAPCHAIN_USE_MAX_SUPPORTED_INTERPOLATED_FRAMES;
    params.uiMode = XEFG_SWAPCHAIN_UI_MODE_AUTO;

    if (m_enableExternalDescriptorHeap)
    {
        params.initFlags |= XEFG_SWAPCHAIN_INIT_FLAG_EXTERNAL_DESCRIPTOR_HEAP;
    }

    ThrowIfFailed(xefgSwapChainD3D12GetProperties(
                      m_xefgSwapChain,
                      &params,
                      swapChainDesc.Width,
                      swapChainDesc.Height,
                      swapChainDesc.Format,
                      &m_xefgProperties),
                  "Failed to query XeSS-FG properties");

    // This sample is using external heaps purely for illustration purposes.
    // In practice it would be better to use `tempBufferHeapSize` and `tempTextureHeapSize`
    // to give XeSS-FG a portion of a bigger heap and share this heap with the application.
    //
    // If the swap chain resolution changes, XeSS-FG may require more heap space - see implementation
    // of `OnUpdate` for more details.

    m_fgBufferHeap = CreateExternalBufferHeap(m_device.Get(), m_xefgProperties.tempBufferHeapSize);
    params.pTempBufferHeap = m_fgBufferHeap.Get();

    m_fgTextureHeap = CreateExternalTextureHeap(m_device.Get(), m_xefgProperties.tempTextureHeapSize);
    params.pTempTextureHeap = m_fgTextureHeap.Get();

    if (m_initializeFromApplicationSwapChain)
    {
        // Important: XeSS-FG takes ownership of the native swap chain and destroys it.
        // Do not reference the application swap chain after calling `xefgSwapChainD3D12InitFromSwapChain`,
        // instead obtain a new pointer using `xefgSwapChainD3D12GetSwapChainPtr`.
        params.pApplicationSwapChain = applicationSwapChain.Detach();
    }

    // At this point: either we never created our own swap chain, or we've transferred the ownership to XeSS-FG.
    assert(applicationSwapChain == nullptr);

    if (m_initializeFromApplicationSwapChain)
    {
        ThrowIfFailed(
            xefgSwapChainD3D12InitFromSwapChain(m_xefgSwapChain, m_commandQueue.Get(), &params),
            "Failed to initialize XeSS-FG context");
    }
    else
    {
        ThrowIfFailed(
            xefgSwapChainD3D12InitFromSwapChainDesc(
                m_xefgSwapChain,
                Win32Application::GetHwnd(),
                &swapChainDesc,
                nullptr,
                m_commandQueue.Get(),
                factory.Get(),
                &params),
            "Failed to initialize XeSS-FG context");
    }

    ThrowIfFailed(
        xefgSwapChainD3D12GetSwapChainPtr(m_xefgSwapChain, IID_PPV_ARGS(&m_swapChain)),
        "Failed to get swap chain pointer");

    if (m_useWaitableSwapChain)
    {
        m_frameLatencyWaitableObject = m_swapChain->GetFrameLatencyWaitableObject();
        if (!m_frameLatencyWaitableObject)
        {
            throw std::runtime_error("Failed to get frame latency waitable object");
        }
    }

    if (!m_maxInterpolatedFrames)
    {
        m_maxInterpolatedFrames = m_xefgProperties.maxSupportedInterpolations;
    }

    m_numInterpolatedFrames = m_maxInterpolatedFrames;
    ThrowIfFailed(
        xefgSwapChainSetNumInterpolatedFrames(m_xefgSwapChain, m_numInterpolatedFrames),
        "Failed to set max interpolated frames");

    ThrowIfFailed(
        xefgSwapChainEnableDebugFeature(
            m_xefgSwapChain, XEFG_SWAPCHAIN_DEBUG_FEATURE_TAG_INTERPOLATED_FRAMES, m_tagInterpolatedFrames, nullptr),
        "Failed to enable XEFG_SWAPCHAIN_DEBUG_FEATURE_TAG_INTERPOLATED_FRAMES");

    if (m_fullScreen)
    {
        m_swapChain->SetFullscreenState(TRUE, nullptr);
        ThrowIfFailed(m_swapChain->ResizeBuffers(FrameCount, m_width, m_height, m_format, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING));
    }

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

    m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc{
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, AppDescriptorCount, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0};

    m_device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&m_appDescriptorHeap));
    m_appDescriptorHeap->SetName(L"app_descriptor_heap");
}

void BasicSample::CreateFrameResources()
{
    // Create frame resources.
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

        // Create render targets for each frame
        for (UINT n = 0; n < FrameCount; n++)
        {
            D3D12_HEAP_PROPERTIES heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            DXGI_FORMAT fmt = m_format;
            D3D12_RESOURCE_DESC tex_desc =
                CD3DX12_RESOURCE_DESC::Tex2D(fmt, m_width, m_height);
            tex_desc.MipLevels = 1;

            // Get swapchain
            ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_presentRenderTargets[n])));

            D3D12_CLEAR_VALUE clear_value;
            clear_value.Color[0] = clearColor[0];
            clear_value.Color[1] = clearColor[1];
            clear_value.Color[2] = clearColor[2];
            clear_value.Color[3] = clearColor[3];
            clear_value.Format = fmt;

            // Render target
            tex_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            ThrowIfFailed(m_device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE,
                &tex_desc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, &clear_value,
                IID_PPV_ARGS(&m_renderTargets[n])));

            // Depth
            tex_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            tex_desc.Format = m_dsvFormat;
            D3D12_CLEAR_VALUE clear_value_depth;
            clear_value_depth.DepthStencil.Depth = 0.0;
            clear_value_depth.DepthStencil.Stencil = 0;
            clear_value_depth.Format = m_dsvTypedFormat;
            ThrowIfFailed(m_device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE,
                &tex_desc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, &clear_value_depth,
                IID_PPV_ARGS(&m_depthTargets[n])));

            // Interpolated
            fmt = m_format;
            tex_desc = CD3DX12_RESOURCE_DESC::Tex2D(
                fmt, m_width, m_height);
            tex_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            tex_desc.MipLevels = 1;
            clear_value.Format = fmt;
            ThrowIfFailed(m_device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE,
                &tex_desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &clear_value,
                IID_PPV_ARGS(&m_interpolatedTargets[n])));

            // Velocity
            fmt = DXGI_FORMAT_R16G16_FLOAT;
            tex_desc = CD3DX12_RESOURCE_DESC::Tex2D(
                fmt, m_width, m_height);
            tex_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            tex_desc.MipLevels = 1;
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

            // RT_Interpolated
            m_device->CreateRenderTargetView(m_interpolatedTargets[n].Get(), nullptr, rtvHandle);
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
            m_renderTargetsVelocity[n]->SetName(
                (std::wstring(L"Velocity") + std::to_wstring(n)).c_str());
            m_interpolatedTargets[n]->SetName(
                (std::wstring(L"Interpolated") + std::to_wstring(n)).c_str());
        }
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

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }
    {
        // Describe and create a depth stencil view (DSV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = FrameCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

        m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    }

    m_uavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_dsvFormat = DXGI_FORMAT_R32G8X24_TYPELESS;
    m_dsvTypedFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

    CreateFrameResources();

    if (m_enableExternalDescriptorHeap)
    {
        const uint32_t requiredDescriptorCount = m_xefgProperties.requiredDescriptorCount;
        if (m_xefgDescriptorCount < requiredDescriptorCount * FrameCount)
        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                FrameCount * requiredDescriptorCount,
                D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
                0};

            ThrowIfFailed(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_xefgDescriptorHeap)));

            m_xefgDescriptorCount = FrameCount * requiredDescriptorCount;
        }
    }

    ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
}

// Load the sample assets.
void BasicSample::LoadAssets()
{
    // Create a root signature consisting of a descriptor table with a single CBV.
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

        // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
        CD3DX12_ROOT_PARAMETER1 rootParameters[1];

        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);

        // Allow input layout and deny unnecessary access to certain pipeline stages.
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
        ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
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

        ThrowIfFailed(
            D3DCompileFromFile(
                GetAssetFullPath(L"basic_sample_shaders/hlsl/shader_xess_fg.hlsl").c_str(),
                nullptr,
                nullptr,
                "VSMainColor",
                "vs_5_0",
                compileFlags,
                0,
                &vertexShader,
                nullptr),
            "Failed to find shader basic_sample_shaders/hlsl/shader_xess_fg.hlsl");

        ThrowIfFailed(
            D3DCompileFromFile(
                GetAssetFullPath(L"basic_sample_shaders/hlsl/shader_xess_fg.hlsl").c_str(),
                nullptr,
                nullptr,
                "PSMainColor",
                "ps_5_0",
                compileFlags,
                0,
                &pixelShader,
                nullptr),
            "Failed to find shader basic_sample_shaders/hlsl/shader_xess_fg.hlsl");

        // Define the vertex input layout.
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
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
        psoDesc.RTVFormats[0] = m_format;
        psoDesc.DSVFormat = m_dsvTypedFormat;
        psoDesc.SampleDesc.Count = 1;

        ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateColorPass)));
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

        ThrowIfFailed(
            D3DCompileFromFile(
                GetAssetFullPath(L"basic_sample_shaders/hlsl/shader_xess_fg.hlsl").c_str(),
                nullptr,
                nullptr,
                "VSMainVelocity",
                "vs_5_0",
                compileFlags,
                0,
                &vertexShader,
                nullptr),
            "Failed to find shader basic_sample_shaders/hlsl/shader_xess_fg.hlsl");

        ThrowIfFailed(
            D3DCompileFromFile(
                GetAssetFullPath(L"basic_sample_shaders/hlsl/shader_xess_fg.hlsl").c_str(),
                nullptr,
                nullptr,
                "PSMainVelocity",
                "ps_5_0",
                compileFlags,
                0,
                &pixelShader,
                nullptr),
            "Failed to find shader basic_sample_shaders/hlsl/shader_xess_fg.hlsl");

        // Define the vertex input layout.
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0} };

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

        ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateVelocityPass)));
    }

    // Create the command list.
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineStateColorPass.Get(), IID_PPV_ARGS(&m_commandList)));

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    ThrowIfFailed(m_commandList->Close());

    // Create the vertex buffer.
    {
        // Define the geometry for a triangle.
        Vertex triangleVertices[] =
        {
            { { 0.0f, 0.25f * m_aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        };

        const UINT vertexBufferSize = sizeof(triangleVertices);

        // Note: using upload heaps to transfer static data like vert buffers is not 
        // recommended. Every time the GPU needs it, the upload heap will be marshalled 
        // over. Please read up on Default Heap usage. An upload heap is used here for 
        // code simplicity and because there are very few verts to actually transfer.
        {
            CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
            CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
            ThrowIfFailed(m_device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_vertexBuffer)));
        }

        // Copy the triangle data to the vertex buffer.
        UINT8* pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
        m_vertexBuffer->Unmap(0, nullptr);

        // Initialize the vertex buffer view.
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(Vertex);
        m_vertexBufferView.SizeInBytes = vertexBufferSize;
    }

    // Create the constant buffer.
    {
        const UINT constantBufferSize = sizeof(SceneConstantBuffer);    // CB size is required to be 256-byte aligned.

        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);
        ThrowIfFailed(m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_constantBuffer)));

        // Describe and create a constant buffer view.
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = constantBufferSize;
        m_device->CreateConstantBufferView(&cbvDesc, m_appDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

        // Map and initialize the constant buffer. We don't unmap this until the
        // app closes. Keeping things mapped for the lifetime of the resource is okay.
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin)));
        memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));
    }

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fenceValue = 1;

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
        }
        else
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
        addTexture(DescriptorsPerFrame * n + RT_Color,
            m_renderTargets[n].Get(), m_format);
        addTexture(DescriptorsPerFrame * n + RT_Velocity,
            m_renderTargetsVelocity[n].Get(), DXGI_FORMAT_R16G16_FLOAT);
        addTexture(DescriptorsPerFrame * n + RT_Interpolated,
            m_interpolatedTargets[n].Get(), m_format);
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

        ThrowIfFailed(
            D3DCompileFromFile(
                GetAssetFullPath(L"basic_sample_shaders/hlsl/shader_xess_fg.hlsl").c_str(),
                nullptr,
                nullptr,
                "VSMainFSQ",
                "vs_5_0",
                compileFlags,
                0,
                &vertexShader,
                nullptr),
            "Failed to find shader basic_sample_shaders/hlsl/shader_xess_fg.hlsl");

        ThrowIfFailed(
            D3DCompileFromFile(
                GetAssetFullPath(L"basic_sample_shaders/hlsl/shader_xess_fg.hlsl").c_str(),
                nullptr,
                nullptr,
                "PSMainFSQ",
                "ps_5_0",
                compileFlags,
                0,
                &pixelShader,
                nullptr),
            "Failed to find shader basic_sample_shaders/hlsl/shader_xess_fg.hlsl");

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
        psoDesc.RTVFormats[0] = m_format;
        psoDesc.SampleDesc.Count = 1;

        ThrowIfFailed(
            m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateFSQPass)));
    }
}

// Update frame-based values.
void BasicSample::OnUpdate()
{
    ThrowIfFailed(xellAddMarkerData(m_xellContext, m_frameCounter, XELL_SIMULATION_START),
                  "Failed to add XeLL marker XELL_SIMULATION_START");

    if (m_switchFullScreen)
    {
        WaitForExec();
        m_switchFullScreen = false;

        ThrowIfFailed(m_swapChain->SetFullscreenState(m_fullScreen, nullptr));

        for (int i=0; i<FrameCount; i++)
        {
            m_renderTargets[i].Reset();
            m_depthTargets[i].Reset();
            m_interpolatedTargets[i].Reset();
            m_renderTargetsVelocity[i].Reset();
            m_presentRenderTargets[i].Reset();
        }
        ThrowIfFailed(m_swapChain->ResizeBuffers(FrameCount, m_width, m_height, m_format, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING));
        m_rtvHeap.Reset();
        m_dsvHeap.Reset();

        LoadPipeline();
        PopulateDescriptorHeap();
    }

    if (m_switchResolution)
    {
        WaitForExec();
        m_switchResolution = false;
        if (m_width == 2560)
        {
            SetViewPort(1920, 1080);
        }
        else
        {
            SetViewPort(2560, 1440);
        }
        for (int i = 0; i < FrameCount; i++)
        {
            m_renderTargets[i].Reset();
            m_depthTargets[i].Reset();
            m_interpolatedTargets[i].Reset();
            m_renderTargetsVelocity[i].Reset();
            m_presentRenderTargets[i].Reset();
        }

        DXGI_MODE_DESC mode = {m_width,
                               m_height,
                               0,
                               1,
                               m_fullScreen ? DXGI_FORMAT_UNKNOWN : m_format,
                               DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
                               DXGI_MODE_SCALING_STRETCHED};

        // Query the minimum required heap size for the new resolution. This will tell us how much
        // heap space XeSS-FG will use after resize.
        //
        // Note that if we don't take extra care, XeSS-FG may start aliasing unexpected resources
        // after resize: higher resolutions require more heap space.
        ThrowIfFailed(xefgSwapChainD3D12GetProperties(
                          m_xefgSwapChain,
                          nullptr,
                          m_width,
                          m_height,
                          m_format,
                          &m_xefgProperties),
                      "Failed to query XeSS-FG properties");

        // In this sample we always prepare new heaps for illustration purposes.
        // In practice, if the old heaps have enough space we don't have to do anything.
        m_fgBufferHeap = CreateExternalBufferHeap(m_device.Get(), m_xefgProperties.tempBufferHeapSize);
        m_fgTextureHeap = CreateExternalTextureHeap(m_device.Get(), m_xefgProperties.tempTextureHeapSize);

        ThrowIfFailed(
            xefgSwapChainD3D12UpdateExternalHeapOnResize(
                m_xefgSwapChain, m_fgBufferHeap.Get(), 0U, m_fgTextureHeap.Get(), 0U));

        ThrowIfFailed(m_swapChain->ResizeTarget(&mode));
        ThrowIfFailed(m_swapChain->ResizeBuffers(FrameCount, m_width, m_height, m_format, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING));
        m_rtvHeap.Reset();
        m_dsvHeap.Reset();

        LoadPipeline();
        PopulateDescriptorHeap();
    }

    if (m_lastUpdateTime.time_since_epoch().count() == 0)
    {
        m_lastUpdateTime = std::chrono::high_resolution_clock::now();
    }

    const auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsedSeconds = currentTime - m_lastUpdateTime;
    m_lastUpdateTime = currentTime;

    const double speed = 1.0 / 2; 
    float translationSpeed = m_pause ? 0.f : (float)(speed * elapsedSeconds.count());
    const float offsetBounds = 1.25f;

    m_constantBufferData.offset.y = m_verticalOffset;
    m_constantBufferData.offset.x += translationSpeed;
    if (m_constantBufferData.offset.x > offsetBounds)
    {
        m_constantBufferData.offset.x = -offsetBounds;
    }
    m_constantBufferData.velocity.x = translationSpeed;
    m_constantBufferData.velocity.y = 0;

    memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));

    ThrowIfFailed(xellAddMarkerData(m_xellContext, m_frameCounter, XELL_SIMULATION_END),
                  "Failed to add XELL_SIMULATION_END marker");
}

// Render the scene.
void BasicSample::OnRender()
{
    ThrowIfFailed(xellAddMarkerData(m_xellContext, m_frameCounter, XELL_RENDERSUBMIT_START),
                  "Failed to add XELL_RENDERSUBMIT_START marker");

    // Record all the commands we need to render the scene into the command list.
    PopulateCommandList();

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present rendered frame
    PopulateRenderTargetCommandList();

    ID3D12CommandList* ppCommandLists2[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists2), ppCommandLists2);

    m_lastPresentStatus = {};
    if(m_enableXeFG)
    {
        ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

        // Tag resources for XeSS-FG.
        // All of these resources are valid until `Present`, but we are going to tag
        // some of them as "valid only now" for illustration purposes.

        // color
        {
            xefg_swapchain_d3d12_resource_data_t hudlessColor = {};
            hudlessColor.type = XEFG_SWAPCHAIN_RES_HUDLESS_COLOR;
            hudlessColor.validity = XEFG_SWAPCHAIN_RV_UNTIL_NEXT_PRESENT;
            hudlessColor.resourceBase = { 0, 0 };
            hudlessColor.resourceSize = { m_width, m_height };
            hudlessColor.pResource = m_renderTargets[m_frameIndex].Get();
            hudlessColor.incomingState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            xefgSwapChainD3D12TagFrameResource(m_xefgSwapChain, nullptr, m_frameCounter, &hudlessColor);
        }

        // motion vectors
        {
            xefg_swapchain_d3d12_resource_data_t velocity = {};
            velocity.type = XEFG_SWAPCHAIN_RES_MOTION_VECTOR;
            velocity.validity = XEFG_SWAPCHAIN_RV_ONLY_NOW;
            velocity.pResource = m_renderTargetsVelocity[m_frameIndex].Get();
            D3D12_RESOURCE_DESC desc = m_renderTargetsVelocity[m_frameIndex]->GetDesc();
            velocity.resourceSize = { (std::uint32_t)desc.Width, (std::uint32_t)desc.Height };
            velocity.resourceBase = { 0, 0 };
            velocity.incomingState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            xefgSwapChainD3D12TagFrameResource(m_xefgSwapChain, m_commandList.Get(), m_frameCounter, &velocity);
        }

        // depth
        {
            xefg_swapchain_d3d12_resource_data_t depth = {};
            depth.type = XEFG_SWAPCHAIN_RES_DEPTH;
            depth.validity = XEFG_SWAPCHAIN_RV_ONLY_NOW;
            depth.pResource = m_depthTargets[m_frameIndex].Get();
            D3D12_RESOURCE_DESC desc = m_depthTargets[m_frameIndex]->GetDesc();
            depth.resourceSize = { (std::uint32_t)desc.Width, (std::uint32_t)desc.Height };
            depth.resourceBase = { 0, 0 };
            depth.incomingState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            xefgSwapChainD3D12TagFrameResource(m_xefgSwapChain, m_commandList.Get(), m_frameCounter, &depth);
        }

        // constants
        {
            xefg_swapchain_frame_constant_data_t constData = {};

            XMFLOAT4X4 float4x4;
            float fx = m_constantBufferData.offset.x;
            float fy = m_constantBufferData.offset.y;
            XMStoreFloat4x4(&float4x4, DirectX::XMMatrixTranslation(fx, fy, 0));
            memcpy( constData.viewMatrix, float4x4.m, sizeof(float) * 16 );

            XMStoreFloat4x4(&float4x4, DirectX::XMMatrixIdentity());
            memcpy( constData.projectionMatrix, float4x4.m, sizeof(float) * 16 );

            constData.jitterOffsetX = constData.jitterOffsetY = 0.0f;
            constData.motionVectorScaleX = constData.motionVectorScaleY = 1.0f;
            constData.frameRenderTime = m_lastFrameTimeMS;

            ThrowIfFailed(xefgSwapChainTagFrameConstants(m_xefgSwapChain, m_frameCounter, &constData),
                          "Failed to tag frame constants");
        }

        ThrowIfFailed(m_commandList->Close());

        // Command lists used for resource tagging must be submitted before the corresponding `Present` call.
        {
            ID3D12CommandList* command_lists[] = { m_commandList.Get() };
            m_commandQueue->ExecuteCommandLists(_countof(command_lists), command_lists);
        }

        if (m_enableExternalDescriptorHeap)
        {
            const uint32_t offsetInBytes =
                m_xefgProperties.requiredDescriptorCount * (m_frameCounter % FrameCount) * m_uavDescriptorSize;

            ThrowIfFailed(
                xefgSwapChainD3D12SetDescriptorHeap(m_xefgSwapChain, m_xefgDescriptorHeap.Get(), offsetInBytes),
                "Failed to set descriptor heap");
        }

        ThrowIfFailed(xefgSwapChainSetPresentId(m_xefgSwapChain, m_frameCounter), "Failed to set presentation Id");
    }

    ThrowIfFailed(
        xellAddMarkerData(m_xellContext, m_frameCounter, XELL_RENDERSUBMIT_END),
        "Failed to add XELL_RENDERSUBMIT_END marker");

    ThrowIfFailed(
        xellAddMarkerData(m_xellContext, m_frameCounter, XELL_PRESENT_START),
        "Failed to add XELL_PRESENT_START marker");

    if (m_useAsyncFlip)
    {
        ThrowIfFailed(m_swapChain->Present(
            0, m_fullScreen
                   ? 0
                   : DXGI_PRESENT_ALLOW_TEARING)); // DXGI_PRESENT_ALLOW_TEARING not allowed in full screen mode
    }
    else
    {
        ThrowIfFailed(m_swapChain->Present(1, 0));
    }

    ThrowIfFailed(xellAddMarkerData(m_xellContext, m_frameCounter, XELL_PRESENT_END),
                  "Failed to add XELL_PRESENT_END marker");

    ThrowIfFailed(xefgSwapChainGetLastPresentStatus(m_xefgSwapChain, &m_lastPresentStatus),
                  "Failed to get last presentation status from XeSS-FG");

    if (m_lastPresentStatus.frameGenResult != XEFG_SWAPCHAIN_RESULT_SUCCESS)
    {
        DebugPrintf(
            "Frame generation for frame %u failed with: %s (%d)\n",
            m_frameCounter,
            ToString(m_lastPresentStatus.frameGenResult),
            m_lastPresentStatus.frameGenResult);
    }

    m_frameCounter++;
    WaitForPreviousFrame();

    m_frameIndex++;
    if (m_frameIndex >= FrameCount)
    {
        m_frameIndex = 0;
    }
}

void BasicSample::OnDestroy()
{
    m_useAsyncFlip = false;
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    WaitForPreviousFrame();

    // Swap chain must be windowed before shutdown
    if (m_fullScreen) 
    {
        m_swapChain->SetFullscreenState(false, nullptr);
    }

    if (m_frameLatencyWaitableObject)
    {
        CloseHandle(m_frameLatencyWaitableObject);
    }

    m_swapChain.Reset();
    ThrowIfFailed(xefgSwapChainDestroy(m_xefgSwapChain), "Failed to destroy XeSS-FG swap chain context");
    ThrowIfFailed(xellDestroyContext(m_xellContext), "Failed to destroy XeLL context");
}

// Fill the command list with all the render commands and dependent state.
void BasicSample::PopulateCommandList()
{
    if (!m_useAsyncFlip && (m_frameCounter > 0 && m_frameIndex % FrameCount == 0))
    {
        // Command list allocators can only be reset when the associated 
        // command lists have finished execution on the GPU; apps should use 
        // fences to determine GPU execution progress.
        ThrowIfFailed(m_commandAllocator->Reset());
    }

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineStateColorPass.Get()));

    // Run Color pass
    {
        // Set necessary state.
        m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

        ID3D12DescriptorHeap* ppHeaps[] = { m_appDescriptorHeap.Get() };
        m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

        m_commandList->SetGraphicsRootDescriptorTable(
            0, m_appDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
        m_commandList->RSSetViewports(1, &m_viewport);
        m_commandList->RSSetScissorRects(1, &m_scissorRect);

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
        m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 0.0f, 0, 0, nullptr);
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

        m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

        // Record commands.
        m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        m_commandList->DrawInstanced(3, 1, 0, 0);
    }

    std::vector<CD3DX12_RESOURCE_BARRIER> transitions = {
            CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargetsVelocity[m_frameIndex].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(m_depthTargets[m_frameIndex].Get(),
            D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
    };
    m_commandList->ResourceBarrier((UINT)transitions.size(), transitions.data());

    ThrowIfFailed(m_commandList->Close());
}

void BasicSample::PopulateRenderTargetCommandList()
{
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineStateColorPass.Get()));

    // Render output using full screen quad
    {
        ID3D12DescriptorHeap* ppHeaps[] = { m_appDescriptorHeap.Get() };
        m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

        m_commandList->SetPipelineState(m_pipelineStateFSQPass.Get());
        m_commandList->SetGraphicsRootSignature(m_rootSignatureFSQ.Get());

        // Use selected output 
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescHandle(
            m_appDescriptorHeap->GetGPUDescriptorHandleForHeapStart(),
            (INT)((DescriptorsPerFrame * m_frameIndex) + RT_Color), m_uavDescriptorSize);
        m_commandList->SetGraphicsRootDescriptorTable(0, gpuDescHandle);
        m_commandList->RSSetViewports(1, &m_viewport);
        m_commandList->RSSetScissorRects(1, &m_scissorRect);

        {
            // Indicate that the back buffer will be used as a render target.
            std::vector<CD3DX12_RESOURCE_BARRIER> transitions = {
                CD3DX12_RESOURCE_BARRIER::Transition(m_presentRenderTargets[m_backBufferIndex].Get(),
                    D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
                CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(),
                    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
            };
            // Transition RT to present
            m_commandList->ResourceBarrier((UINT)transitions.size(), transitions.data());
        }
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle =
            CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
                (INT)(m_backBufferIndex * RTCount), m_rtvDescriptorSize);
        m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        // Record commands.
        m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        m_commandList->DrawInstanced(3, 1, 0, 0);

        {
            std::vector<CD3DX12_RESOURCE_BARRIER> transitions = {
                CD3DX12_RESOURCE_BARRIER::Transition(m_presentRenderTargets[m_backBufferIndex].Get(),
                    D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT),
                CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(),
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            };
            // Transition RT to present
            m_commandList->ResourceBarrier((UINT)transitions.size(), transitions.data());
        }
    }

    ThrowIfFailed(m_commandList->Close());
}

void BasicSample::WaitForExec()
{
    // Signal and increment the fence value.
    const UINT64 fence = m_fenceValue;
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
    m_fenceValue++;

    // Wait until the previous frame is finished.
    if (m_fence->GetCompletedValue() < fence)
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(fence, nullptr));
    }
}

void BasicSample::WaitForPreviousFrame()
{
    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // This is code implemented as such for simplicity. This
    // sample illustrates how to use fences for efficient resource usage and to
    // maximize GPU utilization.

    if (m_useAsyncFlip)
    {
        if (m_frameCounter > 0 && (m_frameIndex % FrameCount) == 0)
        {
            WaitForExec();
            // Command list allocators can only be reset when the associated 
            // command lists have finished execution on the GPU; apps should use 
            // fences to determine GPU execution progress.
            ThrowIfFailed(m_commandAllocator->Reset());
        }
    }
    else
    {
        WaitForExec();
    }

    m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void BasicSample::CycleNumInterpolatedFrames()
{
    ++m_numInterpolatedFrames;

    if (m_numInterpolatedFrames > m_maxInterpolatedFrames)
    {
        m_numInterpolatedFrames = 1;
    }

    DebugPrintf("Setting number of interpolated frames to %u\n", m_numInterpolatedFrames);
    ThrowIfFailed(
        xefgSwapChainSetNumInterpolatedFrames(m_xefgSwapChain, m_numInterpolatedFrames),
        "Failed to set number of interpolated frames");
}

void BasicSample::CycleUiCompositionState()
{
    if (m_uiCompositionState == XEFG_SWAPCHAIN_UI_COMPOSITION_STATE_DISABLED)
    {
        m_uiCompositionState = XEFG_SWAPCHAIN_UI_COMPOSITION_STATE_ENABLED;
        DebugPrintf("Enabling UI composition\n");
    }
    else
    {
        m_uiCompositionState = XEFG_SWAPCHAIN_UI_COMPOSITION_STATE_DISABLED;
        DebugPrintf("Disabling UI composition\n");
    }

    ThrowIfFailed(
        xefgSwapChainSetUiCompositionState(m_xefgSwapChain, m_uiCompositionState),
        "Failed to set UI composition state");
}
