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

project(BasicSample)

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
    utils.cpp
    utils.h
)

set(RESOURCES shaders.hlsl)

if (NOT XESS_BUILD_INTERNAL_SAMPLE)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
endif()

add_executable(BasicSample WIN32 ${SOURCES})

set_target_properties(BasicSample PROPERTIES RESOURCE "${RESOURCES}")

target_link_libraries(BasicSample PRIVATE d3d12 dxgi dxguid d3dcompiler)

if (NOT XESS_BUILD_INTERNAL_SAMPLE)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../inc)
    target_link_libraries(BasicSample PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/libxess)
else()
    target_link_libraries(BasicSample PRIVATE project_options project_warnings libxess)
endif()

target_compile_definitions(BasicSample PRIVATE UNICODE)

# Copy shaders
if (MSVC AND (NOT (${CMAKE_GENERATOR} STREQUAL "Ninja")))
    set(DEST_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)
else ()
    set(DEST_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

add_custom_target(CopyBasicSampleRequiredFiles ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/shaders.hlsl "${DEST_DIR}/shaders.hlsl"
)

if (NOT XESS_BUILD_INTERNAL_SAMPLE)

    set(SRC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../bin/")
    add_custom_target(CopyRequiredDLLs ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_PATH}/libxess.dll" "${DEST_DIR}/libxess.dll"
    )
else ()
    install(TARGETS BasicSample
            RUNTIME DESTINATION bin
            ARCHIVE DESTINATION lib
            PUBLIC_HEADER DESTINATION inc
            RESOURCE DESTINATION bin
    )
endif()
