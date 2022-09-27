###############################################################################
# Copyright 2021 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files(the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions :
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
##############################################################################

set (TARGET_NAME MiniEngine)

option(ENABLE_MINIENGINE_SHADER_DEBUG "Enable MiniEngine shaders debugging and disable optimizations" OFF)

file(GLOB_RECURSE SOURCE_FILES
    "Core/*.cpp"
    "Core/*.h"
    "Model/*.cpp"
    "Model/*.h"
)

list(FILTER SOURCE_FILES EXCLUDE REGEX ".*GameInput.cpp$")

file(GLOB_RECURSE VERTEX_SHADER_FILES
    "Core/Shaders/*VS.hlsl"
    "Model/Shaders/*VS.hlsl"
)

file(GLOB_RECURSE PIXEL_SHADER_FILES
    "Core/Shaders/*PS.hlsl"
    "Model/Shaders/*PS.hlsl"
)

file(GLOB_RECURSE COMPUTE_SHADER_FILES
    "Core/Shaders/*CS.hlsl"
    "Model/Shaders/*CS.hlsl"
)

file(GLOB_RECURSE SHADER_INCLUDE_FILES
    "Core/Shaders/*.hlsli"
    "Model/Shaders/*.hlsli"
)

set(THIRD_PARTY_SOURCE_FILES
    "Dependencies/imgui/imgui.h"
    "Dependencies/imgui/imgui.cpp"
    "Dependencies/imgui/imgui_draw.cpp"
    "Dependencies/imgui/imgui_internal.h"
    "Dependencies/imgui/imgui_tables.cpp"
    "Dependencies/imgui/imgui_widgets.cpp"
    "Dependencies/imgui/backends/imgui_impl_win32.h"
    "Dependencies/imgui/backends/imgui_impl_win32.cpp"
)

set(SHADER_INC_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}
)

include(../Script/utility.cmake)

hlsl_compile("${VERTEX_SHADER_FILES}" Vertex "${SHADER_INC_PATH}" "_GAMING_DESKTOP=1" ENABLE_MINIENGINE_SHADER_DEBUG)
hlsl_compile("${PIXEL_SHADER_FILES}" Pixel "${SHADER_INC_PATH}" "_GAMING_DESKTOP=1" ENABLE_MINIENGINE_SHADER_DEBUG)
hlsl_compile("${COMPUTE_SHADER_FILES}" Compute "${SHADER_INC_PATH}" "_GAMING_DESKTOP=1" ENABLE_MINIENGINE_SHADER_DEBUG)
set_property(SOURCE ${SHADER_INCLUDE_FILES} PROPERTY HEADER_FILE_ONLY true)

set_msvc_precompiled_header(pch.h Core/pch.cpp "${SOURCE_FILES}")

set(SOURCE_FILES
    ${SOURCE_FILES}
    ${VERTEX_SHADER_FILES}
    ${PIXEL_SHADER_FILES}
    ${COMPUTE_SHADER_FILES}
    ${SHADER_INCLUDE_FILES}
    ${THIRD_PARTY_SOURCE_FILES}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})

add_library (${TARGET_NAME} STATIC ${SOURCE_FILES})

target_compile_definitions(${TARGET_NAME} PRIVATE 
    UNICODE _UNICODE __WRL_NO_DEFAULT_LIB__ _GAMING_DESKTOP
    $<$<CONFIG:Debug>:_DEBUG> 
    $<$<CONFIG:Release>:RELEASE> 
    $<$<CONFIG:MinSizeRel>:RELEASE> 
    $<$<CONFIG:RelWithDebInfo>:PROFILE> # We treat RelWithDebInfo as "Profile" config
)

#set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS "/NODEFAULTLIB:MSVCRT")

target_include_directories(
    ${TARGET_NAME} PRIVATE ${PROJECT_BINARY_DIR}/MiniEngine/$<CONFIG>
    ${TARGET_NAME} PRIVATE ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
    ${TARGET_NAME} PRIVATE ${PROJECT_BINARY_DIR}/MiniEngine/Dependencies/zlib

    ${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Core
    ${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/DirectXMesh/DirectXMesh
    ${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/DirectXTex/DirectXTex
    ${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/zlib
    ${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/WinPixEventRuntime/Include/WinPixEventRuntime
    ${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/imgui
)

# DirectXMesh dependency
option(BUILD_TOOLS "" OFF)
add_subdirectory(Dependencies/DirectXMesh)

# DirectTex dependency
option(BUILD_SAMPLE "" OFF)
option(BUILD_TOOLS "" OFF)
option(BUILD_DX11 "" OFF)
add_subdirectory(Dependencies/DirectXTex)

# zlib dependency
add_subdirectory(Dependencies/zlib)
target_compile_options(zlibstatic PRIVATE /wd4244)
