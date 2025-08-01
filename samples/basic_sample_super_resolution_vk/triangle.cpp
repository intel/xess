/*
* Vulkan Example - Basic indexed triangle rendering
*
* Note:
*	This is a "pedal to the metal" example to show off how to get Vulkan up and displaying something
*	Contrary to the other examples, this one won't make use of helper functions or initializers
*	Except in a few cases (swap chain setup e.g.)
*
* Copyright (C) 2016-2023 by Sascha Willems - www.saschawillems.de
* Copyright (C) 2024 Intel Corporation
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

//#define USE_LOWRES_MV 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fstream>
#include <vector>
#include <exception>
#include <Windows.h>

#include <vulkan/vulkan.h>
#include "vulkanexamplebase.h"

// XeSS-related utilities
#include "utils.h"

#include "xess/xess_vk.h"

// Set to "true" to enable Vulkan's validation layers (see vulkandebug.cpp for details)
#if defined(_DEBUG)
#define ENABLE_VALIDATION true
#else
#define ENABLE_VALIDATION false
#endif

// We want to keep GPU and CPU busy. To do that we may start building a new command buffer while the previous one is still being executed
// This number defines how many frames may be worked on simultaneously at once
#define MAX_CONCURRENT_FRAMES 2

class VulkanExample : public VulkanExampleBase
{
public:

	VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mutableDescriptorFeatures{};
	VkPhysicalDeviceShaderFloat16Int8Features shaderFloat16IntFeatures{};
	VkPhysicalDeviceShaderIntegerDotProductFeatures shaderIntegerDotProductFeatures{};

	// Vertex layout used in this example
	struct Vertex {
		float position[3];
		float color[4];
	};

	// Vertex buffer and attributes
	struct {
		VkDeviceMemory memory = VK_NULL_HANDLE; // Handle to the device memory for this buffer
		VkBuffer buffer = VK_NULL_HANDLE;       // Handle to the Vulkan buffer object that the memory is bound to
		uint32_t count = 0;
	} vertices{};

	// Uniform buffer block object
	struct UniformBuffer {
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkBuffer buffer = VK_NULL_HANDLE;
		// The descriptor set stores the resources bound to the binding points in a shader
		// It connects the binding points of the different shaders with the buffers and images used for those bindings
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		// We keep a pointer to the mapped buffer, so we can easily update it's contents via a memcpy
		uint8_t* mapped{ nullptr };
	};
	// We use one UBO per frame, so we can have a frame overlap and make sure that uniforms aren't updated while still in use
	std::array<UniformBuffer, MAX_CONCURRENT_FRAMES> uniformBuffers;

	// The pipeline layout is used by a pipeline to access the descriptor sets
	// It defines interface (without binding any actual data) between the shader stages used by the pipeline and the shader resources
	// A pipeline layout can be shared among multiple pipelines as long as their interfaces match
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

	VkPipelineLayout fsqPipelineLayout = VK_NULL_HANDLE;

	// Pipelines (often called "pipeline state objects") are used to bake all states that affect a pipeline
	// While in OpenGL every state can be changed at (almost) any time, Vulkan requires to layout the graphics (and compute) pipeline states upfront
	// So for each combination of non-dynamic pipeline states you need a new pipeline (there are a few exceptions to this not discussed here)
	// Even though this adds a new dimension of planning ahead, it's a great opportunity for performance optimizations by the driver
	VkPipeline pipeline = VK_NULL_HANDLE;

	// Pipeline for rendering velocity buffer
	VkPipeline velocityPipeline = VK_NULL_HANDLE;

	// Pipeline for full screen quad
	VkPipeline fsqPipeline = VK_NULL_HANDLE;

	// The descriptor set layout describes the shader binding layout (without actually referencing descriptor)
	// Like the pipeline layout it's pretty much a blueprint and can be used with different descriptor sets as long as their layout matches
	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

	VkDescriptorSetLayout fsqDescriptorSetLayout = VK_NULL_HANDLE;

	VkDescriptorSet fsqDescriptorSet = VK_NULL_HANDLE;

	VkSampler fsqSampler = VK_NULL_HANDLE;

	// Synchronization primitives
	// Synchronization is an important concept of Vulkan that OpenGL mostly hid away. Getting this right is crucial to using Vulkan.

	// Semaphores are used to coordinate operations within the graphics queue and ensure correct command ordering
	std::array<VkSemaphore, MAX_CONCURRENT_FRAMES> presentCompleteSemaphores{ VK_NULL_HANDLE };
	std::array<VkSemaphore, MAX_CONCURRENT_FRAMES> renderCompleteSemaphores{ VK_NULL_HANDLE };

	VkCommandPool commandPool = VK_NULL_HANDLE;
	std::array<VkCommandBuffer, MAX_CONCURRENT_FRAMES> commandBuffers{ VK_NULL_HANDLE };
	std::array<VkFence, MAX_CONCURRENT_FRAMES> waitFences{ VK_NULL_HANDLE };

	uint32_t currentFrame = 0;

	xess_2d_t xessInputResolution = { 0u, 0u };

	// For simplicity we use the same uniform block layout as in the shader:
	//
	// This way we can just memcopy the ubo data to the ubo
	// Note: You should use data types that align with the GPU in order to avoid manual padding (vec4, mat4)
	struct ShaderData {
		glm::vec4 offset;
		glm::vec4 velocity;
		glm::vec4 resolution;
	};

	ShaderData m_uniformBufferData;

	bool m_pause = false;

	// Last time the previous shader data was updated
	std::chrono::time_point<std::chrono::high_resolution_clock> last_shader_data_time;

	// Jitter
	std::vector<std::pair<float, float>> m_haltonPointSet;
	std::size_t m_haltonIndex = 0;
	float jitter[2];

	const xess_quality_settings_t xessQuality = XESS_QUALITY_SETTING_PERFORMANCE;

	xess_context_handle_t xessContext = nullptr;

	struct FrameBufferAttachment {
		VkImage image = VK_NULL_HANDLE;
		VkDeviceMemory mem = VK_NULL_HANDLE;
		VkImageView view = VK_NULL_HANDLE;
		VkFormat format = VK_FORMAT_UNDEFINED;
		void destroy(VkDevice device_)
		{
			vkDestroyImage(device_, image, nullptr);
			vkDestroyImageView(device_, view, nullptr);
			vkFreeMemory(device_, mem, nullptr);
		}
	};

	struct FrameBuffer {
		int32_t width, height;
		VkFramebuffer frameBuffer = VK_NULL_HANDLE;
		VkRenderPass renderPass = VK_NULL_HANDLE;
		void setSize(int32_t w, int32_t h)
		{
			this->width = w;
			this->height = h;
		}
		void destroy(VkDevice device_)
		{
			vkDestroyFramebuffer(device_, frameBuffer, nullptr);
			vkDestroyRenderPass(device_, renderPass, nullptr);
		}
	};

	struct {
		struct Offscreen : public FrameBuffer {
			FrameBufferAttachment color, depth;
		} render;
		struct Velocity : public FrameBuffer {
			FrameBufferAttachment velocity;
		} velocity;
	} offscreenFrameBuffers;

	uint32_t velocityWidth = 0;
	uint32_t velocityHeight = 0;

	FrameBufferAttachment xessOutput;
	
	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		title = "XeSS-SR VK basic sample";
		// To keep things simple, we don't use the UI overlay from the framework
		settings.overlay = false;
		// Setup a default look-at camera
		camera.type = Camera::CameraType::lookat;
		camera.setPosition(glm::vec3(0.0f, 0.0f, -2.5f));
		camera.setRotation(glm::vec3(0.0f));
		// Values not set here are initialized in the base class constructor

		m_haltonPointSet = Utils::GenerateHalton(2, 3, 1, 32);

		m_uniformBufferData.offset = glm::vec4(0.f);
		m_uniformBufferData.resolution = glm::vec4(0.f);
		m_uniformBufferData.velocity = glm::vec4(0.f);

		// Require Vulkan 1.1
		apiVersion = VK_API_VERSION_1_1;
	}

	~VulkanExample()
	{
		auto status = xessDestroyContext(xessContext);
		assert(status == XESS_RESULT_SUCCESS);
		(void)status;

		// Clean up used Vulkan resources
		// Note: Inherited destructor cleans up resources stored in base class
		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipeline(device, velocityPipeline, nullptr);
		vkDestroyPipeline(device, fsqPipeline, nullptr);

		offscreenFrameBuffers.render.destroy(device);
		offscreenFrameBuffers.render.color.destroy(device);
		offscreenFrameBuffers.render.depth.destroy(device);

		offscreenFrameBuffers.velocity.destroy(device);
		offscreenFrameBuffers.velocity.velocity.destroy(device);

		xessOutput.destroy(device);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		vkDestroySampler(device, fsqSampler, nullptr);

		vkDestroyPipelineLayout(device, fsqPipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, fsqDescriptorSetLayout, nullptr);


		vkDestroyBuffer(device, vertices.buffer, nullptr);
		vkFreeMemory(device, vertices.memory, nullptr);

		vkDestroyCommandPool(device, commandPool, nullptr);

		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++)
		{
			vkDestroyFence(device, waitFences[i], nullptr);
			vkDestroySemaphore(device, presentCompleteSemaphores[i], nullptr);
			vkDestroySemaphore(device, renderCompleteSemaphores[i], nullptr);
			vkDestroyBuffer(device, uniformBuffers[i].buffer, nullptr);
			vkFreeMemory(device, uniformBuffers[i].memory, nullptr);
		}
	}

	void keyReleased(uint32_t key) // override
	{
		switch (key)
		{
			case VK_SPACE:
				m_pause = !m_pause;
				break;
		}
	}

	// This function is used to request a device memory type that supports all the property flags we request (e.g. device local, host visible)
	// Upon success it will return the index of the memory type that fits our requested memory properties
	// This is necessary as implementations can offer an arbitrary number of memory types with different
	// memory properties.
	// You can check https://vulkan.gpuinfo.org/ for details on different memory configurations
	uint32_t getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties)
	{
		// Iterate over all memory types available for the device used in this example
		for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++)
		{
			if ((typeBits & 1) == 1)
			{
				if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
				{
					return i;
				}
			}
			typeBits >>= 1;
		}

		throw std::runtime_error("Could not find a suitable memory type!");
	}

	void getEnabledFeatures() override
	{
		void* features = &enabledFeatures;
		auto status = xessVKGetRequiredDeviceFeatures(instance, physicalDevice, &features);
		if (status != XESS_RESULT_SUCCESS)
		{
			throw std::runtime_error("Unable to get required XeSS device features");
		}
	}

	void getEnabledExtensions() override
	{
		uint32_t extensionCount;
		const char*const* extensions;

		auto status = xessVKGetRequiredDeviceExtensions(instance, physicalDevice, &extensionCount, &extensions);
		if (status != XESS_RESULT_SUCCESS)
		{
			throw std::runtime_error("Unable to get required XeSS device extensions");
		}
		for (size_t extensionIdx = 0; extensionIdx < extensionCount; ++extensionIdx)
		{
			if (std::find(enabledDeviceExtensions.begin(), enabledDeviceExtensions.end(), extensions[extensionIdx]) == enabledDeviceExtensions.end())
			{
				enabledDeviceExtensions.push_back(extensions[extensionIdx]);
			}
		}
	}

	void setupXess()
	{
		auto status = xessVKCreateContext(instance, physicalDevice, device, &xessContext);
		if (status != XESS_RESULT_SUCCESS)
		{
			throw std::runtime_error("Unable to create XeSS context");
		}

		if (XESS_RESULT_WARNING_OLD_DRIVER == xessIsOptimalDriver(xessContext))
		{
			MessageBox(NULL, L"Please install the latest graphics driver from your vendor for optimal Intel(R) XeSS performance and visual quality", L"Important notice", MB_OK | MB_TOPMOST | MB_ICONINFORMATION);
		}

		xess_properties_t props;
		xess_2d_t outputResoulution = { width, height };
		status = xessGetProperties(xessContext, &outputResoulution, &props);
		if (status != XESS_RESULT_SUCCESS)
		{
			throw std::runtime_error("Unable to get XeSS props");
		}

		xess_version_t xefx_version;
		status = xessGetIntelXeFXVersion(xessContext, &xefx_version);
		if (status != XESS_RESULT_SUCCESS)
		{
			throw std::runtime_error("Unable to get XeFX version");
		}
#if 0
		D3D12_HEAP_DESC textures_heap_desc{ props.tempTextureHeapSize,
			{D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0},
			0, D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES };

		m_device->CreateHeap(&textures_heap_desc, IID_PPV_ARGS(&m_texturesHeap));
		m_texturesHeap->SetName(L"xess_textures_heap");

		m_uavDescriptorSize =
			m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
#endif
		xess_vk_init_params_t params = {
			/* Output width and height */
			{width, height},
			/* Quality setting */
			xessQuality,
			/* Initialization flags. */
			#if defined(USE_LOWRES_MV)
			XESS_INIT_FLAG_NONE,
			#else
			XESS_INIT_FLAG_HIGH_RES_MV,
			#endif
			/* Specfies the node mask for internally created resources on
			 * multi-adapter systems. */
			0,
			/* Specfies the node visibility mask for internally created resources
			 * on multi-adapter systems. */
			0,
			/* Optional externally allocated buffers storage for XeSS-SR. If NULL the
			 * storage is allocated internally. If allocated, the heap type must be
			 * D3D12_HEAP_TYPE_DEFAULT. This heap is not accessed by the CPU. */
			nullptr,
			/* Offset in the externally allocated heap for temporary buffers storage. */
			0,
			/* Optional externally allocated textures storage for XeSS-SR. If NULL the
			 * storage is allocated internally. If allocated, the heap type must be
			 * D3D12_HEAP_TYPE_DEFAULT. This heap is not accessed by the CPU. */
			nullptr,
			/* Offset in the externally allocated heap for temporary textures storage. */
			0,
			/* No pipeline library */
			NULL
		};

		status = xessVKInit(xessContext, &params);
		if (status != XESS_RESULT_SUCCESS)
		{
			throw std::runtime_error("Unable to get XeSS props");
		}

		// Get optimal input resolution
		xess_2d_t outputResolution = { width, height };
		status = xessGetInputResolution(xessContext, &outputResolution, xessQuality, &xessInputResolution);

		if (status != XESS_RESULT_SUCCESS)
		{
			throw std::runtime_error("Unable to get XeSS props");
		}
		camera.setPerspective(60.0f, (float)xessInputResolution.x / (float)xessInputResolution.y, 1.0f, 256.0f);

		// Xess output
		createAttachment(VK_FORMAT_R16G16B16A16_UNORM, VK_IMAGE_USAGE_STORAGE_BIT, &xessOutput, width, height);
	}

	// Create a frame buffer attachment
	void createAttachment(
		VkFormat format,
		VkImageUsageFlags usage,
		FrameBufferAttachment* attachment,
		uint32_t width_,
		uint32_t height_)
	{
		VkImageAspectFlags aspectMask = 0;

		attachment->format = format;

		if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT || usage & VK_IMAGE_USAGE_STORAGE_BIT)
		{
			aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			// the view is also going to be used by XeSS, so it needs single aspect (without stencil)
			aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		}

		assert(aspectMask > 0);

		VkImageCreateInfo image = vks::initializers::imageCreateInfo();
		image.imageType = VK_IMAGE_TYPE_2D;
		image.format = format;
		image.extent.width = width_;
		image.extent.height = height_;
		image.extent.depth = 1;
		image.mipLevels = 1;
		image.arrayLayers = 1;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
		image.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;

		VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &attachment->image));
		vkGetImageMemoryRequirements(device, attachment->image, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &attachment->mem));
		VK_CHECK_RESULT(vkBindImageMemory(device, attachment->image, attachment->mem, 0));

		VkImageViewCreateInfo imageView = vks::initializers::imageViewCreateInfo();
		imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageView.format = format;
		imageView.subresourceRange = {};
		imageView.subresourceRange.aspectMask = aspectMask;
		imageView.subresourceRange.baseMipLevel = 0;
		imageView.subresourceRange.levelCount = 1;
		imageView.subresourceRange.baseArrayLayer = 0;
		imageView.subresourceRange.layerCount = 1;
		imageView.image = attachment->image;
		VK_CHECK_RESULT(vkCreateImageView(device, &imageView, nullptr, &attachment->view));
	}

	// Create the Vulkan synchronization primitives used in this example
	void createSynchronizationPrimitives()
	{
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
			
			// Semaphores (Used for correct command ordering)
			VkSemaphoreCreateInfo semaphoreCI{};
			semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			// Semaphore used to ensure that image presentation is complete before starting to submit again
			VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &presentCompleteSemaphores[i]));
			// Semaphore used to ensure that all commands submitted have been finished before submitting the image to the queue
			VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &renderCompleteSemaphores[i]));

			// Fences (Used to check draw command buffer completion)
			VkFenceCreateInfo fenceCI{};
			fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			// Create in signaled state so we don't wait on first render of each command buffer
			fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			VK_CHECK_RESULT(vkCreateFence(device, &fenceCI, nullptr, &waitFences[i]));

		}
	}

	void createCommandBuffers()
	{
		// All command buffers are allocated from a command pool
		VkCommandPoolCreateInfo commandPoolCI{};
		commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCI.queueFamilyIndex = swapChain.queueNodeIndex;
		commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK_RESULT(vkCreateCommandPool(device, &commandPoolCI, nullptr, &commandPool));

		// Allocate one command buffer per max. concurrent frame from above pool
		VkCommandBufferAllocateInfo cmdBufAllocateInfo = vks::initializers::commandBufferAllocateInfo(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, MAX_CONCURRENT_FRAMES);
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, commandBuffers.data()));
	}

	// Prepare vertex and index buffers for an indexed triangle
	// Also uploads them to device local memory using staging and initializes vertex input and attribute binding to match the vertex shader
	void createVertexBuffer()
	{
		// A note on memory management in Vulkan in general:
		//	This is a very complex topic and while it's fine for an example application to small individual memory allocations that is not
		//	what should be done a real-world application, where you should allocate large chunks of memory at once instead.

		// Setup vertices
		float aspectRatio = static_cast<float>(xessInputResolution.x) / static_cast<float>(xessInputResolution.y);
		std::vector<Vertex> vertexBuffer{
			{{0.0f, -0.25f * aspectRatio, 0.01f}, {1.0f, 0.0f, 0.0f, 1.0f}},
			{{0.25f, 0.25f * aspectRatio, 0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
			{{-0.25f, 0.25f * aspectRatio, 0.2f}, {0.0f, 0.0f, 1.0f, 1.0f}}
		};
		uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(Vertex);

		vertices.count = static_cast<uint32_t>(vertexBuffer.size());

		VkMemoryAllocateInfo memAlloc{};
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		VkMemoryRequirements memReqs;

		// Static data like vertex and index buffer should be stored on the device memory for optimal (and fastest) access by the GPU
		//
		// To achieve this we use so-called "staging buffers" :
		// - Create a buffer that's visible to the host (and can be mapped)
		// - Copy the data to this buffer
		// - Create another buffer that's local on the device (VRAM) with the same size
		// - Copy the data from the host to the device using a command buffer
		// - Delete the host visible (staging) buffer
		// - Use the device local buffers for rendering
		//
		// Note: On unified memory architectures where host (CPU) and GPU share the same memory, staging is not necessary
		// To keep this sample easy to follow, there is no check for that in place

		struct StagingBuffer {
			VkDeviceMemory memory;
			VkBuffer buffer;
		};

		struct {
			StagingBuffer vertices;
		} stagingBuffers;

		void* data;

		// Vertex buffer
		VkBufferCreateInfo vertexBufferInfoCI{};
		vertexBufferInfoCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferInfoCI.size = vertexBufferSize;
		// Buffer is used as the copy source
		vertexBufferInfoCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		// Create a host-visible buffer to copy the vertex data to (staging buffer)
		VK_CHECK_RESULT(vkCreateBuffer(device, &vertexBufferInfoCI, nullptr, &stagingBuffers.vertices.buffer));
		vkGetBufferMemoryRequirements(device, stagingBuffers.vertices.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		// Request a host visible memory type that can be used to copy our data do
		// Also request it to be coherent, so that writes are visible to the GPU right after unmapping the buffer
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffers.vertices.memory));
		// Map and copy
		VK_CHECK_RESULT(vkMapMemory(device, stagingBuffers.vertices.memory, 0, memAlloc.allocationSize, 0, &data));
		memcpy(data, vertexBuffer.data(), vertexBufferSize);
		vkUnmapMemory(device, stagingBuffers.vertices.memory);
		VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffers.vertices.buffer, stagingBuffers.vertices.memory, 0));

		// Create a device local buffer to which the (host local) vertex data will be copied and which will be used for rendering
		vertexBufferInfoCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		VK_CHECK_RESULT(vkCreateBuffer(device, &vertexBufferInfoCI, nullptr, &vertices.buffer));
		vkGetBufferMemoryRequirements(device, vertices.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &vertices.memory));
		VK_CHECK_RESULT(vkBindBufferMemory(device, vertices.buffer, vertices.memory, 0));

				// Buffer copies have to be submitted to a queue, so we need a command buffer for them
		// Note: Some devices offer a dedicated transfer queue (with only the transfer bit set) that may be faster when doing lots of copies
		VkCommandBuffer copyCmd;

		VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
		cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocateInfo.commandPool = commandPool;
		cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufAllocateInfo.commandBufferCount = 1;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &copyCmd));

		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
		VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));
		// Put buffer region copies into command buffer
		VkBufferCopy copyRegion{};
		// Vertex buffer
		copyRegion.size = vertexBufferSize;
		vkCmdCopyBuffer(copyCmd, stagingBuffers.vertices.buffer, vertices.buffer, 1, &copyRegion);
		VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

		// Submit the command buffer to the queue to finish the copy
		VkSubmitInfo submitInfo_{};
		submitInfo_.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo_.commandBufferCount = 1;
		submitInfo_.pCommandBuffers = &copyCmd;

		// Create fence to ensure that the command buffer has finished executing
		VkFenceCreateInfo fenceCI{};
		fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCI.flags = 0;
		VkFence fence;
		VK_CHECK_RESULT(vkCreateFence(device, &fenceCI, nullptr, &fence));

		// Submit to the queue
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo_, fence));
		// Wait for the fence to signal that command buffer has finished executing
		VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

		vkDestroyFence(device, fence, nullptr);
		vkFreeCommandBuffers(device, commandPool, 1, &copyCmd);

		// Destroy staging buffers
		// Note: Staging buffer must not be deleted before the copies have been submitted and executed
		vkDestroyBuffer(device, stagingBuffers.vertices.buffer, nullptr);
		vkFreeMemory(device, stagingBuffers.vertices.memory, nullptr);
	}

	// Descriptors are allocated from a pool, that tells the implementation how many and what types of descriptors we are going to use (at maximum)
	void createDescriptorPool()
	{
		// We need to tell the API the number of max. requested descriptors per type
		VkDescriptorPoolSize descriptorTypeCounts[2];
		// This example only one descriptor type (uniform buffer)
		descriptorTypeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		// We have one buffer (and as such descriptor) per frame
		descriptorTypeCounts[0].descriptorCount = MAX_CONCURRENT_FRAMES;
		// For additional types you need to add new entries in the type count list
		// E.g. for two combined image samplers :
		// typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		// typeCounts[1].descriptorCount = 2;


		descriptorTypeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorTypeCounts[1].descriptorCount = 1;

		// Create the global descriptor pool
		// All descriptors used in this example are allocated from this pool
		VkDescriptorPoolCreateInfo descriptorPoolCI{};
		descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCI.pNext = nullptr;
		descriptorPoolCI.poolSizeCount = 2;
		descriptorPoolCI.pPoolSizes = descriptorTypeCounts;
		// Set the max. number of descriptor sets that can be requested from this pool (requesting beyond this limit will result in an error)
		// Our sample will create one set per uniform buffer per frame
		descriptorPoolCI.maxSets = MAX_CONCURRENT_FRAMES + 1;
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &descriptorPool));
	}

	// Descriptor set layouts define the interface between our application and the shader
	// Basically connects the different shader stages to descriptors for binding uniform buffers, image samplers, etc.
	// So every shader binding should map to one descriptor set layout binding
	void createDescriptorSetLayout()
	{
		// Binding 0: Uniform buffer (Vertex shader)
		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layoutBinding.descriptorCount = 1;
		layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		layoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
		descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorLayoutCI.pNext = nullptr;
		descriptorLayoutCI.bindingCount = 1;
		descriptorLayoutCI.pBindings = &layoutBinding;
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &descriptorSetLayout));


		VkDescriptorSetLayoutBinding fsqLayoutBinding{};
		fsqLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		fsqLayoutBinding.descriptorCount = 1;
		fsqLayoutBinding.stageFlags =  VK_SHADER_STAGE_FRAGMENT_BIT;
		fsqLayoutBinding.pImmutableSamplers = nullptr;

		descriptorLayoutCI.pBindings = &fsqLayoutBinding;
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &fsqDescriptorSetLayout));

		// Create the pipeline layout that is used to generate the rendering pipelines that are based on this descriptor set layout
		// In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that could be reused
		VkPipelineLayoutCreateInfo pipelineLayoutCI{};
		pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCI.pNext = nullptr;
		pipelineLayoutCI.setLayoutCount = 1;
		pipelineLayoutCI.pSetLayouts = &descriptorSetLayout;
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));

		pipelineLayoutCI.pSetLayouts = &fsqDescriptorSetLayout;
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &fsqPipelineLayout));
	}

	// Shaders access data using descriptor sets that "point" at our uniform buffers
	// The descriptor sets make use of the descriptor set layouts created above 
	void createDescriptorSets()
	{
		// Allocate one descriptor set per frame from the global descriptor pool
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = descriptorPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &descriptorSetLayout;
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &uniformBuffers[i].descriptorSet));

			// Update the descriptor set determining the shader binding points
			// For every binding point used in a shader there needs to be one
			// descriptor set matching that binding point
			VkWriteDescriptorSet writeDescriptorSet{};
			
			// The buffer's information is passed using a descriptor info structure
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i].buffer;
			bufferInfo.range = sizeof(ShaderData);

			// Binding 0 : Uniform buffer
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = uniformBuffers[i].descriptorSet;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSet.pBufferInfo = &bufferInfo;
			writeDescriptorSet.dstBinding = 0;
			vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
		}


		{
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = descriptorPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &fsqDescriptorSetLayout;
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &fsqDescriptorSet));

			// Update the descriptor set determining the shader binding points
			// For every binding point used in a shader there needs to be one
			// descriptor set matching that binding point
			VkWriteDescriptorSet writeDescriptorSet{};

			// The image + sampler  information is passed using a descriptor info structure
			VkDescriptorImageInfo imageInfo{};
			imageInfo.sampler = fsqSampler;
			imageInfo.imageView = xessOutput.view;
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			// Binding 0 : Uniform buffer
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = fsqDescriptorSet;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSet.pImageInfo = &imageInfo;
			writeDescriptorSet.dstBinding = 0;
			vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
		}
	}

	// Create a frame buffer for each swap chain image
	// Note: Override of virtual function in the base class and called from within VulkanExampleBase::prepare
	void setupFrameBuffer()
	{
		// Create a frame buffer for every image in the swapchain
		frameBuffers.resize(swapChain.imageCount);
		for (size_t i = 0; i < frameBuffers.size(); i++)
		{
			std::array<VkImageView, 1> attachments;
			// Color attachment is the view of the swapchain image
			attachments[0] = swapChain.buffers[i].view;

			VkFramebufferCreateInfo frameBufferCI{};
			frameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			// All frame buffers use the same renderpass setup
			frameBufferCI.renderPass = renderPass;
			frameBufferCI.attachmentCount = static_cast<uint32_t>(attachments.size());
			frameBufferCI.pAttachments = attachments.data();
			frameBufferCI.width = width;
			frameBufferCI.height = height;
			frameBufferCI.layers = 1;
			// Create the framebuffer
			VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCI, nullptr, &frameBuffers[i]));
		}
	}

	// Render pass setup
	// Render passes are a new concept in Vulkan. They describe the attachments used during rendering and may contain multiple subpasses with attachment dependencies
	// This allows the driver to know up-front what the rendering will look like and is a good opportunity to optimize especially on tile-based renderers (with multiple subpasses)
	// Using sub pass dependencies also adds implicit layout transitions for the attachment used, so we don't need to add explicit image memory barriers to transform them
	// Note: Override of virtual function in the base class and called from within VulkanExampleBase::prepare
	void setupRenderPass()
	{
		// This example will use a single render pass with one subpass

		// Descriptors for the attachments used by this renderpass
		std::array<VkAttachmentDescription, 1> attachments{};

		// Color attachment
		attachments[0].format = swapChain.colorFormat;                                  // Use the color format selected by the swapchain
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;                                 // We don't use multi sampling in this example
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;                            // Clear this attachment at the start of the render pass
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;                          // Keep its contents after the render pass is finished (for displaying it)
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;                 // We don't use stencil, so don't care for load
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;               // Same for store
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;                       // Layout at render pass start. Initial doesn't matter, so we use undefined
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;                   // Layout to which the attachment is transitioned when the render pass is finished

		// Setup attachment references
		VkAttachmentReference colorReference{};
		colorReference.attachment = 0;                                    // Attachment 0 is color
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Attachment layout used as color during the subpass


		// Setup a single subpass reference
		VkSubpassDescription subpassDescription{};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;                            // Subpass uses one color attachment
		subpassDescription.pColorAttachments = &colorReference;                 // Reference to the color attachment in slot 0
		subpassDescription.pDepthStencilAttachment = nullptr;                   // Reference to the depth attachment in slot 1
		subpassDescription.inputAttachmentCount = 0;                            // Input attachments can be used to sample from contents of a previous subpass
		subpassDescription.pInputAttachments = nullptr;                         // (Input attachments not used by this example)
		subpassDescription.preserveAttachmentCount = 0;                         // Preserved attachments can be used to loop (and preserve) attachments through subpasses
		subpassDescription.pPreserveAttachments = nullptr;                      // (Preserve attachments not used by this example)
		subpassDescription.pResolveAttachments = nullptr;                       // Resolve attachments are resolved at the end of a sub pass and can be used for e.g. multi sampling

		// Setup subpass dependencies
		// These will add the implicit attachment layout transitions specified by the attachment descriptions
		// The actual usage layout is preserved through the layout specified in the attachment reference
		// Each subpass dependency will introduce a memory and execution dependency between the source and dest subpass described by
		// srcStageMask, dstStageMask, srcAccessMask, dstAccessMask (and dependencyFlags is set)
		// Note: VK_SUBPASS_EXTERNAL is a special constant that refers to all commands executed outside of the actual renderpass)
		std::array<VkSubpassDependency, 1> dependencies;

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = 0;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		dependencies[0].dependencyFlags = 0;

		// Create the actual renderpass
		VkRenderPassCreateInfo renderPassCI{};
		renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCI.attachmentCount = static_cast<uint32_t>(attachments.size());  // Number of attachments used by this render pass
		renderPassCI.pAttachments = attachments.data();                            // Descriptions of the attachments used by the render pass
		renderPassCI.subpassCount = 1;                                             // We only use one subpass in this example
		renderPassCI.pSubpasses = &subpassDescription;                             // Description of that subpass
		renderPassCI.dependencyCount = static_cast<uint32_t>(dependencies.size()); // Number of subpass dependencies
		renderPassCI.pDependencies = dependencies.data();                          // Subpass dependencies used by the render pass
		VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassCI, nullptr, &renderPass));
	}

	// Vulkan loads its shaders from an immediate binary representation called SPIR-V
	// Shaders are compiled offline from e.g. GLSL using the reference glslang compiler
	// This function loads such a shader from a binary file and returns a shader module structure
	VkShaderModule loadSPIRVShader(std::string filename)
	{
		size_t shaderSize = 0;
		char* shaderCode{ nullptr };

#if defined(__ANDROID__)
		// Load shader from compressed asset
		AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, filename.c_str(), AASSET_MODE_STREAMING);
		assert(asset);
		shaderSize = AAsset_getLength(asset);
		assert(shaderSize > 0);

		shaderCode = new char[shaderSize];
		AAsset_read(asset, shaderCode, shaderSize);
		AAsset_close(asset);
