/*******************************************************************************
 * Copyright 2021 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files(the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions :
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#pragma once
class ColorBuffer;
class GraphicsContext;
class Texture;
struct ImFont;

namespace ImGuiModule
{
    extern LRESULT WinProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    bool Initialize();

    bool IsIntialized();

    void Shutdown();

    void PreGUI();

    void PostGUI();

    void Render(GraphicsContext& Context);

    /// Add a new font, returns an index for looking up the ImFont pointer later.
    uint32_t AddFont(const std::wstring& FontPath, float Size, bool Merge);

    /// Get font by index.
    ImFont* GetFont(uint32_t Index);

    /// Register ColorBuffer used as image before it is referenced by ImGui
    void RegisterImage(ColorBuffer& image);

    /// Register Texture used as image before it is referenced by ImGui
    void RegisterImage(Texture& image);

    /// Return if ImGui wants to capture keyboard.
    bool WantCaptureKeyboard();

    /// Return if ImGui wants to capture mouse.
    bool WantCaptureMouse();

} // namespace ImGuiModule
