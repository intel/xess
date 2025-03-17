################################################################################
# Copyright (c) 2025 Intel Corporation
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
################################################################################

cmake_minimum_required(VERSION 3.22)
project(basic_xess_fg_sample)

set(SOURCES
    basic_sample.cpp
    basic_sample.h
    DXSample.cpp
    DXSample.h
    DXSampleHelper.h
    Main.cpp
    Win32Application.cpp
    Win32Application.h
    stdafx.cpp
    stdafx.h
    d3dx12.h
)

if (NOT XEFG_BUILD_INTERNAL_SAMPLE)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
endif()

set(RESOURCES README.md)

add_executable(basic_xess_fg_sample WIN32 ${SOURCES})
set_target_properties(basic_xess_fg_sample PROPERTIES RESOURCE "${RESOURCES}")
target_link_libraries(basic_xess_fg_sample PRIVATE d3d12 dxgi dxguid d3dcompiler)

target_compile_definitions(basic_xess_fg_sample PRIVATE UNICODE)

if (NOT XEFG_BUILD_INTERNAL_SAMPLE)
    set(XELL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../..")
    
    include_directories(
        ${XELL_PATH}/inc/xess_fg
        ${XELL_PATH}/inc/xell
    )
    target_link_libraries(basic_xess_fg_sample PRIVATE
        ${XELL_PATH}/lib/libxess_fg.lib
        ${XELL_PATH}/lib/libxell.lib
    )
else()
    set(SRC_AGILITY_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/dxsdk")
    set(XELL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/libxell")

    target_link_libraries(basic_xess_fg_sample PRIVATE project_options libxess_fg ${XELL_PATH}/lib/libxell.lib)

    target_include_directories(basic_xess_fg_sample PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../libxefg/include/xefg
        ${SRC_AGILITY_SDK_PATH}/include/d3dx12
        ${XELL_PATH}/inc/xell
    )
endif()

# Copy shaders
if (MSVC AND (NOT (${CMAKE_GENERATOR} STREQUAL "Ninja")))
    set(DEST_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)
else ()
    set(DEST_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

add_custom_target(basic_xess_fg_sample_RequiredFiles ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/shader_xess_fg.hlsl "${DEST_DIR}/basic_sample_shaders/hlsl/shader_xess_fg.hlsl"
        # Copy XeLR
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${XELL_PATH}/bin/libxell.dll" "${DEST_DIR}/libxell.dll"
)

set_target_properties(basic_xess_fg_sample_RequiredFiles
        PROPERTIES
        FOLDER "Copies"
)

if(DEFINED SRC_AGILITY_SDK_PATH)
add_custom_target(basic_xess_fg_sample_AgilitySDKFiles ALL
        # Copy Agility DX SDK
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_AGILITY_SDK_PATH}/bin/x64/D3D12Core.dll" "${DEST_DIR}/d3d12/D3D12Core.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_AGILITY_SDK_PATH}/bin/x64/d3d12SDKLayers.dll" "${DEST_DIR}/d3d12/d3d12SDKLayers.dll"
)

set_target_properties(basic_xess_fg_sample_AgilitySDKFiles
        PROPERTIES
        FOLDER "Copies"
)
endif()

set_target_properties(basic_xess_fg_sample
        PROPERTIES
        FOLDER "Samples"
)

if (NOT XEFG_BUILD_INTERNAL_SAMPLE)
    set(DLLS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../bin")
    add_custom_target(basic_xess_fg_sample_RequiredDLLs ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DLLS_PATH}/libxess_fg.dll" "${DEST_DIR}/libxess_fg.dll"
    )
    set_target_properties(basic_xess_fg_sample_RequiredDLLs
            PROPERTIES
            FOLDER "Copies"
    )
else ()
    install(TARGETS basic_xess_fg_sample
            RUNTIME DESTINATION bin
            ARCHIVE DESTINATION lib
            PUBLIC_HEADER DESTINATION inc
            RESOURCE DESTINATION bin
    )
    install(FILES shader_xess_fg.hlsl DESTINATION bin/basic_sample_shaders/hlsl)
endif()