#else
		std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);

		if (is.is_open())
		{
			shaderSize = is.tellg();
			is.seekg(0, std::ios::beg);
			// Copy file contents into a buffer
			shaderCode = new char[shaderSize];
			is.read(shaderCode, shaderSize);
			is.close();
			assert(shaderSize > 0);
		}
#endif
		if (shaderCode)
		{
			// Create a new shader module that will be used for pipeline creation
			VkShaderModuleCreateInfo shaderModuleCI{};
			shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shaderModuleCI.codeSize = shaderSize;
			shaderModuleCI.pCode = (uint32_t*)shaderCode;

			VkShaderModule shaderModule;
			VK_CHECK_RESULT(vkCreateShaderModule(device, &shaderModuleCI, nullptr, &shaderModule));

			delete[] shaderCode;

			return shaderModule;
		}
		else
		{
			std::cerr << "Error: Could not open shader file \"" << filename << "\"" << std::endl;
			return VK_NULL_HANDLE;
		}
	}

	void prepareOffscreenFramebuffers()
	{
		
#if defined(USE_LOWRES_MV)
		velocityWidth = xessInputResolution.x;
		velocityHeight = xessInputResolution.y;
#else
		velocityWidth = width;
		velocityHeight = height;
#endif

		offscreenFrameBuffers.render.setSize(xessInputResolution.x, xessInputResolution.y);

		offscreenFrameBuffers.velocity.setSize(velocityWidth, velocityHeight);

		// Find a suitable depth format
		VkFormat attDepthFormat;
		VkBool32 validDepthFormat = vks::tools::getSupportedDepthFormat(physicalDevice, &attDepthFormat);
		assert(validDepthFormat);
		(void)validDepthFormat;

		// Color buffer
		createAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, &offscreenFrameBuffers.render.color, xessInputResolution.x, xessInputResolution.y);
		createAttachment(attDepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, &offscreenFrameBuffers.render.depth, xessInputResolution.x, xessInputResolution.y);

		// Velocity
		createAttachment(VK_FORMAT_R16G16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, &offscreenFrameBuffers.velocity.velocity, velocityWidth, velocityHeight);

		// Render passes
		{
			std::array<VkAttachmentDescription, 2> attachmentDescs = {};

			// Init attachment properties
			for (uint32_t i = 0; i < static_cast<uint32_t>(attachmentDescs.size()); i++)
			{
				attachmentDescs[i].samples = VK_SAMPLE_COUNT_1_BIT;
				attachmentDescs[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				attachmentDescs[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				attachmentDescs[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachmentDescs[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
#if defined(USE_LOWRES_MV)
				attachmentDescs[i].finalLayout = (i == 1) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
#else
				attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
#endif
			}

			// Formats
			attachmentDescs[0].format = offscreenFrameBuffers.render.color.format;
			attachmentDescs[1].format = offscreenFrameBuffers.render.depth.format;

			std::vector<VkAttachmentReference> colorReferences;
			colorReferences.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

			VkAttachmentReference depthReference = {};
			depthReference.attachment = 1;
			depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.pColorAttachments = colorReferences.data();
			subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
			subpass.pDepthStencilAttachment = &depthReference;

			// Use subpass dependencies for attachment layout transitions
			std::array<VkSubpassDependency, 2> dependencies;

#if defined(USE_LOWRES_MV)
			VkPipelineStageFlags attachmentWriteStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			VkAccessFlags attachmentAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
#else
			VkPipelineStageFlags attachmentWriteStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			VkAccessFlags attachmentAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
#endif

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			dependencies[0].dstStageMask = attachmentWriteStages;
			dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dependencies[0].dstAccessMask = attachmentAccess;
			dependencies[0].dependencyFlags = 0u;

			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = attachmentWriteStages;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			dependencies[1].srcAccessMask = attachmentAccess;
			dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dependencies[1].dependencyFlags = 0u;

			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.pAttachments = attachmentDescs.data();
			renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 2;
			renderPassInfo.pDependencies = dependencies.data();
			VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &offscreenFrameBuffers.render.renderPass));

			std::array<VkImageView, 2> attachments;
			attachments[0] = offscreenFrameBuffers.render.color.view;
			attachments[1] = offscreenFrameBuffers.render.depth.view;

			VkFramebufferCreateInfo fbufCreateInfo = vks::initializers::framebufferCreateInfo();
			fbufCreateInfo.renderPass = offscreenFrameBuffers.render.renderPass;
			fbufCreateInfo.pAttachments = attachments.data();
			fbufCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			fbufCreateInfo.width = offscreenFrameBuffers.render.width;
			fbufCreateInfo.height = offscreenFrameBuffers.render.height;
			fbufCreateInfo.layers = 1;
			VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &offscreenFrameBuffers.render.frameBuffer));
		}

		// Velocity
		{
			VkAttachmentDescription attachmentDescription{};
			attachmentDescription.format = offscreenFrameBuffers.velocity.velocity.format;
			attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
			attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.pColorAttachments = &colorReference;
			subpass.colorAttachmentCount = 1;

			std::array<VkSubpassDependency, 2> dependencies;

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dependencies[0].dstAccessMask =  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = 0u;

			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			dependencies[1].srcAccessMask =  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dependencies[1].dependencyFlags = 0u;

			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.pAttachments = &attachmentDescription;
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 2;
			renderPassInfo.pDependencies = dependencies.data();
			VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &offscreenFrameBuffers.velocity.renderPass));

			VkFramebufferCreateInfo fbufCreateInfo = vks::initializers::framebufferCreateInfo();
			fbufCreateInfo.renderPass = offscreenFrameBuffers.velocity.renderPass;
			fbufCreateInfo.pAttachments = &offscreenFrameBuffers.velocity.velocity.view;
			fbufCreateInfo.attachmentCount = 1;
			fbufCreateInfo.width = offscreenFrameBuffers.velocity.width;
			fbufCreateInfo.height = offscreenFrameBuffers.velocity.height;
			fbufCreateInfo.layers = 1;
			VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &offscreenFrameBuffers.velocity.frameBuffer));
		}
	}

	void createPipelines()
	{
		// Create the graphics pipeline used in this example
		// Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing OpenGL's complex state machine
		// A pipeline is then stored and hashed on the GPU making pipeline changes very fast
		// Note: There are still a few dynamic states that are not directly part of the pipeline (but the info that they are used is)

		VkGraphicsPipelineCreateInfo pipelineCI{};
		pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		// The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
		pipelineCI.layout = pipelineLayout;
		// Renderpass this pipeline is attached to
		pipelineCI.renderPass = offscreenFrameBuffers.render.renderPass;

		// Construct the different states making up the pipeline

		// Input assembly state describes how primitives are assembled
		// This pipeline will assemble vertex data as a triangle lists (though we only use one triangle)
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
		inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		// Rasterization state
		VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
		rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
		rasterizationStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationStateCI.depthClampEnable = VK_FALSE;
		rasterizationStateCI.rasterizerDiscardEnable = VK_FALSE;
		rasterizationStateCI.depthBiasEnable = VK_FALSE;
		rasterizationStateCI.lineWidth = 1.0f;

		// Color blend state describes how blend factors are calculated (if used)
		// We need one blend attachment state per color attachment (even if blending is not used)
		VkPipelineColorBlendAttachmentState blendAttachmentState{};
		blendAttachmentState.colorWriteMask = 0xf;
		blendAttachmentState.blendEnable = VK_FALSE;
		VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
		colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendStateCI.attachmentCount = 1;
		colorBlendStateCI.pAttachments = &blendAttachmentState;

		// Viewport state sets the number of viewports and scissor used in this pipeline
		// Note: This is actually overridden by the dynamic states (see below)
		VkPipelineViewportStateCreateInfo viewportStateCI{};
		viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCI.viewportCount = 1;
		viewportStateCI.scissorCount = 1;

		// Enable dynamic states
		// Most states are baked into the pipeline, but there are still a few dynamic states that can be changed within a command buffer
		// To be able to change these we need do specify which dynamic states will be changed using this pipeline. Their actual states are set later on in the command buffer.
		// For this example we will set the viewport and scissor using dynamic states
		std::vector<VkDynamicState> dynamicStateEnables;
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
		VkPipelineDynamicStateCreateInfo dynamicStateCI{};
		dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
		dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

		// Depth and stencil state containing depth and stencil compare and test operations
		// We only use depth tests and want depth tests and writes to be enabled and compare with less or equal
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
		depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilStateCI.depthTestEnable = VK_TRUE;
		depthStencilStateCI.depthWriteEnable = VK_TRUE;
		depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_ALWAYS;
		depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;
		depthStencilStateCI.back.failOp = VK_STENCIL_OP_KEEP;
		depthStencilStateCI.back.passOp = VK_STENCIL_OP_KEEP;
		depthStencilStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;
		depthStencilStateCI.stencilTestEnable = VK_FALSE;
		depthStencilStateCI.front = depthStencilStateCI.back;

		// Multi sampling state
		// This example does not make use of multi sampling (for anti-aliasing), the state must still be set and passed to the pipeline
		VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
		multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleStateCI.pSampleMask = nullptr;

		// Vertex input descriptions
		// Specifies the vertex input parameters for a pipeline

		// Vertex input binding
		// This example uses a single vertex input binding at binding point 0 (see vkCmdBindVertexBuffers)
		VkVertexInputBindingDescription vertexInputBinding{};
		vertexInputBinding.binding = 0;
		vertexInputBinding.stride = sizeof(Vertex);
		vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		// Input attribute bindings describe shader attribute locations and memory layouts
		std::array<VkVertexInputAttributeDescription, 2> vertexInputAttributs;
		// These match the following shader layout (see triangle.vert):
		//	layout (location = 0) in vec3 inPos;
		//	layout (location = 1) in vec4 inColor;
		// Attribute location 0: Position
		vertexInputAttributs[0].binding = 0;
		vertexInputAttributs[0].location = 0;
		// Position attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
		vertexInputAttributs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexInputAttributs[0].offset = offsetof(Vertex, position);
		// Attribute location 1: Color
		vertexInputAttributs[1].binding = 0;
		vertexInputAttributs[1].location = 1;
		// Color attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32 A32)
		vertexInputAttributs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		vertexInputAttributs[1].offset = offsetof(Vertex, color);

		// Vertex input state used for pipeline creation
		VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};
		vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputStateCI.vertexBindingDescriptionCount = 1;
		vertexInputStateCI.pVertexBindingDescriptions = &vertexInputBinding;
		vertexInputStateCI.vertexAttributeDescriptionCount = 2;
		vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributs.data();

		// Shaders
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

		// Vertex shader
		shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		// Set pipeline stage for this shader
		shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		// Load binary SPIR-V shader
		shaderStages[0].module = loadSPIRVShader(getShadersPath() + "triangle/triangle.vert.spv");
		// Main entry point for the shader
		shaderStages[0].pName = "main";
		assert(shaderStages[0].module != VK_NULL_HANDLE);

		// Fragment shader
		shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		// Set pipeline stage for this shader
		shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		// Load binary SPIR-V shader
		shaderStages[1].module = loadSPIRVShader(getShadersPath() + "triangle/triangle.frag.spv");
		// Main entry point for the shader
		shaderStages[1].pName = "main";
		assert(shaderStages[1].module != VK_NULL_HANDLE);

		// Set pipeline shader stage info
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();

		// Assign the pipeline states to the pipeline creation info structure
		pipelineCI.pVertexInputState = &vertexInputStateCI;
		pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
		pipelineCI.pRasterizationState = &rasterizationStateCI;
		pipelineCI.pColorBlendState = &colorBlendStateCI;
		pipelineCI.pMultisampleState = &multisampleStateCI;
		pipelineCI.pViewportState = &viewportStateCI;
		pipelineCI.pDepthStencilState = &depthStencilStateCI;
		pipelineCI.pDynamicState = &dynamicStateCI;

		// Create rendering pipeline using the specified states
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));

		// Shader modules are no longer needed once the graphics pipeline has been created
		vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
		vkDestroyShaderModule(device, shaderStages[1].module, nullptr);

		// Vertex shader
		shaderStages[0].module = loadSPIRVShader(getShadersPath() + "triangle/velocity.vert.spv");
		// Fragment shader
		shaderStages[1].module = loadSPIRVShader(getShadersPath() + "triangle/velocity.frag.spv");

		pipelineCI.renderPass = offscreenFrameBuffers.velocity.renderPass;

		// Create rendering pipeline using the specified states
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &velocityPipeline));

		// Shader modules are no longer needed once the graphics pipeline has been created
		vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
		vkDestroyShaderModule(device, shaderStages[1].module, nullptr);

		// Vertex shader
		shaderStages[0].module = loadSPIRVShader(getShadersPath() + "triangle/quad.vert.spv");
		// Fragment shader
		shaderStages[1].module = loadSPIRVShader(getShadersPath() + "triangle/quad.frag.spv");

		pipelineCI.renderPass = renderPass;
		pipelineCI.layout = fsqPipelineLayout;

		// Create rendering pipeline using the specified states
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &fsqPipeline));

		// Shader modules are no longer needed once the graphics pipeline has been created
		vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
		vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
	}

	void createUniformBuffers()
	{
		// Prepare and initialize the per-frame uniform buffer blocks containing shader uniforms
		// Single uniforms like in OpenGL are no longer present in Vulkan. All Shader uniforms are passed via uniform buffer blocks
		VkMemoryRequirements memReqs;

		// Vertex shader uniform buffer block
		VkBufferCreateInfo bufferInfo{};
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.allocationSize = 0;
		allocInfo.memoryTypeIndex = 0;

		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(ShaderData);
		// This buffer will be used as a uniform buffer
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		// Create the buffers
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
			VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &uniformBuffers[i].buffer));
			// Get memory requirements including size, alignment and memory type
			vkGetBufferMemoryRequirements(device, uniformBuffers[i].buffer, &memReqs);
			allocInfo.allocationSize = memReqs.size;
			// Get the memory type index that supports host visible memory access
			// Most implementations offer multiple memory types and selecting the correct one to allocate memory from is crucial
			// We also want the buffer to be host coherent so we don't have to flush (or sync after every update.
			// Note: This may affect performance so you might not want to do this in a real world application that updates buffers on a regular base
			allocInfo.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			// Allocate memory for the uniform buffer
			VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &(uniformBuffers[i].memory)));
			// Bind memory to buffer
			VK_CHECK_RESULT(vkBindBufferMemory(device, uniformBuffers[i].buffer, uniformBuffers[i].memory, 0));
			// We map the buffer once, so we can update it without having to map it again
			VK_CHECK_RESULT(vkMapMemory(device, uniformBuffers[i].memory, 0, sizeof(ShaderData), 0, (void**)&uniformBuffers[i].mapped));
		}

	}

	void createSampler()
	{
		// Create sampler
		VkSamplerCreateInfo samplerCI = vks::initializers::samplerCreateInfo();
		samplerCI.magFilter = VK_FILTER_LINEAR;
		samplerCI.minFilter = VK_FILTER_LINEAR;
		samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCI.mipLodBias = 0.0f;
		samplerCI.maxAnisotropy = 1.0f;
		samplerCI.compareOp = VK_COMPARE_OP_NEVER;
		samplerCI.minLod = 0.0f;
		samplerCI.maxLod = 1.0f;
		samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(device, &samplerCI, nullptr, &fsqSampler));
	}

	void prepare()
	{
		setupXess();
		VulkanExampleBase::prepare();
		prepareOffscreenFramebuffers();
		createSynchronizationPrimitives();
		createCommandBuffers();
		createVertexBuffer();
		createUniformBuffers();
		createSampler();
		createDescriptorSetLayout();
		createDescriptorPool();
		createDescriptorSets();

		createPipelines();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;

		// Use a fence to wait until the command buffer has finished execution before using it again
		vkWaitForFences(device, 1, &waitFences[currentFrame], VK_TRUE, UINT64_MAX);

		// Get the next swap chain image from the implementation
		// Note that the implementation is free to return the images in any order, so we must use the acquire function and can't just cycle through the images
		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(device, swapChain.swapChain, UINT64_MAX, presentCompleteSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			windowResize();
			return;
		} else if ((result != VK_SUCCESS) && (result != VK_SUBOPTIMAL_KHR)) {
			throw "Could not acquire the next swap chain image!";
		}

		// Update the uniform buffer for the next frame
		if (last_shader_data_time.time_since_epoch().count() == 0)
		{
			last_shader_data_time = std::chrono::high_resolution_clock::now();
		}

		auto current_time = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed_seconds = current_time - last_shader_data_time;
		last_shader_data_time = current_time;

		const double speed = 1. / 2; //4 second for screen
		float translationSpeed = m_pause ? 0.f : (float)(speed * elapsed_seconds.count());
		const float offsetBounds = 1.25f;

		m_uniformBufferData.offset.x += translationSpeed;
		if (m_uniformBufferData.offset.x > offsetBounds)
		{
			m_uniformBufferData.offset.x = -offsetBounds;
		}

		auto haltonValue = m_haltonPointSet[m_haltonIndex];
		m_haltonIndex = (m_haltonIndex + 1) % m_haltonPointSet.size();

		jitter[0] = haltonValue.first;
		jitter[1] = haltonValue.second;

		m_uniformBufferData.offset.z = jitter[0];
		m_uniformBufferData.offset.w = jitter[1];

		m_uniformBufferData.resolution.x = (float)xessInputResolution.x;
		m_uniformBufferData.resolution.y = (float)xessInputResolution.y;

		m_uniformBufferData.velocity.x = -translationSpeed * ((float)width / 2.f);


		// Copy the current matrices to the current frame's uniform buffer
		// Note: Since we requested a host coherent memory type for the uniform buffer, the write is instantly visible to the GPU
		memcpy(uniformBuffers[currentBuffer].mapped, &m_uniformBufferData, sizeof(m_uniformBufferData));

		VK_CHECK_RESULT(vkResetFences(device, 1, &waitFences[currentFrame]));

		// Build the command buffer
		// Unlike in OpenGL all rendering commands are recorded into command buffers that are then submitted to the queue
		// This allows to generate work upfront in a separate thread
		// For basic command buffers (like in this sample), recording is so fast that there is no need to offload this

		vkResetCommandBuffer(commandBuffers[currentBuffer], 0);

		VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		// Set clear values for all framebuffer attachments with loadOp set to clear
		// We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear values for both
		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[1].depthStencil = { 0.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.renderPass = offscreenFrameBuffers.render.renderPass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = xessInputResolution.x;
		renderPassBeginInfo.renderArea.extent.height = xessInputResolution.y;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;
		renderPassBeginInfo.framebuffer = offscreenFrameBuffers.render.frameBuffer;
		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffers[currentBuffer], &cmdBufInfo));

		// Start the first sub pass specified in our default render pass setup by the base class
		// This will clear the color and depth attachment
		vkCmdBeginRenderPass(commandBuffers[currentBuffer], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		// Update dynamic viewport state
		VkViewport viewport{};
		viewport.height = (float)xessInputResolution.y;
		viewport.width = (float)xessInputResolution.x;
		viewport.minDepth = (float)0.0f;
		viewport.maxDepth = (float)1.0f;
		vkCmdSetViewport(commandBuffers[currentBuffer], 0, 1, &viewport);
		// Update dynamic scissor state
		VkRect2D scissor{};
		scissor.extent.width = xessInputResolution.x;
		scissor.extent.height = xessInputResolution.y;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		vkCmdSetScissor(commandBuffers[currentBuffer], 0, 1, &scissor);
		// Bind descriptor set for the currrent frame's uniform buffer, so the shader uses the data from that buffer for this draw
		vkCmdBindDescriptorSets(commandBuffers[currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &uniformBuffers[currentBuffer].descriptorSet, 0, nullptr);
		// Bind the rendering pipeline
		// The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states specified at pipeline creation time
		vkCmdBindPipeline(commandBuffers[currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		// Bind triangle vertex buffer (contains position and colors)
		VkDeviceSize offsets[1]{ 0 };
		vkCmdBindVertexBuffers(commandBuffers[currentBuffer], 0, 1, &vertices.buffer, offsets);
		// Draw triangle
		vkCmdDraw(commandBuffers[currentBuffer], vertices.count, 1, 0, 0);
		vkCmdEndRenderPass(commandBuffers[currentBuffer]);


		// Velocity pass
		// Update dynamic viewport state
		viewport.height = (float)velocityHeight;
		viewport.width = (float)velocityWidth;
		viewport.minDepth = (float)0.0f;
		viewport.maxDepth = (float)1.0f;
		vkCmdSetViewport(commandBuffers[currentBuffer], 0, 1, &viewport);
		// Update dynamic scissor state
		scissor.extent.width = velocityWidth;
		scissor.extent.height = velocityHeight;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		vkCmdSetScissor(commandBuffers[currentBuffer], 0, 1, &scissor);

		renderPassBeginInfo.renderPass = offscreenFrameBuffers.velocity.renderPass;
		renderPassBeginInfo.framebuffer = offscreenFrameBuffers.velocity.frameBuffer;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = velocityWidth;
		renderPassBeginInfo.renderArea.extent.height = velocityHeight;
		renderPassBeginInfo.clearValueCount = 1;
		vkCmdBeginRenderPass(commandBuffers[currentBuffer], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	
		// Bind the rendering pipeline
		// The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states specified at pipeline creation time
		vkCmdBindPipeline(commandBuffers[currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, velocityPipeline);
		
		// Draw triangle
		vkCmdDraw(commandBuffers[currentBuffer], vertices.count, 1, 0, 0);
		vkCmdEndRenderPass(commandBuffers[currentBuffer]);

		{
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			imageMemoryBarrier.image = xessOutput.image;
			imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			vkCmdPipelineBarrier(commandBuffers[currentBuffer], VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
		}

		// Run XeSS
		{
			xess_vk_execute_params_t exec_params{};
			exec_params.inputWidth = xessInputResolution.x;
			exec_params.inputHeight = xessInputResolution.y;
			exec_params.jitterOffsetX = jitter[0];
			exec_params.jitterOffsetY = -jitter[1];
			exec_params.exposureScale = 1.0f;

			exec_params.colorTexture = { offscreenFrameBuffers.render.color.view, offscreenFrameBuffers.render.color.image, { VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u}, offscreenFrameBuffers.render.color.format, xessInputResolution.x, xessInputResolution.y };
			exec_params.velocityTexture = { offscreenFrameBuffers.velocity.velocity.view, offscreenFrameBuffers.velocity.velocity.image, { VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u}, offscreenFrameBuffers.velocity.velocity.format, width, height };
			exec_params.outputTexture = { xessOutput.view, xessOutput.image, { VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u}, xessOutput.format, width, height };
#if defined(USE_LOWRES_MV)
			exec_params.depthTexture = { offscreenFrameBuffers.render.depth.view, offscreenFrameBuffers.render.depth.image, { VK_IMAGE_ASPECT_DEPTH_BIT, 0u, 1u, 0u, 1u}, offscreenFrameBuffers.render.depth.format, xessInputResolution.x, xessInputResolution.y };
#else
			exec_params.depthTexture.image = nullptr;
			exec_params.depthTexture.imageView = nullptr;
#endif
			exec_params.exposureScaleTexture.image = nullptr;
			exec_params.exposureScaleTexture.imageView = nullptr;
			auto status = xessVKExecute(xessContext, commandBuffers[currentBuffer], &exec_params);
			if (status != XESS_RESULT_SUCCESS)
			{
				throw std::runtime_error("Unable to run XeSS");
			}
		}

		{
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			imageMemoryBarrier.image = xessOutput.image;
			imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			vkCmdPipelineBarrier(commandBuffers[currentBuffer], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
		}

		// Final pass
		// Update dynamic viewport state
		viewport.height = (float)height;
		viewport.width = (float)width;
		viewport.minDepth = (float)0.0f;
		viewport.maxDepth = (float)1.0f;
		vkCmdSetViewport(commandBuffers[currentBuffer], 0, 1, &viewport);
		// Update dynamic scissor state
		scissor.extent.width = width;
		scissor.extent.height = height;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		vkCmdSetScissor(commandBuffers[currentBuffer], 0, 1, &scissor);
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.framebuffer = frameBuffers[imageIndex];
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = 1;
		vkCmdBeginRenderPass(commandBuffers[currentBuffer], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Bind the rendering pipeline
		// The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states specified at pipeline creation time
		vkCmdBindPipeline(commandBuffers[currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, fsqPipeline);

		vkCmdBindDescriptorSets(commandBuffers[currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, fsqPipelineLayout, 0, 1, &fsqDescriptorSet, 0, nullptr);

		// Draw triangle
		vkCmdDraw(commandBuffers[currentBuffer], 3, 1, 0, 0);
		vkCmdEndRenderPass(commandBuffers[currentBuffer]);


		// Ending the render pass will add an implicit barrier transitioning the frame buffer color attachment to
		// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system
		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffers[currentBuffer]));

		// Submit the command buffer to the graphics queue

		// Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
		VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		// The submit info structure specifies a command buffer queue submission batch
		VkSubmitInfo submitInfo_{};
		submitInfo_.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo_.pWaitDstStageMask = &waitStageMask;               // Pointer to the list of pipeline stages that the semaphore waits will occur at
		submitInfo_.waitSemaphoreCount = 1;                           // One wait semaphore
		submitInfo_.signalSemaphoreCount = 1;                         // One signal semaphore
		submitInfo_.pCommandBuffers = &commandBuffers[currentBuffer]; // Command buffers(s) to execute in this batch (submission)
		submitInfo_.commandBufferCount = 1;                           // One command buffer

		// Semaphore to wait upon before the submitted command buffer starts executing
		submitInfo_.pWaitSemaphores = &presentCompleteSemaphores[currentFrame];
		// Semaphore to be signaled when command buffers have completed
		submitInfo_.pSignalSemaphores = &renderCompleteSemaphores[currentFrame];

		// Submit to the graphics queue passing a wait fence
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo_, waitFences[currentFrame]));

		// Present the current frame buffer to the swap chain
		// Pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap chain presentation
		// This ensures that the image is not presented to the windowing system until all commands have been submitted

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderCompleteSemaphores[currentFrame];
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChain.swapChain;
		presentInfo.pImageIndices = &imageIndex;
		result = vkQueuePresentKHR(queue, &presentInfo);

		if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
			windowResize();
		}
		else if (result != VK_SUCCESS) {
			throw "Could not present the image to the swap chain!";
		}

	}
};

// OS specific main entry points
// Most of the code base is shared for the different supported operating systems, but stuff like message handling differs

#if defined(_WIN32)
// Windows entry point
VulkanExample *vulkanExample;
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleMessages(hWnd, uMsg, wParam, lParam);
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	try
	{
		for (size_t i = 0; i < __argc; i++)
		{
			VulkanExample::args.push_back(__argv[i]);
		};
		vulkanExample = new VulkanExample();
		vulkanExample->initVulkan();
		vulkanExample->setupWindow(hInstance, WndProc);
		vulkanExample->prepare();
		vulkanExample->renderLoop();
		delete(vulkanExample);
		return 0;
	}
	catch (const std::exception& err)
	{
		MessageBoxA(NULL, (std::string("XeSS-SR sample error: ") + err.what()).c_str(), "Error", MB_OK | MB_TOPMOST | MB_ICONINFORMATION);
		return -1;
	}
}

#elif defined(_DIRECT2DISPLAY)

// Linux entry point with direct to display wsi
// Direct to Displays (D2D) is used on embedded platforms
VulkanExample *vulkanExample;
static void handleEvent()
{
}
int main(const int argc, const char *argv[])
{
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
VulkanExample *vulkanExample;
static void handleEvent(const DFBWindowEvent *event)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleEvent(event);
	}
}
int main(const int argc, const char *argv[])
{
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->setupWindow();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
VulkanExample *vulkanExample;
int main(const int argc, const char *argv[])
{
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->setupWindow();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
#elif defined(__linux__) || defined(__FreeBSD__)

// Linux entry point
VulkanExample *vulkanExample;
#if defined(VK_USE_PLATFORM_XCB_KHR)
static void handleEvent(const xcb_generic_event_t *event)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleEvent(event);
	}
}
#else
static void handleEvent()
{
}
#endif
int main(const int argc, const char *argv[])
{
	try
	{
		for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
		vulkanExample = new VulkanExample();
		vulkanExample->initVulkan();
		vulkanExample->setupWindow();
		vulkanExample->prepare();
		vulkanExample->renderLoop();
		delete(vulkanExample);
	}
	catch(const std::runtime_error &err)
	{
		std::cerr<<err.what()<<std::endl;
		return 1;
	}
	return 0;
}
#endif
