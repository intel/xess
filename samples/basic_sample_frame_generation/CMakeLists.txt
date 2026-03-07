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

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(basic_xess_fg_sample)

if (NOT XEFG_BUILD_INTERNAL_SAMPLE)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
endif()

add_executable(basic_xess_fg_sample WIN32)
target_sources(basic_xess_fg_sample
    PRIVATE
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
target_compile_features(basic_xess_fg_sample INTERFACE cxx_std_11)
target_link_libraries(basic_xess_fg_sample PRIVATE d3d12 dxgi dxguid d3dcompiler)

target_compile_definitions(basic_xess_fg_sample PRIVATE UNICODE)

if (NOT XEFG_BUILD_INTERNAL_SAMPLE)
    set(SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../..")
    
    target_include_directories(basic_xess_fg_sample
        PRIVATE
            "${SDK_PATH}/inc/xess_fg"
            "${SDK_PATH}/inc/xell"
    )
    target_link_libraries(basic_xess_fg_sample
        PRIVATE
            ${SDK_PATH}/lib/libxess_fg.lib
            ${SDK_PATH}/lib/libxell.lib
    )
else()
    target_link_libraries(basic_xess_fg_sample PRIVATE libxess_fg xell xess_fg_headers)
endif()

if (MSVC)
    target_compile_options(basic_xess_fg_sample PRIVATE /W4 /WX /MP /EHsc)
else()
    target_compile_options(basic_xess_fg_sample PRIVATE -Wall -Wextra -pedantic -Werror -fexceptions -frtti)
endif()

if (MSVC AND (NOT (${CMAKE_GENERATOR} STREQUAL "Ninja")))
    set(DEST_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)
else ()
    set(DEST_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

add_custom_target(sample_assets ALL
    COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/shader_xess_fg.hlsl
            "${DEST_DIR}/basic_sample_shaders/hlsl/shader_xess_fg.hlsl"
)

set_target_properties(sample_assets PROPERTIES FOLDER "Copies")

if (NOT XEFG_BUILD_INTERNAL_SAMPLE)
    add_custom_target(sample_dependencies ALL
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
                "${SDK_PATH}/bin/libxess_fg.dll"
                "${DEST_DIR}/libxess_fg.dll"
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
                "${SDK_PATH}/bin/libxell.dll"
                "${DEST_DIR}/libxell.dll"
    )
    set_target_properties(sample_dependencies PROPERTIES FOLDER "Copies")
    add_dependencies(basic_xess_fg_sample sample_assets)
    add_dependencies(basic_xess_fg_sample sample_dependencies)
else ()
    install(TARGETS basic_xess_fg_sample
            RUNTIME DESTINATION bin
            ARCHIVE DESTINATION lib
            PUBLIC_HEADER DESTINATION inc
            RESOURCE DESTINATION bin
    )
    install(FILES shader_xess_fg.hlsl DESTINATION bin/basic_sample_shaders/hlsl)

    # Copy XeLL automatically for convenience
    add_dependencies(sample_assets copy_xell)
endif()
