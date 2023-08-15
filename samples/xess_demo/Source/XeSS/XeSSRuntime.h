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

#include "xess/xess.h"
#include "xess/xess_d3d12.h"

class ColorBuffer;
class DepthBuffer;
class ComputeContext;

namespace XeSS
{
    /// Convert from xess_result_t to string.
    extern const char* ResultToString(xess_result_t result);

    /// Quality level of XeSS.
    enum eQualityLevel
    {
        kQualityPerformance = 0,
        kQualityBalanced,
        kQualityQuality,
        kQualityUltraQuality
    };

    /// Arguments for initialization of XeSS.
    struct InitArguments
    {
        /// Output buffer width.
        uint32_t OutputWidth;
        /// Output buffer height.
        uint32_t OutputHeight;
        /// Quality level of XeSS.
        eQualityLevel Quality;
        /// If we are in High-Res motion vectors mode.
        bool UseHiResMotionVectors;
        /// If motion vectors are jittered.
        bool UseJitteredMotionVectors;
        /// If motion vectors are in Normalized Device Coordinate (NDC).
        bool UseMotionVectorsInNDC;
        /// If use exposure texture.
        bool UseExposureTexture;
        /// If use responsive mask.
        bool UseResponsiveMask;
        /// If use auto exposure.
        bool UseAutoExposure;
        /// If enable GPU profiling.
        bool EnableProfiling;

        /// Constructor.
        InitArguments()
            : OutputWidth(0)
            , OutputHeight(0)
            , Quality(kQualityPerformance)
            , UseHiResMotionVectors(false)
            , UseJitteredMotionVectors(false)
            , UseMotionVectorsInNDC(false)
            , UseExposureTexture(false)
            , UseResponsiveMask(false)
            , UseAutoExposure(false)
            , EnableProfiling(false)
        {
        }
    };

    /// Arguments for execution of XeSS.
    struct ExecuteArguments
    {
        /// CommandList which is used to record XeSS commands.
        ID3D12GraphicsCommandList* CommandList;
        /// Width of input buffer.
        uint32_t InputWidth;
        /// Height of input buffer.
        uint32_t InputHeight;
        /// If we should reset history when executing XeSS.
        bool ResetHistory;

        /// Input color buffer.
        ColorBuffer* ColorTexture;
        /// Input motion vectors buffer.
        ColorBuffer* VelocityTexture;
        /// Output color buffer.
        ColorBuffer* OutputTexture;
        /// Input depth texture. (Used in low-res motion vectors mode.)
        ColorBuffer* DepthTexture;
        /// Input Exposure texture
        ColorBuffer* ExposureTexture;
        /// Input Responsive mask
        ColorBuffer* ResponsiveMask;

        /// Constructor.
        ExecuteArguments()
            : CommandList(nullptr)
            , InputWidth(0)
            , InputHeight(0)
            , ResetHistory(false)
            , ColorTexture(nullptr)
            , VelocityTexture(nullptr)
            , OutputTexture(nullptr)
            , DepthTexture(nullptr)
            , ExposureTexture(nullptr)
            , ResponsiveMask(nullptr)
        {
        }
    };

    struct PerfRecord 
    {
        double Time = 0.0;
        double Sum = 0.0;
        double Avg = 0.0;
        double Min = 0.0;
        double Max = 0.0;
        uint32_t Frames = 0;
        bool Dirty = false;

        void Reset()
        {
            Sum = Time;
            Avg = Time;
            Min = Time;
            Max = Time;
            Frames = 0;
            Dirty = false;
        }

        void Accumulate()
        {
            Frames++;
            Sum += Time;
            Avg = Sum / Frames;
            Max = max(Max, Time);
            Min = min(Min, Time);
            Dirty = false;
        }
    };

    using PerfPair = std::pair<std::string, PerfRecord>;
    using PerfData = std::unordered_map<std::string, PerfRecord>;
    constexpr size_t PERF_GRAPH_SIZE = 128;
    using PerfGraphData = std::array<float, PERF_GRAPH_SIZE>;

    /// A wrapper of XeSS SDK.
    class XeSSRuntime
    {
    public:
        /// Constructor.
        XeSSRuntime();

        /// Create XeSS context.
        bool CreateContext();
        /// Build pipeline of XeSS.
        bool InitializePipeline(uint32_t InitFlag, bool Blocking = false);
        // Destruction interface.
        void Shutdown(void);
        /// Return if runtime is initialized.
        bool IsInitialzed() const;
        /// Create the effect of XeSS.
        bool Initialize(const InitArguments& Args);
        /// Execute XeSS.
        void Execute(ExecuteArguments& ExeArgs);
        /// Get recommenced input resolution.
        bool GetInputResolution(uint32_t& Width, uint32_t& Height);
        /// Set jitter scale values.
        bool SetJitterScale(float X, float Y);
        /// Set velocity scale values.
        bool SetVelocityScale(float X, float Y);
        /// Get XeSS Context.
        xess_context_handle_t GetContext();
        /// Get version string.
        const std::string& GetVersionString();
        /// Get GPU profiling data.
        const PerfData& GetProfilingData() const { return m_DisplayPerfData_; }
        /// Set perf graph record to generate.
        void SetPerfGraphRecord(const std::string& RecordName);
        /// Get Perf graph data.
        const PerfGraphData& GetPerfGraphData() const { return m_PerfGraphData; }
        /// GPU side profiling using profiling API
        void DoGPUProfile();

        void ProcessPerfData();
    private:
        /// Save initialization arguments.
        void SetInitArguments(const InitArguments& Args);

        /// If pipeline is already built.
        bool m_PipelineBuilt;
        /// If blocking build?
        bool m_PipelineBuiltBlocking;
        /// Current pipeline built flag
        uint32_t m_PipelineBuiltFlag;

        /// If runtime is initialized.
        bool m_Initialized;

        /// Initialization arguments.
        InitArguments m_InitArguments;

        /// Context of XeSS.
        xess_context_handle_t m_Context;

        /// Version string.
        std::string m_VersionStr;

        /// If pipeline built.
        bool m_PipelineLibBuilt;

        /// DX12 pipeline library object.
        Microsoft::WRL::ComPtr<ID3D12PipelineLibrary> m_PipelineLibrary;

        /// Processed GPU profiling data.
        PerfData m_PerfData_;
        /// GPU profiling data being processed.
        PerfData m_DisplayPerfData_;
        // Frame count for perf data processing.
        float m_PerfAccumTime;
        // Current perf graphics record name.
        std::string m_PerfGraphRecordName;
        /// Perf graph data.
        PerfGraphData m_PerfGraphData;
    };
} // namespace XeSS
