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

#include "pch.h"
#include "DemoGui.h"
#include "DemoApp.h"
#include "ImGuiModule.h"
#include "GraphicsCore.h"
#include "Display.h"
#include "TemporalEffects.h"
#include "ParticleEffectManager.h"
#include "PostEffects.h"
#include "SSAO.h"
#include "MotionBlur.h"
#include "BufferManager.h"
#include "XeSS/XeSSProcess.h"
#include "XeSS/XeSSDebug.h"
#include "imgui.h"
#include "DemoCameraController.h"
#include "Renderer.h"

#include <iterator>

using namespace Graphics;
using namespace XeSS;

namespace DemoGui
{
    static const ImVec4 DEF_TEXT_COLOR(0.94f, 0.94f, 0.94f, 1.00f);
    static const ImVec4 DEF_TEXT_GREY(0.5f, 0.5f, 0.5f, 1.00f);
    static const ImVec4 DEF_GRAPH_GREEN(0.0f, 1.0f, 0.0f, 1.00f);

    bool m_ShowUI = true;
    bool m_ShowLog = false;
    bool m_ShowInspectInput = false;
    bool m_ShowInspectOutput = false;
    bool m_ShowInspectMotionVectors = false;
    bool m_ShowInspectDepth = false;
    bool m_ShowInspectResponsiveMask = false;

    float m_DPIScale = 1.0f;

    inline float DESIGN_SIZE(float size);
    inline ImVec2 DESIGN_SIZE(const ImVec2& size);
    void DPISetNextWindowRect(bool& adjust, const ImVec4& winRect);
    void DPIGetWindowRect(bool& resize, ImVec4& winRect);

    void OnGUI_XeSS();
    void OnGUI_Profiling();
    void OnGUI_Debug();
    void OnGUI_Rendering();
    void OnGUI_Camera(DemoApp& App);
    void OnGUI_DebugBufferWindows();
    void OnGUI_LogWindow(DemoApp& Log);

    class BufferViewer
    {
    public:
        BufferViewer(const std::string& title)
            : m_Title(title) { }
        void OnGUI(bool& Show, ColorBuffer& Buffer);

    private:
        std::string m_Title;
        int m_ZoomIndex = 0;
        ImVec2 m_ImagePos;
        ImVec2 m_LastMousePos;
        bool m_ScrollDirty = false;
        bool m_DPIAdjust = false;
        ImVec4 m_DPIWinRect;
    };
} // namespace DemoGui

float DemoGui::DESIGN_SIZE(float size)
{
    // We use DPI scale of 1.5 for the design.
    float scale = Display::GetDPIScale() / 1.5f;
    return size * scale;
}

ImVec2 DemoGui::DESIGN_SIZE(const ImVec2& size)
{
    // We use DPI scale of 1.5 for the design.
    float scale = Display::GetDPIScale() / 1.5f;
    return ImVec2(size.x * scale, size.y * scale);
}

void DemoGui::DPISetNextWindowRect(bool& adjust, const ImVec4& winRect)
{
    if (!adjust)
        return;

    ImGui::SetNextWindowPos(ImVec2(winRect.x, winRect.y));
    ImGui::SetNextWindowSize(ImVec2(winRect.z, winRect.w));

    adjust = false;

    LOG_DEBUG("DPI Set next GUI window rect for DPI change.");
}

void DemoGui::DPIGetWindowRect(bool& adjust, ImVec4& winRect)
{
    float newDPIScale = Display::GetDPIScale();
    adjust = newDPIScale != m_DPIScale;
    if (!adjust)
        return;

    float scale = newDPIScale / m_DPIScale;
    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();

    winRect = ImVec4(pos.x * scale, pos.y * scale, size.x * scale, size.y * scale);

    LOG_DEBUG("Get next GUI window rect for DPI change.");
}

bool DemoGui::Initialize()
{
    ImGui::GetIO().IniFilename = nullptr;

    std::wstring programDir = Utility::GetProgramDirectory();
    std::wstring fontPath = programDir + L"DroidSans.ttf";
    ImGuiModule::AddFont(fontPath, 15.0f, false);

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.04f, 0.06f, 0.08f, 0.77f);
    colors[ImGuiCol_Header] = ImVec4(0.13f, 0.35f, 0.55f, 0.87f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.20f, 0.29f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.46f, 0.80f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.09f, 0.12f, 0.14f, 0.94f);
    colors[ImGuiCol_Text] = DEF_TEXT_COLOR;

    m_DPIScale = Display::GetDPIScale();

    return true;
}

