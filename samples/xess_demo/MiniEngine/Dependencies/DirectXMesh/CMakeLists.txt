# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.13)

set(DIRECTXMESH_VERSION 1.6.3)

project(DirectXMesh
  VERSION ${DIRECTXMESH_VERSION}
  DESCRIPTION "DirectXMesh geometry Library"
  HOMEPAGE_URL "https://go.microsoft.com/fwlink/?LinkID=324981"
  LANGUAGES CXX)

option(BUILD_TOOLS "Build meshconvert" ON)

# Includes the support for DirectX 12 input layouts
option(BUILD_DX12 "Build with DirectX12 Runtime support" ON)

option(ENABLE_CODE_ANALYSIS "Use Static Code Analysis on build" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/CMake")

if (DEFINED VCPKG_TARGET_ARCHITECTURE)
    set(DIRECTX_ARCH ${VCPKG_TARGET_ARCHITECTURE})
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Ww][Ii][Nn]32$")
    set(DIRECTX_ARCH x86)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Xx]64$")
    set(DIRECTX_ARCH x64)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Aa][Rr][Mm]$")
    set(DIRECTX_ARCH arm)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Aa][Rr][Mm]64$")
    set(DIRECTX_ARCH arm64)
endif()

include(GNUInstallDirs)

#--- Library
set(LIBRARY_HEADERS
    DirectXMesh/DirectXMesh.h
    DirectXMesh/DirectXMesh.inl)

set(LIBRARY_SOURCES
    DirectXMesh/DirectXMeshP.h
    DirectXMesh/scoped.h
    DirectXMesh/DirectXMeshAdjacency.cpp
    DirectXMesh/DirectXMeshClean.cpp
    DirectXMesh/DirectXMeshGSAdjacency.cpp
    DirectXMesh/DirectXMeshletGenerator.cpp
    DirectXMesh/DirectXMeshNormals.cpp
    DirectXMesh/DirectXMeshOptimize.cpp
    DirectXMesh/DirectXMeshOptimizeLRU.cpp
    DirectXMesh/DirectXMeshOptimizeTVC.cpp
    DirectXMesh/DirectXMeshRemap.cpp
    DirectXMesh/DirectXMeshTangentFrame.cpp
    DirectXMesh/DirectXMeshUtil.cpp
    DirectXMesh/DirectXMeshValidate.cpp
    DirectXMesh/DirectXMeshVBReader.cpp
    DirectXMesh/DirectXMeshVBWriter.cpp
    DirectXMesh/DirectXMeshWeldVertices.cpp)

add_library(${PROJECT_NAME} STATIC ${LIBRARY_SOURCES} ${LIBRARY_HEADERS})

