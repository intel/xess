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
project(basic_xell_sample)

set(SOURCES
    basic_sample.cpp
    basic_sample.h
    DXSample.cpp
    DXSample.h
    DXSampleHelper.h
    Main.cpp
    Win32Application.cpp
    Win32Application.h
    d3dx12.h
    stdafx.cpp
    stdafx.h
)

set(RESOURCES README.md)

if (NOT XELL_BUILD_INTERNAL_SAMPLE)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
endif()

add_executable(basic_xell_sample WIN32 ${SOURCES})

set_target_properties(basic_xell_sample PROPERTIES RESOURCE "${RESOURCES}")

target_link_libraries(basic_xell_sample PRIVATE d3d12 dxgi dxguid d3dcompiler)

if (NOT XELL_BUILD_INTERNAL_SAMPLE)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../inc)
    target_link_libraries(basic_xell_sample PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/libxell.lib)
else()
    target_link_libraries(basic_xell_sample PRIVATE libxell)
    target_include_directories(basic_xell_sample PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../libxell/include)
endif()

target_compile_definitions(basic_xell_sample PRIVATE UNICODE)
	
# Copy shaders
if (MSVC AND (NOT (${CMAKE_GENERATOR} STREQUAL "Ninja")))
    set(DEST_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)
else ()
    set(DEST_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

add_custom_target(copy_basic_xell_sample_files ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/shader_xell.hlsl "${DEST_DIR}/basic_sample_shaders/hlsl/shader_xell.hlsl"
)

if (NOT XELL_BUILD_INTERNAL_SAMPLE)
    set(SRC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../bin/")
    add_custom_target(CopyRequiredDLLs ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_PATH}/libxell.dll" "${DEST_DIR}/libxell.dll"
    )
else()
    install(TARGETS basic_xell_sample
            RUNTIME DESTINATION bin
            ARCHIVE DESTINATION lib
            PUBLIC_HEADER DESTINATION inc
            RESOURCE DESTINATION bin
    )
    install(FILES shader_xell.hlsl DESTINATION bin/basic_sample_shaders/hlsl)
endif()