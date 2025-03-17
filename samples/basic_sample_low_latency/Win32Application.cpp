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

//#define _STATS
#ifdef _STATS
#include <algorithm>
#include <numeric>
#include <thread>
#define DBG_PRINTF(...) {char cad[512]; sprintf(cad, __VA_ARGS__);  OutputDebugStringA(cad);}
#endif

DWORD WINAPI RenderThread(LPVOID lpParam)
{
    auto pSample = static_cast<DXSample*>(lpParam);
    auto last_frame = std::chrono::high_resolution_clock::now();

#ifdef _STATS
    auto last_report = std::chrono::high_resolution_clock::now();
    std::list<float> frametime;
#endif

    try
    {
        pSample->OnInit();

        // Wait for window creation
        WaitForSingleObject(hWindowCreated, INFINITE);

        while (!m_exit)
        {
            // Allow the sample to pace/delay to minimize latency
            pSample->OnSleep();

            //std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Note: This is commented out as it causes large variance in frame times.  Without it we spin in this loop on the CPU
            auto current_time = std::chrono::high_resolution_clock::now();
            float duration = std::chrono::duration_cast<std::chrono::microseconds>(current_time - last_frame).count() / 1000.0f;
            if (duration >= pSample->m_frameMS)
            {
                last_frame = current_time;
#ifdef _STATS
                frametime.push_back(duration);
#endif
                pSample->OnUpdate();
                pSample->OnRender();
            }

#ifdef _STATS
            // Report stats once per second
            if (std::chrono::duration_cast<std::chrono::seconds>(current_time - last_report).count())
            {
                float min = *std::min_element(frametime.begin(), frametime.end());
                float average = std::accumulate(frametime.begin(), frametime.end(), 0.0f) / frametime.size();
                float max = *std::max_element(frametime.begin(), frametime.end());
                DBG_PRINTF("[Stats] Frame min:average:max %fms, %fms, %fms\n", min, average, max);
                frametime.clear();
                last_report = current_time;
            }
#endif
        }

        pSample->OnDestroy();
    }
    catch (const std::runtime_error& err)
    {
        m_exit = true;
        MessageBoxA(NULL, (std::string("XeLL sample error: ") + err.what()).c_str(), "Error", MB_OK | MB_TOPMOST | MB_ICONINFORMATION);
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
        AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

        // Create the window and store a handle to it.
        m_hwnd = CreateWindow(
            windowClass.lpszClassName,
            pSample->GetTitle(),
            WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_SIZEBOX),
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            windowRect.right - windowRect.left,
            windowRect.bottom - windowRect.top,
            nullptr,        // We have no parent window.
            nullptr,        // We aren't using menus.
            hInstance,
            pSample);
    }

    ShowWindow(m_hwnd, pSample->m_fullScreen ? SW_MAXIMIZE : nCmdShow);

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

    // Wait for render thread to stop
    if (hRenderThread != nullptr) {
        WaitForSingleObject(hRenderThread, INFINITE);
        CloseHandle(hRenderThread);
    }

    if (hWindowCreated != nullptr) {
        CloseHandle(hWindowCreated);
        hWindowCreated = nullptr;
    }

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
        m_exit = true;
        PostQuitMessage(0);
        return 0;
    }

    // Handle any messages the switch statement didn't.
    return DefWindowProc(hWnd, message, wParam, lParam);
}

