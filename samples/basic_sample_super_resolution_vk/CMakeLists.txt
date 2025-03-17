################################################################################
# Copyright (C) 2025 Intel Corporation
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

project(BasicSampleVK)

set(VULKAN_SAMPLE_SOURCES
	base/camera.hpp
	base/benchmark.hpp
	base/CommandLineParser.hpp
	base/keycodes.hpp
	base/VulkanBuffer.cpp
	base/VulkanBuffer.h
	base/VulkanDebug.cpp
	base/VulkanDebug.h
	base/VulkanDevice.cpp
	base/VulkanDevice.h
	base/VulkanInitializers.hpp
	base/VulkanSwapChain.cpp
	base/VulkanSwapChain.h
	base/VulkanTexture.cpp
	base/VulkanTexture.h
	base/VulkanTools.h
	base/VulkanTools.cpp
	vulkanexamplebase.h
	vulkanexamplebase.cpp
	triangle.cpp
	utils.cpp
	utils.h
)

if (NOT XESS_BUILD_INTERNAL_SAMPLE)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR} base)

add_executable(BasicSampleVK WIN32 ${SOURCES} ${VULKAN_SAMPLE_SOURCES})

set_target_properties(BasicSampleVK PROPERTIES COMPILE_FLAGS "-DVK_USE_PLATFORM_WIN32_KHR=1 -DXESS=1") 

if (NOT XESS_BUILD_INTERNAL_SAMPLE)
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../inc)
	target_link_libraries(BasicSampleVK PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/libxess.lib Vulkan::Vulkan)
else()
	target_link_libraries(BasicSampleVK PRIVATE project_options project_warnings libxess Vulkan::Vulkan)
endif()


target_compile_definitions(BasicSampleVK PRIVATE UNICODE)


# Mathematics library used by VK sample
if (NOT XESS_BUILD_INTERNAL_SAMPLE)
	add_subdirectory(glm)
endif()
target_link_libraries(BasicSampleVK PRIVATE glm::glm)

# Compile shaders
find_package(Vulkan COMPONENTS glslc)
function(target_compile_shaders target)
	cmake_parse_arguments(PARSE_ARGV 1 arg "" "DEST_DIR;SRC_DIR" "SOURCES")

	foreach(source ${arg_SOURCES})
		set(COMPILED_SHADERS ${COMPILED_SHADERS} ${CMAKE_CURRENT_BINARY_DIR}/${source}.spv)
		set(SOURCE_SHADERS ${SOURCE_SHADERS} ${CMAKE_CURRENT_SOURCE_DIR}/${arg_SRC_DIR}/${source})
		add_custom_command(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${source}.spv
			DEPENDS ${arg_SRC_DIR}/${source}
			DEPFILE ${CMAKE_CURRENT_BINARY_DIR}/${source}.d
			COMMAND
				${Vulkan_GLSLC_EXECUTABLE}
				--target-env=vulkan
				-mfmt=bin
				-MD -MF ${CMAKE_CURRENT_BINARY_DIR}/${source}.d
				-o ${CMAKE_CURRENT_BINARY_DIR}/${source}.spv
				${CMAKE_CURRENT_SOURCE_DIR}/${arg_SRC_DIR}/${source}
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/${source}.spv ${arg_DEST_DIR}/${source}.spv
		)
	endforeach()
	add_custom_target(${target}Shaders DEPENDS ${COMPILED_SHADERS} SOURCES ${SOURCE_SHADERS})
	add_dependencies(${target} ${target}Shaders)
endfunction()


if (MSVC AND (NOT (${CMAKE_GENERATOR} STREQUAL "Ninja")))
	set(DESTDIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)
else ()
	set(DESTDIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

target_compile_shaders(BasicSampleVK SRC_DIR . DEST_DIR ${DESTDIR} SOURCES
	basic_sample_shaders/glsl/triangle/triangle.vert
	basic_sample_shaders/glsl/triangle/triangle.frag
	basic_sample_shaders/glsl/triangle/velocity.vert
	basic_sample_shaders/glsl/triangle/velocity.frag
	basic_sample_shaders/glsl/triangle/quad.vert
	basic_sample_shaders/glsl/triangle/quad.frag)

if (NOT XESS_BUILD_INTERNAL_SAMPLE)
    set(SRC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../bin/")
    add_custom_target(CopyRequiredDLLs ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_PATH}/libxess.dll" "${DESTDIR}/libxess.dll"
    )
else ()
	install(TARGETS BasicSampleVK
			RUNTIME DESTINATION bin
			ARCHIVE DESTINATION lib
			PUBLIC_HEADER DESTINATION inc
			RESOURCE DESTINATION bin
	)
	install(DIRECTORY ${DESTDIR}/basic_sample_shaders DESTINATION bin PATTERN *.d EXCLUDE)
endif()