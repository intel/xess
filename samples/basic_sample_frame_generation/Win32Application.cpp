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
#include "Win32Application.h"

HWND Win32Application::m_hwnd = nullptr;
BOOL m_exit = false;
HANDLE hWindowCreated = nullptr;
BOOL m_firstPaint = false;
HANDLE hRenderThread = 0;

DWORD WINAPI RenderThread(LPVOID lpParam)
{
    auto pSample = static_cast<DXSample*>(lpParam);
    auto previousTime = std::chrono::high_resolution_clock::now();

    try
    {
        pSample->OnInit();

        // Wait for window creation
        WaitForSingleObject(hWindowCreated, INFINITE);

        const auto targetInterval =
            std::chrono::microseconds(static_cast<int>(pSample->m_minimumFrameTimeMS * 1000.0f));

        int frameCount = 0;

        while (!m_exit)
        {
            // Allow the sample to pace/delay to minimize latency
            pSample->OnSleep();

            // Spin-wait to maintain the target application frame rate.
            //
            // We (optionally) maintain a fixed application frame rate to clearly
            // show the effects of enabling XeSS Frame Generation.
            //
            // We don't use XeLL's frame limiter for simplicity. XeLL caps the display frame rate,
            // which includes both application and generated frames, so using XeLL would require
            // extra logic which may obscure the sample's purpose.
            while (previousTime + targetInterval > std::chrono::high_resolution_clock::now())
            {
                YieldProcessor();
            }

            const auto currentTime = std::chrono::high_resolution_clock::now();
            const auto duration = std::chrono::duration<float, std::milli>(currentTime - previousTime);
            previousTime = currentTime;

            pSample->SetFrameTime(duration.count());
            pSample->OnUpdate();
            pSample->OnRender();

            ++frameCount;

            if (frameCount >= pSample->m_maxFrames)
            {
                m_exit = true;
                break;
            }
        }

        pSample->OnDestroy();
    }
    catch (const std::runtime_error& err)
    {
        m_exit = true;
        MessageBoxA(
            NULL,
            (std::string("XeSS-FG sample error: ") + err.what()).c_str(),
            "Error",
            MB_OK | MB_TOPMOST | MB_ICONINFORMATION);
        return 1;
    }
    return 0;
}


int Win32Application::Run(DXSample* pSample, HINSTANCE hInstance, int nCmdShow)
{
    // Parse the command line parameters
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    pSample->ParseCommandLineArgs(argv, argc);
    LocalFree(argv);
    // Update render size based on command line arguments
    pSample->SetViewPort(pSample->GetWidth(), pSample->GetHeight());

    // Initialize the window class.
    WNDCLASSEX windowClass = { 0 };
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = L"DXSampleClass";
    RegisterClassEx(&windowClass);

    if (pSample->m_fullScreen)
    {
        // Get the settings of the primary display
        DEVMODE devMode = {};
        devMode.dmSize = sizeof(DEVMODE);
        EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &devMode);

        // Create fullscreen window and store a handle to it.
        m_hwnd = CreateWindow(
            windowClass.lpszClassName,
            nullptr, 
            WS_POPUP, 
            devMode.dmPosition.x,
            devMode.dmPosition.y,
            static_cast<LONG>(devMode.dmPelsWidth),
            static_cast<LONG>(devMode.dmPelsHeight),
            nullptr,        // We have no parent window.
            nullptr,        // We aren't using menus.
            hInstance,
            pSample);

        pSample->SetViewPort(static_cast<UINT>(devMode.dmPelsWidth), static_cast<UINT>(devMode.dmPelsHeight));

        SetWindowPos(
            m_hwnd,
            HWND_TOPMOST,
            0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOACTIVATE);
    }
    else
    {
        // Get the settings for the windows display
        RECT windowRect = {
            0,
            0,
            static_cast<LONG>(pSample->GetWidth()),
            static_cast<LONG>(pSample->GetHeight())
        };

        AdjustWindowRect(&windowRect, pSample->m_borderless ? WS_POPUP : WS_OVERLAPPEDWINDOW, FALSE);

        // Create the window and store a handle to it.
        m_hwnd = CreateWindowEx(
            pSample->m_topmost ? WS_EX_TOPMOST : 0,
            windowClass.lpszClassName,
            pSample->GetTitle(),
            pSample->m_borderless ? WS_POPUP : WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            windowRect.right - windowRect.left,
            windowRect.bottom - windowRect.top,
            nullptr,        // We have no parent window.
            nullptr,        // We aren't using menus.
            hInstance,
            pSample);
    }

    ShowWindow(m_hwnd, pSample->m_fullScreen || pSample->m_maximized ? SW_MAXIMIZE : nCmdShow);

    // Start render thread
    hWindowCreated = CreateEvent(NULL, TRUE, FALSE, L"CreateWindowEvent");
    hRenderThread = CreateThread(NULL, 0, RenderThread, (LPVOID)pSample, 0, NULL);

    // Main sample loop.
    MSG msg = {};
    while ((msg.message != WM_QUIT) && !m_exit)
    {
        // Process all messages in the queue.
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    m_exit = true;
    SetEvent(hWindowCreated);

    // Wait for render thread to stop
    WaitForSingleObject(hRenderThread, INFINITE);
    CloseHandle(hRenderThread);

    CloseHandle(hWindowCreated);
    hWindowCreated = nullptr;

    // Return this part of the WM_QUIT message to Windows.
    return static_cast<char>(msg.wParam);
}

// Main message handler for the sample.
LRESULT CALLBACK Win32Application::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    DXSample* pSample = reinterpret_cast<DXSample*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_CREATE:
    {
        // Save the DXSample* passed in to CreateWindow.
        LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
    }
    return 0;

    case WM_KEYDOWN:
        if (pSample)
        {
            pSample->OnKeyDown(static_cast<UINT8>(wParam));
        }
        return 0;

    case WM_KEYUP:
        if (pSample)
        {
            pSample->OnKeyUp(static_cast<UINT8>(wParam));
        }

        switch (static_cast<UINT8>(wParam)) {
            case VK_ESCAPE:
                // Exit render thread
                m_exit = true;
                PostQuitMessage(0);
                return 0;
        }

        return 0;

    case WM_MOUSEWHEEL:
            pSample->OnMouseWheel(HIWORD(wParam));
        return 0;

    case WM_PAINT:
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);

        if (!m_firstPaint)
        {
            m_firstPaint = true;
            SetEvent(hWindowCreated);
        }
        return 0;

    case WM_DESTROY:
        // Exit render thread
        PostQuitMessage(0);
        return 0;
    }

    // Handle any messages the switch statement didn't.
    return DefWindowProc(hWnd, message, wParam, lParam);
}

