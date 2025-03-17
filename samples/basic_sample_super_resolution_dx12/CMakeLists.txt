################################################################################
# Copyright (C) 2021 Intel Corporation
# 
# This software and the related documents are Intel copyrighted materials, and
# your use of them is governed by the express license under which they were
# provided to you ("License"). Unless the License provides otherwise, you may
# not use, modify, copy, publish, distribute, disclose or transmit this
# software or the related documents without Intel's prior written permission.
# 
# This software and the related documents are provided as is, with no express
# or implied warranties, other than those that are expressly stated in the
# License.
###############################################################################

cmake_minimum_required(VERSION 3.22)

project(BasicSampleD3D12)

set(D3D12_SAMPLE_SOURCES
    basic_sample_d3d12.cpp
    basic_sample_d3d12.h
    DXSample.cpp
    DXSample.h
    DXSampleHelper.h
    Main.cpp
    Win32Application.cpp
    Win32Application.h
    d3dx12.h
    stdafx.cpp
    stdafx.h
    utils.cpp
    utils.h
)

set(D3D12_SAMPLE_RESOURCES README.md)

if (NOT XESS_BUILD_INTERNAL_SAMPLE)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
endif()

add_executable(BasicSampleD3D12 WIN32 ${D3D12_SAMPLE_SOURCES})

set_target_properties(BasicSampleD3D12 PROPERTIES RESOURCE "${D3D12_SAMPLE_RESOURCES}")

target_link_libraries(BasicSampleD3D12 PRIVATE d3d12 dxgi dxguid d3dcompiler)

if (NOT XESS_BUILD_INTERNAL_SAMPLE)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../inc)
    target_link_libraries(BasicSampleD3D12 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/libxess.lib)
else()
    target_link_libraries(BasicSampleD3D12 PRIVATE project_options project_warnings libxess)
endif()

target_compile_definitions(BasicSampleD3D12 PRIVATE UNICODE)
target_compile_features(BasicSampleD3D12 PRIVATE cxx_std_17)

# Copy shaders
if (MSVC AND (NOT (${CMAKE_GENERATOR} STREQUAL "Ninja")))
    set(DEST_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)
else ()
    set(DEST_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

add_custom_target(CopyBasicSampleD3D12RequiredFiles ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/shader_xess_sr_d3d12.hlsl "${DEST_DIR}/basic_sample_shaders/hlsl/shader_xess_sr_d3d12.hlsl"
)

if (NOT XESS_BUILD_INTERNAL_SAMPLE)

    set(SRC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../bin/")
    add_custom_target(CopyRequiredDLLs ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_PATH}/libxess.dll" "${DEST_DIR}/libxess.dll"
    )
else ()
    install(TARGETS BasicSampleD3D12
            RUNTIME DESTINATION bin
            ARCHIVE DESTINATION lib
            PUBLIC_HEADER DESTINATION inc
            RESOURCE DESTINATION bin
    )
endif()
