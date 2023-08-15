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

cmake_minimum_required(VERSION 3.16)

option(ENABLE_XESS_DEMO_ASSETS_COPY "Enable XeSS Demo Assets folder copy" ON)
option(ENABLE_XESS_DEMO_SHADER_DEBUG "Enable XeSS Demo shaders debugging and disable optimizations" OFF)

# Set minimum supported SDK and MSVC versions
set(MIN_SDK_VERSION "10.0.18363")
set(MIN_MSVC_VERSION "1920")

set (TARGET_NAME XeSSDemo)

project(${TARGET_NAME})

# Force MIN_SDK_VERSION or newer
if((${CMAKE_SYSTEM_VERSION} STRLESS ${MIN_SDK_VERSION}) AND ("$ENV{Version_Number}" STRLESS ${MIN_SDK_VERSION}))
    message(FATAL_ERROR "Windows SDK ${MIN_SDK_VERSION} or later required. Installed: ${CMAKE_SYSTEM_VERSION}")
endif()

#Force MIN_MSVC_VERSION or newer
if((${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC") AND (${MSVC_VERSION} STRLESS ${MIN_MSVC_VERSION}))
    message(FATAL_ERROR "MSVC ${MIN_MSVC_VERSION} required. Installed: ${CMAKE_CXX_COMPILER_ID} - ${CMAKE_CXX_COMPILER_VERSION}")
endif()

file(GLOB_RECURSE SRC_FILES
    "Source/*.cpp"
    "Source/*.h"
)

file(GLOB_RECURSE VERTEX_SHADER_FILES
    "Source/*VS.hlsl"
)

file(GLOB_RECURSE PIXEL_SHADER_FILES
    "Source/*PS.hlsl"
)

file(GLOB_RECURSE COMPUTE_SHADER_FILES
    "Source/*CS.hlsl"
)

file(GLOB_RECURSE SHADER_INCLUDE_FILES
    "Source/*.hlsli"
)

set(SHADER_INC_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/MiniEngine/Core
)

set(ALL_SHADER_FILES
    ${VERTEX_SHADER_FILES}
    ${PIXEL_SHADER_FILES}
    ${COMPUTE_SHADER_FILES}
)

set(SOURCE_FILES
    ${SRC_FILES}
    ${ALL_SHADER_FILES}
    ${SHADER_INCLUDE_FILES}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})

add_executable(${TARGET_NAME} WIN32 ${SOURCE_FILES} Source/XeSSDemo.manifest)

# Pre-compiled header
target_precompile_headers(${TARGET_NAME} PRIVATE Source/pch.h)
set_source_files_properties(${ALL_SHADER_FILES} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
set_source_files_properties(${SHADER_INCLUDE_FILES} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

target_compile_definitions(${TARGET_NAME} PRIVATE 
    UNICODE _UNICODE __WRL_NO_DEFAULT_LIB__ _GAMING_DESKTOP
    $<$<CONFIG:Debug>:_DEBUG> 
    $<$<CONFIG:Release>:RELEASE> 
    $<$<CONFIG:MinSizeRel>:RELEASE> 
    $<$<CONFIG:RelWithDebInfo>:RELEASE>
)

target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/MiniEngine/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/MiniEngine/Model
    ${CMAKE_CURRENT_SOURCE_DIR}/MiniEngine/Dependencies/WinPixEventRuntime/Include/WinPixEventRuntime
    ${CMAKE_CURRENT_SOURCE_DIR}/MiniEngine/Dependencies/imgui
    ${CMAKE_SOURCE_DIR}/libxess/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../inc/xess
    ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
    ${PROJECT_BINARY_DIR}/MiniEngine/$<CONFIG>
    ${PROJECT_BINARY_DIR}/$<CONFIG>
)

#set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS "/NODEFAULTLIB:MSVCRT")

if (NOT XESS_BUILD_INTERNAL_SAMPLE)
    set(XESS_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../lib)
    target_link_libraries(${TARGET_NAME} PRIVATE MiniEngine ${XESS_LIB_DIR}/libxess.lib)
else ()
    set(XESS_LIB_DIR ${CMAKE_BINARY_DIR}/bin/$<CONFIG>)
    target_link_libraries(${TARGET_NAME} PRIVATE MiniEngine libxess)
endif()

target_link_libraries(${TARGET_NAME} PRIVATE MiniEngine 
    DirectXMesh
    DirectXTex
    zlibstatic
)

target_link_libraries(${TARGET_NAME} PRIVATE 
    debug ${CMAKE_CURRENT_SOURCE_DIR}/MiniEngine/Dependencies/WinPixEventRuntime/bin/x64/WinPixEventRuntime.lib
)

set_source_files_properties(${VERTEX_SHADER_FILES} PROPERTIES ShaderType "vs")
set_source_files_properties(${PIXEL_SHADER_FILES} PROPERTIES ShaderType "ps")
set_source_files_properties(${COMPUTE_SHADER_FILES} PROPERTIES ShaderType "cs")

include(Script/utility.cmake)
if (NOT (${CMAKE_GENERATOR} STREQUAL "Ninja"))
    hlsl_compile(${TARGET_NAME} "${VERTEX_SHADER_FILES}" Vertex "${SHADER_INC_PATH}" "_GAMING_DESKTOP=1" ENABLE_XESS_DEMO_SHADER_DEBUG)
    hlsl_compile(${TARGET_NAME} "${PIXEL_SHADER_FILES}" Pixel "${SHADER_INC_PATH}" "_GAMING_DESKTOP=1" ENABLE_XESS_DEMO_SHADER_DEBUG)
    hlsl_compile(${TARGET_NAME} "${COMPUTE_SHADER_FILES}" Compute "${SHADER_INC_PATH}" "_GAMING_DESKTOP=1" ENABLE_XESS_DEMO_SHADER_DEBUG)
else()
    # For Ninja generator
    set (ShaderCompileTarget ${TARGET_NAME}_ShaderCompile)
    # Add an extra shader target
    add_custom_target(${ShaderCompileTarget})
    # add dxc commands to the shader target
    add_target_dxc_commands(${ShaderCompileTarget} "${ALL_SHADER_FILES}" "${SHADER_INCLUDE_FILES}" "${SHADER_INC_PATH}" "_GAMING_DESKTOP=1" ENABLE_XESS_DEMO_SHADER_DEBUG)

    # Make sure main target depends on this shader target
    add_dependencies(${TARGET_NAME} ${ShaderCompileTarget})
endif()

set_property(SOURCE ${SHADER_INCLUDE_FILES} PROPERTY HEADER_FILE_ONLY true)

# Custom command for copying Pix runtime.
add_custom_command(TARGET ${TARGET_NAME}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/MiniEngine/Dependencies/WinPixEventRuntime/bin/x64/WinPixEventRuntime.dll" 
    "$<TARGET_FILE_DIR:${TARGET_NAME}>/WinPixEventRuntime.dll")

if (ENABLE_XESS_DEMO_ASSETS_COPY)
    # Custom command for copying art assets.
    add_custom_command(TARGET ${TARGET_NAME}
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/Assets" "$<TARGET_FILE_DIR:${TARGET_NAME}>/Assets")
endif()

# Custom command for copying font.
add_custom_command(TARGET ${TARGET_NAME}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECT_SOURCE_DIR}/MiniEngine/Dependencies/imgui/misc/fonts/DroidSans.ttf" 
    "$<TARGET_FILE_DIR:${TARGET_NAME}>/DroidSans.ttf")

add_subdirectory(MiniEngine)

if (NOT XESS_BUILD_INTERNAL_SAMPLE)
    # Custom command for copying SDK dlls.
    set(SRC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../bin/")
    add_custom_command(TARGET ${TARGET_NAME}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_PATH}/libxess.dll" "$<TARGET_FILE_DIR:${TARGET_NAME}>/libxess.dll"
    )

    # Use project folders
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakePredefined")

    # "Dependencies" folder
    set_target_properties(zlibstatic PROPERTIES FOLDER "Dependencies")
    set_target_properties(DirectXMesh PROPERTIES FOLDER "Dependencies")
    set_target_properties(DirectXTex PROPERTIES FOLDER "Dependencies")

    # Set XeSSDemo as VS startup project
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${TARGET_NAME})
else ()
    add_dependencies(${TARGET_NAME} libxess)
    
    install(TARGETS ${TARGET_NAME}
            RUNTIME DESTINATION bin
            ARCHIVE DESTINATION lib
            PUBLIC_HEADER DESTINATION inc
            RESOURCE DESTINATION bin
)

install(FILES ${PROJECT_SOURCE_DIR}/MiniEngine/Dependencies/imgui/misc/fonts/DroidSans.ttf DESTINATION bin/)
endif()
