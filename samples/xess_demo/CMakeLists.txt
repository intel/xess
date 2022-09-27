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

cmake_minimum_required(VERSION 3.12)

#set_property(GLOBAL PROPERTY USE_FOLDERS ON)

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

#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

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

include(Script/utility.cmake)

hlsl_compile("${VERTEX_SHADER_FILES}" Vertex "${SHADER_INC_PATH}" "_GAMING_DESKTOP=1" ENABLE_XESS_DEMO_SHADER_DEBUG)
hlsl_compile("${PIXEL_SHADER_FILES}" Pixel "${SHADER_INC_PATH}" "_GAMING_DESKTOP=1" ENABLE_XESS_DEMO_SHADER_DEBUG)
hlsl_compile("${COMPUTE_SHADER_FILES}" Compute "${SHADER_INC_PATH}" "_GAMING_DESKTOP=1" ENABLE_XESS_DEMO_SHADER_DEBUG)
set_property(SOURCE ${SHADER_INCLUDE_FILES} PROPERTY HEADER_FILE_ONLY true)

set(SOURCE_FILES
    ${SRC_FILES}
    ${VERTEX_SHADER_FILES}
    ${PIXEL_SHADER_FILES}
    ${COMPUTE_SHADER_FILES}
    ${SHADER_INCLUDE_FILES}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})

add_executable(${TARGET_NAME} WIN32 ${SOURCE_FILES} Source/XeSSDemo.manifest)

set_msvc_precompiled_header(pch.h Source/pch.cpp "${SOURCE_FILES}")

target_compile_definitions(${TARGET_NAME} PRIVATE 
    UNICODE _UNICODE __WRL_NO_DEFAULT_LIB__ _GAMING_DESKTOP
    $<$<CONFIG:Debug>:_DEBUG> 
    $<$<CONFIG:Release>:RELEASE> 
    $<$<CONFIG:MinSizeRel>:RELEASE> 
    $<$<CONFIG:RelWithDebInfo>:PROFILE> # We treat RelWithDebInfo as "Profile" config
)

target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/MiniEngine/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/MiniEngine/Model
    ${CMAKE_CURRENT_SOURCE_DIR}/MiniEngine/Dependencies/WinPixEventRuntime/Include/WinPixEventRuntime
    ${CMAKE_CURRENT_SOURCE_DIR}/MiniEngine/Dependencies/imgui
    ${CMAKE_SOURCE_DIR}/libxess/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../inc
    ${CMAKE_BINARY_DIR}/bin/$<CONFIG>

    ${PROJECT_BINARY_DIR}/MiniEngine/$<CONFIG>
    ${PROJECT_BINARY_DIR}/$<CONFIG>
)

#set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS "/NODEFAULTLIB:MSVCRT")

if (NOT XESS_BUILD_INTERNAL_SAMPLE)
    set(XESS_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../lib)
else ()
    set(XESS_LIB_DIR ${CMAKE_BINARY_DIR}/bin/$<CONFIG>)
endif()

target_link_libraries(${TARGET_NAME} PRIVATE MiniEngine 
    ${XESS_LIB_DIR}/libxess.lib
    DirectXMesh
    DirectXTex
    zlibstatic
    ${CMAKE_CURRENT_SOURCE_DIR}/MiniEngine/Dependencies/WinPixEventRuntime/bin/x64/WinPixEventRuntime.lib
)

# Custom command for copying font.
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

# Custom command for copying SDK dlls.
file(GLOB_RECURSE SDK_DLLS
    "SDK/bin/*.dll"
)

foreach(dll ${SDK_DLLS})
    add_custom_command(TARGET ${TARGET_NAME}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${dll} "$<TARGET_FILE_DIR:${TARGET_NAME}>")
endforeach()

add_subdirectory(MiniEngine)

# project folder
set_target_properties(zlibstatic PROPERTIES FOLDER "Dependencies")
set_target_properties(DirectXMesh PROPERTIES FOLDER "Dependencies")
set_target_properties(DirectXTex PROPERTIES FOLDER "Dependencies")

if (NOT XESS_BUILD_INTERNAL_SAMPLE)
    set(SRC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../bin/")
    add_custom_target(CopyRequiredDLLs ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_PATH}/dxcompiler.dll" "$<TARGET_FILE_DIR:${TARGET_NAME}>/dxcompiler.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_PATH}/dxil.dll" "$<TARGET_FILE_DIR:${TARGET_NAME}>/dxil.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_PATH}/igxess.dll" "$<TARGET_FILE_DIR:${TARGET_NAME}>/igxess.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_PATH}/libxess.dll" "$<TARGET_FILE_DIR:${TARGET_NAME}>/libxess.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_PATH}/XeFX.dll" "$<TARGET_FILE_DIR:${TARGET_NAME}>/XeFX.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_PATH}/XeFX_loader.dll" "$<TARGET_FILE_DIR:${TARGET_NAME}>/XeFX_loader.dll"
    )
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