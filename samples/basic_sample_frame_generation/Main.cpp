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

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
    try
    {
        BasicSample sample(1920, 1080, L"XeSS FG DX12 basic sample");
        return Win32Application::Run(&sample, hInstance, nCmdShow);
    }
    catch (const std::runtime_error& err)
    {
        MessageBoxA(NULL, (std::string("XeSS FG sample error: ") + err.what()).c_str(), "Error", MB_OK | MB_TOPMOST | MB_ICONINFORMATION);
        return 1;
    }
}