source_group(${PROJECT_NAME} REGULAR_EXPRESSION DirectXMesh/*.*)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/DirectXMesh>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

if ((${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16") AND (NOT MINGW))
    target_precompile_headers(${PROJECT_NAME} PRIVATE DirectXMesh/DirectXMeshP.h)
endif()

if(MSVC)
    # Use max Warning Level
    string(REPLACE "/W3 " "/Wall " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    string(REPLACE "/W3 " "/Wall " CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
    string(REPLACE "/W3 " "/Wall " CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})

    # Not using typeid or dynamic_cast, so disable RTTI to save binary size
    string(REPLACE "/GR " "/GR- " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    string(REPLACE "/GR " "/GR- " CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
    string(REPLACE "/GR " "/GR- " CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
endif()

if (MINGW OR (NOT WIN32) OR VCPKG_TOOLCHAIN)
    message("INFO: Using VCPKG for DirectX-Headers and DirectXMath.")
    find_package(directx-headers CONFIG REQUIRED)
    find_package(directxmath CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Microsoft::DirectX-Headers Microsoft::DirectXMath)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USING_DIRECTX_HEADERS)
endif()

#--- Utilities
set(UTILS_HEADERS
    Utilities/FlexibleVertexFormat.h
    Utilities/WaveFrontReader.h)

add_library(Utilities INTERFACE)

source_group(Utilities REGULAR_EXPRESSION Utilities/*.*)

target_include_directories(Utilities INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Utilities>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/utils>)

#--- Command-line tool
if(BUILD_TOOLS AND WIN32 AND (NOT WINDOWS_STORE))
  set(TOOL_EXES meshconvert)

  add_executable(meshconvert
    Meshconvert/Meshconvert.cpp
    Meshconvert/MeshOBJ.cpp
    Meshconvert/Mesh.h
    Meshconvert/Mesh.cpp
    Meshconvert/SDKMesh.h)
  target_include_directories(meshconvert PUBLIC MeshConvert Utilities)
  target_link_libraries(meshconvert ${PROJECT_NAME} version.lib)
  source_group(meshconvert REGULAR_EXPRESSION meshconvert/*.*)

  if (MINGW OR VCPKG_TOOLCHAIN)
    target_link_libraries(meshconvert Microsoft::DirectXMath)
  endif()
endif()

if(MSVC)
    foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
      target_compile_options(${t} PRIVATE /fp:fast "$<$<NOT:$<CONFIG:DEBUG>>:/guard:cf>")
      target_link_options(${t} PRIVATE /DYNAMICBASE /NXCOMPAT)
    endforeach()

    if((${CMAKE_SIZEOF_VOID_P} EQUAL 4) AND (NOT ${DIRECTX_ARCH} MATCHES "^arm"))
        foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
          target_link_options(${t} PRIVATE /SAFESEH)
        endforeach()
    endif()
endif()

if(NOT ${DIRECTX_ARCH} MATCHES "^arm")
    if (${CMAKE_SIZEOF_VOID_P} EQUAL "4")
        set(ARCH_SSE2 $<$<CXX_COMPILER_ID:MSVC>:/arch:SSE2> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-msse2>)
    else()
        set(ARCH_SSE2 $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-msse2>)
    endif()

    foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
      target_compile_options(${t} PRIVATE ${ARCH_SSE2})
    endforeach()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(WarningsLib "-Wpedantic" "-Wextra")
    target_compile_options(${PROJECT_NAME} PRIVATE ${WarningsLib})

    set(WarningsEXE ${WarningsLib} "-Wno-c++98-compat" "-Wno-c++98-compat-pedantic" "-Wno-switch" "-Wno-switch-enum" "-Wno-double-promotion" "-Wno-exit-time-destructors" "-Wno-missing-prototypes")
    foreach(t IN LISTS TOOL_EXES)
      target_compile_options(${t} PRIVATE ${WarningsEXE})
    endforeach()
endif()
if(MINGW)
    foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
      target_compile_options(${t} PRIVATE -Wno-ignored-attributes)
      target_link_options(${t} PRIVATE -municode)
    endforeach()
endif()
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
      target_compile_options(${t} PRIVATE /sdl /permissive- /JMC- /Zc:__cplusplus)
    endforeach()

    if(ENABLE_CODE_ANALYSIS)
      foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
        target_compile_options(${t} PRIVATE /analyze)
      endforeach()
    endif()

    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.24)
        foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
          target_compile_options(${t} PRIVATE /ZH:SHA_256)
        endforeach()
    endif()

    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.26)
        foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
          target_compile_options(${t} PRIVATE /Zc:preprocessor /wd5105)
        endforeach()
    endif()
    
    if ((CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.27) AND (NOT (${DIRECTX_ARCH} MATCHES "^arm")))
        foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
          target_link_options(${t} PRIVATE /CETCOMPAT)
        endforeach()
    endif()

    set(WarningsEXE "/wd4061" "/wd4365" "/wd4514" "/wd4625" "/wd4626" "/wd4627" "/wd4668" "/wd4710" "/wd4751" "/wd4820" "/wd5026" "/wd5027" "/wd5039" "/wd5045" "/wd5219")
    foreach(t IN LISTS TOOL_EXES)
      target_compile_options(${t} PRIVATE ${WarningsEXE})
    endforeach()
endif()

if(WIN32)
    foreach(t IN LISTS TOOL_EXES ITEMS ${PROJECT_NAME})
      target_compile_definitions(${t} PRIVATE _UNICODE UNICODE)
    endforeach()

    if(WINDOWS_STORE)
      target_compile_definitions(${PROJECT_NAME} PRIVATE WINAPI_FAMILY=WINAPI_FAMILY_APP)
    endif()

    if(BUILD_DX12 OR WINDOWS_STORE)
        if(BUILD_DX12)
          message("INFO: Building with DirectX 12 Runtime support")
        endif()
        target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32_WINNT=0x0A00)
    else()
        message("INFO: Building with Windows 7 compatibility")
        target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32_WINNT=0x0601)
    endif()
    foreach(t IN LISTS TOOL_EXES)
      target_compile_definitions(${t} PRIVATE _WIN32_WINNT=0x0601)
    endforeach()
endif()

if(BUILD_TOOLS AND WIN32 AND (NOT WINDOWS_STORE))
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT meshconvert)
endif()