void DemoGui::Shutdown()
{
}

void DemoGui::OnGUI(DemoApp& App)
{
    // F1 key to toggle GUI.
    if (ImGui::IsKeyPressed(VK_F1))
    {
        m_ShowUI = !m_ShowUI;
    }

    if (!m_ShowUI)
        return;

    ImGui::SetNextWindowPos(DESIGN_SIZE(ImVec2(10, 10)), ImGuiCond_Once);
    ImGui::SetNextWindowSize(DESIGN_SIZE(ImVec2(550, 1100)), ImGuiCond_Once);

    static bool adjust = false;
    static ImVec4 winRect;
    DPISetNextWindowRect(adjust, winRect);

    if (ImGui::Begin("Options (F1 to toggle)", &m_ShowUI))
    {
        DPIGetWindowRect(adjust, winRect);

        ImGui::Text("Adapter: %s", g_AdapterName.c_str());

        ImGui::Text("CPU %7.3f ms, GPU %7.3f ms, %3u Hz",
            EngineProfiling::GetTotalCpuTime(), EngineProfiling::GetTotalGpuTime(), (uint32_t)(EngineProfiling::GetFrameRate() + 0.5f));

        ImGui::Separator();

        ImGui::Text("Output: %ux%u", g_DisplayWidth, g_DisplayHeight);
        ImGui::Text("Input: %ux%u", g_NativeWidth, g_NativeHeight);

        ImGui::Separator();

        bool fullscreen = Display::IsFullscreen();
        if (ImGui::Checkbox(Display::g_SupportTearing ? "Fullscreen" : "Borderless", &fullscreen))
        {
            Display::SetFullscreen(fullscreen);
        }

        ImGui::SameLine();

        bool vsync = Display::s_EnableVSync;
        if (ImGui::Checkbox("VSync", &vsync))
        {
            Display::s_EnableVSync = vsync;
        }

        ImGui::Separator();

        if (ImGui::CollapsingHeader("Technique##Header", ImGuiTreeNodeFlags_DefaultOpen))
        {
            static const char* TECHNIQUE_NAMES[] = { "XeSS", "TAA Without Upscaling", "TAA With Simple Upscaling" };

            int upscalingIndex = App.GetTechnique();
            if (ImGui::Combo("Technique", &upscalingIndex, TECHNIQUE_NAMES, IM_ARRAYSIZE(TECHNIQUE_NAMES)))
            {
                App.SetTechnique(static_cast<eDemoTechnique>(upscalingIndex));
            }

            // XeSS selected
            if (upscalingIndex == kDemoTech_XeSS)
            {
                OnGUI_XeSS();
            }
            else
            {
                float sharpness = TemporalEffects::Sharpness;
                if (ImGui::DragFloat("Sharpness", &sharpness, 0.01f, 0.0f, 1.0f, "%.2f"))
                {
                    TemporalEffects::Sharpness = sharpness;
                }
            }
        }

        if (ImGui::CollapsingHeader("Scene Rendering", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // Scene rendering GUI.
            OnGUI_Rendering();
        }

        if (ImGui::CollapsingHeader("Camera"))
        {
            OnGUI_Camera(App);
        }

        if (ImGui::CollapsingHeader("Other##Header", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Show Log", &m_ShowLog);
        }

        ImGui::Separator();

        ImGui::Text("SDK Version: %s", XeSS::g_XeSSRuntime.GetVersionString().c_str());
    }

    ImGui::End();

    // Show buffer viewers only when XeSS technique is used.
    if (App.GetTechnique() == kDemoTech_XeSS)
    {
        // Buffer viewer windows
        OnGUI_DebugBufferWindows();

        OnGUI_Profiling();
    }

    // Log window.
    OnGUI_LogWindow(App);

    // Update DPI scale for this frame.
    m_DPIScale = Display::GetDPIScale();
}

void DemoGui::OnGUI_XeSS()
{
    if (!XeSS::IsSupported())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
        ImGui::Text("XeSS is not supported on this device.");
        ImGui::PopStyleColor();
        return;
    }

    float sharpness = XeSS::Sharpness;
    if (ImGui::DragFloat("Sharpness", &sharpness, 0.01f, 0.0f, 1.0f, "%.2f"))
    {
        XeSS::Sharpness = sharpness;
    }

    static const char* QUALITY_NAMES[] = { "Performance", "Balanced", "Quality", "Ultra Quality" };
    int32_t quality = XeSS::GetQuality();
    if (ImGui::Combo("Quality", &quality, QUALITY_NAMES, IM_ARRAYSIZE(QUALITY_NAMES)))
    {
        XeSS::SetQuality(static_cast<XeSS::eQualityLevel>(quality));
    }

    bool dynRes = XeSS::IsDynResEnabled();
    if (ImGui::Checkbox("Dynamic Resolution", &dynRes))
    {
        XeSS::SetDynResEnabled(dynRes);
    }

    if (dynRes)
    {
        float upscale = XeSS::GetUpscaleFactor();
        if (ImGui::DragFloat("Upscale Factor", &upscale, 0.01f, 1.0f, 2.0f, "%.2f"));
        {
            XeSS::SetUpscaleFactor(upscale);
        }
    }

    static const char* VELOCITY_MODE_NAMES[] = { "High-Res", "Low-Res" };
    int MVMode = XeSS::GetMotionVectorsMode();
    if (ImGui::Combo("Motion Vectors", &MVMode, VELOCITY_MODE_NAMES, IM_ARRAYSIZE(VELOCITY_MODE_NAMES)))
    {
        XeSS::SetMotionVectorsMode(static_cast<XeSS::eMotionVectorsMode>(MVMode));
    }

    bool jitteredMV = XeSS::IsMotionVectorsJittered();
    if (ImGui::Checkbox("Jittered Motion Vectors", &jitteredMV))
    {
        XeSS::SetMotionVectorsJittered(jitteredMV);
    }

    bool MVinNDC = XeSS::IsMotionVectorsInNDC();
    if (ImGui::Checkbox("NDC Motion Vectors", &MVinNDC))
    {
        XeSS::SetMotionVectorsInNDC(MVinNDC);
    }

    int mipBiasMode = XeSS::GetMipBiasMode();
    static const char* MIPBIAS_METHOD_NAMES[] = { "Recommended", "Customized" };
    if (ImGui::Combo("Mip Bias", &mipBiasMode, MIPBIAS_METHOD_NAMES, IM_ARRAYSIZE(MIPBIAS_METHOD_NAMES)))
    {
        XeSS::SetMipBiasMode(static_cast<eMipBiasMode>(mipBiasMode));
    }

    if (mipBiasMode == 0) // Recommended
        ImGui::BeginDisabled(true);

    float mipBias = XeSS::GetMipBias();
    if (ImGui::DragFloat("Mip Bias Value", &mipBias, 0.02f, -16.0f, 15.99f, "%.03f"))
    {
        XeSS::SetMipBias(mipBias);
    }
    if (mipBiasMode == 0) // Recommended
        ImGui::EndDisabled();

    bool isResponsiveMaskEnabled = XeSS::IsResponsiveMaskEnabled();
    if (ImGui::Checkbox("Responsive Mask", &isResponsiveMaskEnabled))
    {
        XeSS::SetResponsiveMaskEnabled(isResponsiveMaskEnabled);
    }

    bool isAutoExposureEnabled = XeSS::IsAutoExposureEnabled();
    if (ImGui::Checkbox("Auto Exposure", &isAutoExposureEnabled))
    {
        XeSS::SetAutoExposureEnabled(isAutoExposureEnabled);
    }

    if (ImGui::TreeNodeEx("Debug" /*, ImGuiTreeNodeFlags_DefaultOpen*/))
    {
        OnGUI_Debug();
        ImGui::TreePop();
    }
}

void DemoGui::OnGUI_Profiling()
{
    bool profilingEnabled = XeSS::IsProfilingEnabled();
    if (!profilingEnabled)
        return;

    constexpr auto WIDTH = 800.0f;
    constexpr auto HEIGHT = 700.0f;

    ImGui::SetNextWindowPos(DESIGN_SIZE(ImVec2(ImGui::GetIO().DisplaySize.x - WIDTH - 10, 10)), ImGuiCond_Once);
    ImGui::SetNextWindowSize(DESIGN_SIZE(ImVec2(WIDTH, HEIGHT)), ImGuiCond_Once);

    static bool adjust = false;
    static ImVec4 winRect;
    DPISetNextWindowRect(adjust, winRect);

    // Name of the total GPU time record.
    constexpr auto TOTAL_RECORD_NAME = "Execute";

    // Current selected record.
    static std::string selectedRecord;
    if (!selectedRecord.length())
    {
        selectedRecord = TOTAL_RECORD_NAME;
        XeSS::g_XeSSRuntime.SetPerfGraphRecord(selectedRecord);
    }

    if (ImGui::Begin("GPU Profiling Data", &profilingEnabled))
    {
        DPIGetWindowRect(adjust, winRect);

        static bool showAsMicroSec = true;

        constexpr auto TEXT_AS_MICRO = "Use Microseconds";
        constexpr auto TEXT_AS_MILLI = "Use Milliseconds";

        if (ImGui::Button(showAsMicroSec ? TEXT_AS_MILLI : TEXT_AS_MICRO))
        {
            showAsMicroSec = !showAsMicroSec;
        }

        ImGui::Separator();

        // Draw GPU duration text.
        auto DrawTimeText = [=](double time)
        {
            // Time
            if (showAsMicroSec)
                ImGui::Text("%.2f ", time * 1000.0 * 1000.0);
            else
                ImGui::Text("%.2f ", time * 1000.0);

            // Unit
            ImGui::PushStyleColor(ImGuiCol_Text, DEF_TEXT_GREY);
            ImGui::SameLine(0, 0);
            ImGui::TextUnformatted(showAsMicroSec ? "us" : "ms");
            ImGui::PopStyleColor();
        };


        auto& perfData = XeSS::g_XeSSRuntime.GetProfilingData();

        std::vector<PerfPair> displayData;
        std::transform(perfData.begin(), perfData.end(),
            std::back_inserter(displayData), [](const auto& elem)
            { return elem; });

        enum ColumType
        {
            kColRecord = 0,
            kColTime,
            kColAvg,
            kColMin,
            kColMax,
            kColNum
        };

        constexpr ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingStretchSame;
        const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

        if (ImGui::BeginTable("Profiling Table", kColNum, flags, ImVec2(0.0f, TEXT_BASE_HEIGHT * displayData.size() + 2)))
        {
            ImGui::TableSetupScrollFreeze(0, 1);

            constexpr ImGuiTableColumnFlags COLUMN_FLAGS = ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch;
            ImGui::TableSetupColumn("Record", COLUMN_FLAGS, 0, kColRecord);
            ImGui::TableSetupColumn("GPU Duration", COLUMN_FLAGS, 0, kColTime);
            ImGui::TableSetupColumn("Avg", COLUMN_FLAGS, 0, kColAvg);
            ImGui::TableSetupColumn("Min", COLUMN_FLAGS, 0, kColMin);
            ImGui::TableSetupColumn("Max", COLUMN_FLAGS, 0, kColMax);
            ImGui::TableHeadersRow();

            // Sort
            ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs();
            if (sort_specs)
            {
                if (displayData.size() > 1)
                {
                    std::sort(displayData.begin(), displayData.end(),
                        [&](const auto& a, const auto& b)
                        {
                            // Make "Execute" record pinned on the top.
                            if (a.first == TOTAL_RECORD_NAME)
                                return true;
                            else if (b.first == TOTAL_RECORD_NAME)
                                return false;

                            // Sort rest of the entries.
                            for (int i = 0; i < sort_specs->SpecsCount; ++i)
                            {
                                const auto& spec = sort_specs->Specs[i];

                                bool flag = true;
                                bool decend = spec.SortDirection == ImGuiSortDirection_Descending;
                                switch (spec.ColumnUserID)
                                {
                                case kColRecord:
                                    flag = decend ? (a.first > b.first) : (a.first < b.first);
                                    break;
                                case kColTime:
                                    flag = decend ? (a.second.Time > b.second.Time) : (a.second.Time < b.second.Time);
                                    break;
                                case kColAvg:
                                    flag = decend ? (a.second.Avg > b.second.Avg) : (a.second.Avg < b.second.Avg);
                                    break;
                                case kColMin:
                                    flag = decend ? (a.second.Min > b.second.Min) : (a.second.Min < b.second.Min);
                                    break;
                                case kColMax:
                                    flag = decend ? (a.second.Max > b.second.Max) : (a.second.Max < b.second.Max);
                                    break;
                                default:
                                    assert(0 && "Invalid table column for sort");
                                    break;
                                }

                                return flag;
                            }

                            return a.first < b.first;
                        });
                }
            }

            // Draw
            ImGuiListClipper clipper;
            clipper.Begin((int)displayData.size());
            while (clipper.Step())
            {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
                {
                    auto& it = displayData[i];
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();

                    bool selected = (selectedRecord == it.first);
                    if (ImGui::Selectable(it.first.c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap))
                    {
                        if (selected)
                        {
                            selectedRecord = it.first;
                            XeSS::g_XeSSRuntime.SetPerfGraphRecord(selectedRecord);
                        }
                    }

                    // Time
                    ImGui::TableNextColumn();
                    DrawTimeText(it.second.Time);

                    // Avg
                    ImGui::TableNextColumn();
                    DrawTimeText(it.second.Avg);

                    // Min
                    ImGui::TableNextColumn();
                    DrawTimeText(it.second.Min);

                    // Max
                    ImGui::TableNextColumn();
                    DrawTimeText(it.second.Max);
                }
            }

            ImGui::EndTable();
        }

        // Time graph
        if (selectedRecord.length())
        {
            auto& graphData = XeSS::g_XeSSRuntime.GetPerfGraphData();

            float max_height = *std::max_element(graphData.begin(), graphData.end()) * 1.2f;
            const float width = ImGui::GetContentRegionAvailWidth();

            ImGui::PushStyleColor(ImGuiCol_PlotLines, DEF_GRAPH_GREEN);

            std::string lable ="GPU Duration of " + selectedRecord + " (us)";
            ImGui::PlotLines("##gpu_graph", graphData.data(), graphData.size(), 0, 
                lable.c_str(), 0.0f, max_height, ImVec2(width, 70));
            ImGui::PopStyleColor();
        }
    }

    ImGui::End();

    // When window is closed, we disable profiling.
    if (!profilingEnabled)
    {
        XeSS::SetProfilingEnabled(false);
    }
}

void DemoGui::OnGUI_Debug()
{
    bool gpuProfiling = XeSS::IsProfilingEnabled();
    if (ImGui::Checkbox("GPU Profiling", &gpuProfiling))
    {
        XeSS::SetProfilingEnabled(gpuProfiling);
    }

    if (ImGui::Button("Reset History"))
    {
        XeSS::ResetHistory();
    }

    static const char* NETWORK_NAMES[] = {
        "KPSS",
        "2",
        "3",
        "4",
        "5",
        "6"
    };

    int networkModel = XeSSDebug::GetNetworkModel();
    if (ImGui::Combo("Network Model", &networkModel, NETWORK_NAMES, IM_ARRAYSIZE(NETWORK_NAMES)))
    {
        XeSSDebug::SelectNetworkModel(networkModel);
    }

    ImGui::PushItemWidth(DESIGN_SIZE(150.0f));

    static float JitterScaleX = 1.0f;
    static float JitterScaleY = 1.0f;

    bool jitterScaleDirty = false;
    if (ImGui::DragFloat("Jitter Scale X", &JitterScaleX, 0.1f, -16.0f, 16.0f, "%.1f"))
        jitterScaleDirty = true;

    //ImGui::SameLine();
    if (ImGui::DragFloat("Jitter Scale Y", &JitterScaleY, 0.1f, -16.0f, 16.0f, "%.1f"))
        jitterScaleDirty = true;

    if (jitterScaleDirty)
    {
        XeSS::g_XeSSRuntime.SetJitterScale(JitterScaleX, JitterScaleY);
    }

    static float VelocityScaleX = 1.0f;
    static float VelocityScaleY = 1.0f;

    bool velocityScaleDirty = false;
    if (ImGui::DragFloat("Velocity Scale X", &VelocityScaleX, 0.1f, -16.0f, 16.0f, "%.1f"))
        velocityScaleDirty = true;

    //ImGui::SameLine();
    if (ImGui::DragFloat("Velocity Scale Y", &VelocityScaleY, 0.1f, -16.0f, 16.0f, "%.1f"))
        velocityScaleDirty = true;

    if (velocityScaleDirty)
    {
        XeSS::g_XeSSRuntime.SetVelocityScale(VelocityScaleX, VelocityScaleY);
    }

    ImGui::PopItemWidth();

    // Buffer view options

    ImGui::Checkbox("Inspect Input", &m_ShowInspectInput);
    ImGui::Checkbox("Inspect Output", &m_ShowInspectOutput);
    ImGui::Checkbox("Inspect Motion Vectors", &m_ShowInspectMotionVectors);

    // Update the XeSS buffer debugging.
    if (m_ShowInspectOutput != XeSSDebug::IsBufferDebugEnabled())
    {
        XeSSDebug::SetBufferDebugEnabled(m_ShowInspectOutput);
    }

    if (XeSS::GetMotionVectorsMode() == XeSS::kMotionVectorsLowRes)
    {
        ImGui::Checkbox("Inspect Depth", &m_ShowInspectDepth);
    }
    else
    {
        m_ShowInspectDepth = false;
    }

    if (XeSS::IsResponsiveMaskEnabled())
    {
        ImGui::Checkbox("Inspect Responsive Mask", &m_ShowInspectResponsiveMask);
    }
    else
    {
        m_ShowInspectResponsiveMask = false;
    }

    if (ImGui::TreeNode("Frame Dump"))
    {
        bool isDumping = XeSSDebug::IsFrameDumpOn();

        if (isDumping)
            ImGui::BeginDisabled(true);

        static bool reset_history = false;
        ImGui::Checkbox("Reset History Before Dump", &reset_history);

        if (ImGui::Button("Dump Static Frames"))
        {
            if (reset_history)
            {
                XeSS::ResetHistory();
            }

            XeSSDebug::BeginFrameDump(false);
        }
        if (isDumping)
            ImGui::EndDisabled();

        if (XeSSDebug::IsFrameDumpOn() && !XeSSDebug::IsDumpDynamic())
        {
            ImGui::SameLine();
            ImGui::Text("Dumping Frame: %u/32...", XeSSDebug::GetDumpFrameIndex());
        }

        if (isDumping)
            ImGui::BeginDisabled(true);
        if (ImGui::Button("Dump Dynamic Frames"))
        {
            if (reset_history)
            {
                XeSS::ResetHistory();
            }

            XeSSDebug::BeginFrameDump(true);
        }
        if (isDumping)
            ImGui::EndDisabled();

        if (XeSSDebug::IsFrameDumpOn() && XeSSDebug::IsDumpDynamic())
        {
            ImGui::SameLine();
            ImGui::Text("Dumping Frame: %u/32...", XeSSDebug::GetDumpFrameIndex());
        }

        ImGui::TreePop();
    }
}

void DemoGui::OnGUI_DebugBufferWindows()
{
    if (m_ShowInspectInput)
    {
        static BufferViewer viewer("Input Buffer");
        viewer.OnGUI(m_ShowInspectInput, g_SceneColorBuffer);
    }

    if (m_ShowInspectOutput)
    {
        ASSERT(XeSSDebug::IsBufferDebugEnabled());
        static BufferViewer viewer("Output Buffer");
        viewer.OnGUI(m_ShowInspectOutput, XeSSDebug::g_DebugBufferOutput);
    }

    if (m_ShowInspectMotionVectors)
    {
        static BufferViewer viewer("Motion Vectors Buffer");
        if (XeSS::GetMotionVectorsMode() == XeSS::kMotionVectorsHighRes)
            viewer.OnGUI(m_ShowInspectMotionVectors, g_UpscaledVelocityBuffer);
        else
            viewer.OnGUI(m_ShowInspectMotionVectors, g_ConvertedVelocityBuffer);
    }

    if (m_ShowInspectDepth)
    {
        static BufferViewer viewer("Depth Buffer");
        ColorBuffer& depthBuffer = g_LinearDepth[XeSS::GetFrameIndexMod2()];
        viewer.OnGUI(m_ShowInspectDepth, depthBuffer);
    }

    if (m_ShowInspectResponsiveMask)
    {
        static BufferViewer viewer("Responsive Mask Buffer");
        viewer.OnGUI(m_ShowInspectResponsiveMask, g_ResponsiveMaskBuffer);
    }
}

void DemoGui::OnGUI_Rendering()
{
    bool enableParticle = ParticleEffectManager::Enable;
    if (ImGui::Checkbox("Particle", &enableParticle))
    {
        ParticleEffectManager::Enable = enableParticle;
    }

    bool enableBloom = PostEffects::BloomEnable;
    if (ImGui::Checkbox("Bloom", &enableBloom))
    {
        PostEffects::BloomEnable = enableBloom;
    }

    bool enableMotionBlur = MotionBlur::Enable;
    if (ImGui::Checkbox("Motion Blur", &enableMotionBlur))
    {
        MotionBlur::Enable = enableMotionBlur;
    }

    bool enableSSAO = SSAO::Enable;
    if (ImGui::Checkbox("SSAO", &enableSSAO))
    {
        SSAO::Enable = enableSSAO;
    }
}

void DemoGui::OnGUI_Camera(DemoApp& App)
{
    DemoCameraController* controller = static_cast<DemoCameraController*>(App.GetCameraController());
    ASSERT(controller);
    if (!controller)
        return;

    float yaw, pitch;
    Vector3 pos;
    controller->GetHeadingPitchAndPosition(yaw, pitch, pos);

    {
        ImGui::PushItemWidth(DESIGN_SIZE(150.0f));
        bool dirty = false;
        if (ImGui::DragFloat("Yaw", &yaw, 0.01f, -FLT_MAX, FLT_MAX))
            dirty = true;

        ImGui::SameLine();
        if (ImGui::DragFloat("Pitch", &pitch, 0.01f, -FLT_MAX, FLT_MAX))
            dirty = true;

        if (dirty)
        {
            controller->SetHeadingAndPitch(yaw, pitch);
        }
    }

    {
        bool dirty = false;
        float x = pos.GetX();
        if (ImGui::DragFloat("X", &x, 0.1f, -FLT_MAX, FLT_MAX))
            dirty = true;

        ImGui::SameLine();

        float y = pos.GetY();
        if (ImGui::DragFloat("Y", &y, 0.1f, -FLT_MAX, FLT_MAX))
            dirty = true;

        ImGui::SameLine();
        float z = pos.GetZ();
        if (ImGui::DragFloat("Z", &z, 0.1f, -FLT_MAX, FLT_MAX))
            dirty = true;

        if (dirty)
        {
            controller->SetPosition(Vector3(x, y, z));
        }

        ImGui::PopItemWidth();
    }
}

void DemoGui::OnGUI_LogWindow(DemoApp& App)
{
    if (!m_ShowLog)
        return;

    static const char* LEVEL_NAMES[] = { "[DEBUG]", "[INFO]", "[WARN]", "[ERROR]" };
    static const ImVec4 LEVEL_COLORS[] = { ImVec4(0, 1, 0, 1), DEF_TEXT_COLOR, ImVec4(1, 1, 0, 1), ImVec4(1, 0, 0, 1) };

    ImGui::SetNextWindowPos(DESIGN_SIZE(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y - 300.0f)), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(DESIGN_SIZE(ImVec2(1000, 300)), ImGuiCond_Once);

    static bool adjust = false;
    static ImVec4 winRect;
    DPISetNextWindowRect(adjust, winRect);

    if (ImGui::Begin("Log", &m_ShowLog))
    {
        DPIGetWindowRect(adjust, winRect);

        auto& messages = App.GetLog().GetMessages();

        ImGui::BeginChild("Log scroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(messages.size()));

        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
            {
                const Log::LogMessage& message = messages[i];
                ImGui::TextColored(LEVEL_COLORS[message.m_Level], "%s %s", LEVEL_NAMES[message.m_Level], message.m_Message.c_str());
            }
        }
        ImGui::PopStyleVar();

        ImGui::EndChild();
    }

    ImGui::End();
}

void DemoGui::BufferViewer::OnGUI(bool& Show, ColorBuffer& Buffer)
{
    DPISetNextWindowRect(m_DPIAdjust, m_DPIWinRect);
    ImGui::SetNextWindowSize(DESIGN_SIZE(ImVec2(Buffer.GetWidth() * 0.25f + 100.0f,
                                 Buffer.GetHeight() * 0.25f + 100.0f)),
        ImGuiCond_Once);

    if (ImGui::Begin(m_Title.c_str(), &Show))
    {
        DPIGetWindowRect(m_DPIAdjust, m_DPIWinRect);

        ImGui::Text("Resolution: %ux%u", Buffer.GetWidth(), Buffer.GetHeight());

        ImGui::SameLine();
        float left = ImGui::GetContentRegionMax().x - DESIGN_SIZE(300);
        ImGui::SetCursorPosX(left);
        ImGui::PushItemWidth(DESIGN_SIZE(200));

        static const char* ZOOM_NAMES[] = { "1/4", "1/2", "1", "2", "4", "8", "16" };
        static const int MAX_ZOOM_INDEX = IM_ARRAYSIZE(ZOOM_NAMES) - 1;
        ImGui::Combo("Zoom", &m_ZoomIndex, ZOOM_NAMES, IM_ARRAYSIZE(ZOOM_NAMES));
        ImGui::PopItemWidth();

        float scale = Math::Pow(2.0f, (float)m_ZoomIndex - 2.0f);
        const ImVec2 imageSize = ImVec2(static_cast<float>(Buffer.GetWidth()) * scale, static_cast<float>(Buffer.GetHeight()) * scale);

        ImGui::BeginChild("Image Viewport", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        {
            ImGuiModule::RegisterImage(Buffer);

            // Image button shows our buffer.
            ImGui::ImageButton(&Buffer, imageSize, ImVec2(0, 0), ImVec2(1, 1), 0);

            if (!m_ScrollDirty)
            {
                m_ImagePos = ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY());
            }
            else
            {
                // We scroll the image to its place here.
                ImGui::SetScrollX(m_ImagePos.x);
                ImGui::SetScrollY(m_ImagePos.y);

                m_ScrollDirty = false;
            }

            if (ImGui::IsItemHovered())
            {
                const ImVec2 cursorPosInImage = ImVec2(ImGui::GetMousePos().x - ImGui::GetCursorScreenPos().x,
                    ImGui::GetMousePos().y - ImGui::GetCursorScreenPos().y + imageSize.y);

                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

                ImVec2 mousePos = ImGui::GetMousePos();

                // Handle mouse drag.
                if (ImGui::IsAnyMouseDown())
                {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                    ImVec2 mouseOffset(mousePos.x - m_LastMousePos.x, mousePos.y - m_LastMousePos.y);

                    m_ImagePos.x -= mouseOffset.x;
                    m_ImagePos.y -= mouseOffset.y;

                    m_ScrollDirty = true;
                }

                // Handle mouse wheel.
                float wheel = ImGui::GetIO().MouseWheel;
                int zoom = m_ZoomIndex;
                if (wheel > 0.0f)
                {
                    zoom++;
                    zoom = (zoom > MAX_ZOOM_INDEX) ? MAX_ZOOM_INDEX : zoom;
                }
                else if (wheel < 0.0f)
                {
                    zoom--;
                    zoom = (zoom < 0) ? 0 : zoom;
                }

                if (zoom != m_ZoomIndex)
                {
                    // Here we want to translate the image so the cursor still points to the same place.
                    float ratio = Math::Pow(2.0f, (float)(zoom - m_ZoomIndex));

                    float cursorPosInViewportX = cursorPosInImage.x - m_ImagePos.x;
                    float cursorPosInViewportY = cursorPosInImage.y - m_ImagePos.y;

                    m_ImagePos.x = cursorPosInImage.x * ratio - cursorPosInViewportX;
                    m_ImagePos.y = cursorPosInImage.y * ratio - cursorPosInViewportY;

                    m_ScrollDirty = true;

                    m_ZoomIndex = zoom;
                }

                m_LastMousePos = ImGui::GetMousePos();
            }
        }

        ImGui::EndChild();
        ImGui::End();
        return;
    }

    ImGui::End();
}
