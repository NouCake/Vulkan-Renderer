#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include <iostream>
#include <optional>
#include <fstream>


#include "ImGUI/imgui.h"
#include "ImGUI/imgui_impl_vulkan.h"
#include "ImGUI/imgui_impl_win32.h"

class HelloTriangleApplication {
private:
	static std::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::binary | std::ios::ate);

		if (file) {
			size_t fileSize = file.tellg();
			file.seekg(0);

			std::vector<char> buffer(fileSize);
			file.read(buffer.data(), fileSize);
			file.close();

			return buffer;
		}

		throw std::runtime_error("Failed to open");
	}
public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}


private:
	const uint32_t WINDOW_WIDTH = 640;
	const uint32_t WINDOW_HEIGHT = 480;

	const size_t MAX_FRAMES_IN_FLIGHT = 2;

	GLFWwindow* mWindow;

	vk::Instance mInstance;
	vk::PhysicalDevice mPhysDevice;
	vk::Device mDevice;
	vk::Queue mQueue;
	vk::SurfaceKHR mSurface;
	vk::SwapchainKHR mSwapchain;
	std::vector<vk::Image> mSwapchainImages;
	vk::Format mSwapchainFormat;
	vk::Extent2D mSwapchainExtent;
	std::vector<vk::ImageView> mSwapchainImageViews;
	std::vector<vk::Framebuffer> mSwapchainFramebuffer;
	vk::PipelineLayout mPipelineLayout;
	vk::RenderPass mRenderPass;
	vk::Pipeline mPipeline;
	vk::CommandPool mCommandPool;
	std::vector<vk::CommandBuffer> mCommandBuffers;

	vk::Buffer mVertexBuffer;
	vk::Buffer mIndexBuffer;
	vk::DeviceMemory mVertexBufferMemory;
	vk::DeviceMemory mIndexBufferMemory;

	std::vector<vk::Semaphore> mImageAquiredSemaphores;
	std::vector<vk::Semaphore> mRenderFinishedSemaphores;
	std::vector<vk::Fence> mFlightFence;

	struct Vertex {
		glm::vec2 pos;
		glm::vec3 color;

		static vk::VertexInputBindingDescription getBindingDesc() {
			vk::VertexInputBindingDescription desc(0, sizeof(Vertex), vk::VertexInputRate::eVertex);
			return desc;
		}

		static std::vector<vk::VertexInputAttributeDescription> getAttrDesc() {
			std::vector<vk::VertexInputAttributeDescription> desc = {
				vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos)),
				vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color))
			};
			return desc;
		}
	};
	struct SwapchainSupportDetails {
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> presentModes;
	};
	SwapchainSupportDetails querrySwapchainSupportDetails(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
		SwapchainSupportDetails details;
		details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
		details.formats = device.getSurfaceFormatsKHR(surface);
		details.presentModes = device.getSurfacePresentModesKHR(surface);
		return details;
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(mWindow)) {
			glfwPollEvents();
			drawFrame();
		}
	}


	void  (*build_framebuffer_callback(HelloTriangleApplication* a))(GLFWwindow*, int, int, int, int) {
		static HelloTriangleApplication* app = a;

		void (*callback)(GLFWwindow*, int, int, int, int) = ([](GLFWwindow* window, int key, int scancode, int action, int mods) {
			//uint32_t i = app->drawFrame();
			//std::cout << "Hello Framus!" << i << std::endl;
		});

		return callback;
	}

	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
		vk::PhysicalDeviceMemoryProperties memProps = mPhysDevice.getMemoryProperties();

		for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
			if (typeFilter & (1 << i) && (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		return 0;
	}

	void initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		mWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr);

		//GLFWkeyfun f = void GLFWkeyfun keyCallback(GLFWwindow * window, int key, int scancode, int action, int mods) {};
		//glfwSetKeyCallback(mWindow, build_framebuffer_callback(this));
	}

	void cleanup() {
		glfwDestroyWindow(mWindow);
		glfwTerminate();
	}

	//Ripped from: https://github.com/KhronosGroup/Vulkan-Hpp/blob/master/samples/EnableValidationWithCallback/EnableValidationWithCallback.cpp
	bool checkLayers(std::vector<char const*> const& layers, std::vector<vk::LayerProperties> const& properties) {
		// return true if all layers are listed in the properties
		return std::all_of(layers.begin(), layers.end(), [&properties](char const* name) {
			return std::find_if(properties.begin(), properties.end(), [&name](vk::LayerProperties const& property) { return strcmp(property.layerName, name) == 0; }) != properties.end();
		});
	}
	
	void drawFrame() {
		static int curFrame = 0;
		mDevice.waitForFences(mFlightFence[curFrame], VK_TRUE, UINT64_MAX);
		mDevice.resetFences(mFlightFence[curFrame]);
		uint32_t imageIndex = mDevice.acquireNextImageKHR(mSwapchain, UINT64_MAX, mImageAquiredSemaphores[curFrame], nullptr).value;
		vk::PipelineStageFlags waitStage(vk::PipelineStageFlagBits::eColorAttachmentOutput);

		vk::SubmitInfo submitInfo(1, &mImageAquiredSemaphores[curFrame], &waitStage, 1, &(mCommandBuffers[imageIndex]), 1, &mRenderFinishedSemaphores[curFrame]);
		mQueue.submit(1, &submitInfo, mFlightFence[curFrame]);
		
		vk::PresentInfoKHR presentInfo(1, &mRenderFinishedSemaphores[curFrame], 1, &mSwapchain, &imageIndex);
		mQueue.presentKHR(presentInfo);

		curFrame = (curFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void initVulkan() {
		createInstance();
		createSurface();
		pickPhysicalDevice();
		createDevice();
		createSwapChain();
		createRenderPass();
		initImgui();
		createGraphicsPipeline();
		createFrameBuffers();
		createCommandPool();
		createIndexBuffer();
		createVertexBuffer();
		createCommandBuffers();
		createSemaphores();
	}
	void initImgui() {
		/*
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.IniFilename = "ImGUI/imgui.ini";
		ImGui::StyleColorsDark();

		ImGui_ImplVulkan_InitInfo initInfo = { };
		initInfo.Instance = mInstance;
		initInfo.PhysicalDevice = mPhysDevice;
		initInfo.Device = mDevice;
		initInfo.QueueFamily = findQueueFamilies(mPhysDevice, vk::QueueFlagBits::eGraphics);
		initInfo.Queue = mQueue;
		initInfo.PipelineCache = NULL;
		initInfo.DescriptorPool = NULL; //TODO
		initInfo.MinImageCount = 2;
		initInfo.ImageCount = 2;
		initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		initInfo.Allocator = NULL;

		//ImGui_ImplVulkan_Init(&initInfo, mRenderPass);
		*/
	}
	void createBuffer(uint32_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryProperties,
			vk::Buffer& buffer, vk::DeviceMemory& memory) {
		uint32_t queueFamily = findQueueFamilies(mPhysDevice, vk::QueueFlagBits::eGraphics);
		vk::BufferCreateInfo bufferCreateInfo({}, size, usage, vk::SharingMode::eExclusive);
		buffer = mDevice.createBuffer(bufferCreateInfo);

		vk::MemoryRequirements requirements = mDevice.getBufferMemoryRequirements(buffer);
		vk::MemoryAllocateInfo allocateInfo(requirements.size, findMemoryType(requirements.memoryTypeBits, memoryProperties));
		memory = mDevice.allocateMemory(allocateInfo);
		mDevice.bindBufferMemory(buffer, memory, 0);
	}
	void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, uint32_t size) {
		vk::CommandBufferAllocateInfo allocateInfo(mCommandPool, vk::CommandBufferLevel::ePrimary, 1);
		vk::CommandBuffer tmpBuffer = mDevice.allocateCommandBuffers(allocateInfo)[0];
		
		vk::BufferCopy region(0, 0, size);

		tmpBuffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
		tmpBuffer.copyBuffer(srcBuffer, dstBuffer, region);
		tmpBuffer.end();

		vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &tmpBuffer, 0, nullptr);
		mQueue.submit(submitInfo, nullptr);
		mQueue.waitIdle();
	}
	void createIndexBuffer() {
		std::vector<uint16_t> indecies = {
			0, 1, 2, 2, 3, 0
		};

		vk::Buffer stagingBuffer;
		vk::DeviceMemory statgingBufferMemory;

		uint32_t bufferByteSize = indecies.size() * sizeof(uint16_t);
		vk::MemoryPropertyFlags props = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
		createBuffer(bufferByteSize, vk::BufferUsageFlagBits::eTransferSrc, props, stagingBuffer, statgingBufferMemory);

		void* data = mDevice.mapMemory(statgingBufferMemory, 0, bufferByteSize);
		memcpy(data, indecies.data(), bufferByteSize);
		mDevice.unmapMemory(statgingBufferMemory);

		vk::BufferUsageFlags indeciesUsage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer;
		vk::MemoryPropertyFlags vertexProps = vk::MemoryPropertyFlagBits::eDeviceLocal;
		createBuffer(bufferByteSize, indeciesUsage, vertexProps, mIndexBuffer, mIndexBufferMemory);

		copyBuffer(stagingBuffer, mIndexBuffer, bufferByteSize);

		mDevice.destroyBuffer(stagingBuffer);
		mDevice.freeMemory(statgingBufferMemory);
	}
	void createVertexBuffer() {
		std::vector<Vertex> vertecies = {
			{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
			{{0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}},
			{{0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}},
			{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
		};

		vk::Buffer stagingBuffer;
		vk::DeviceMemory statgingBufferMemory;

		uint32_t bufferByteSize = vertecies.size() * sizeof(Vertex);
		vk::MemoryPropertyFlags props = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
		createBuffer(bufferByteSize, vk::BufferUsageFlagBits::eTransferSrc, props, stagingBuffer, statgingBufferMemory);

		void* data = mDevice.mapMemory(statgingBufferMemory, 0, bufferByteSize);
		memcpy(data, vertecies.data(), bufferByteSize);
		mDevice.unmapMemory(statgingBufferMemory);

		vk::BufferUsageFlags vertexUsage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer;
		vk::MemoryPropertyFlags vertexProps = vk::MemoryPropertyFlagBits::eDeviceLocal;
		createBuffer(bufferByteSize, vertexUsage, vertexProps, mVertexBuffer, mVertexBufferMemory);

		copyBuffer(stagingBuffer, mVertexBuffer, bufferByteSize);

		mDevice.destroyBuffer(stagingBuffer);
		mDevice.freeMemory(statgingBufferMemory);
	}
	void createSemaphores() {
		mImageAquiredSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		mRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		mFlightFence.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			mImageAquiredSemaphores[i] = mDevice.createSemaphore(vk::SemaphoreCreateInfo());
			mRenderFinishedSemaphores[i] = mDevice.createSemaphore(vk::SemaphoreCreateInfo());
			mFlightFence[i] = mDevice.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
		}
	}
	void createCommandBuffers() {
		vk::CommandBufferAllocateInfo allocateInfo(mCommandPool, vk::CommandBufferLevel::ePrimary, mSwapchainImages.size());
		mCommandBuffers = mDevice.allocateCommandBuffers(allocateInfo);
		vk::ClearValue clearValues(std::array<float, 4>({ 0.0f, 0.25f, 0.8f, 1.0f }));
		vk::Rect2D extent = vk::Rect2D({ (uint32_t)0, (uint32_t)0 }, { WINDOW_WIDTH, WINDOW_HEIGHT });
		for (size_t i = 0; i < mCommandBuffers.size(); i++) {
			

			vk::RenderPassBeginInfo rpBeginInfo(mRenderPass, mSwapchainFramebuffer[i], extent, 1, &clearValues);
		
			mCommandBuffers[i].begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse));
			mCommandBuffers[i].beginRenderPass(rpBeginInfo, vk::SubpassContents::eInline);
			mCommandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline);
			mCommandBuffers[i].bindIndexBuffer(mIndexBuffer, 0, vk::IndexType::eUint16);
			mCommandBuffers[i].bindVertexBuffers(0, mVertexBuffer, { 0 });
			mCommandBuffers[i].drawIndexed(6, 1, 0, 0, 0);
			mCommandBuffers[i].endRenderPass();
			mCommandBuffers[i].end();
		}
	}
	void createCommandPool() {
		uint32_t queueFamily = findQueueFamilies(mPhysDevice, vk::QueueFlagBits::eGraphics);
		vk::CommandPoolCreateInfo poolCreateInfo({}, queueFamily);
		mCommandPool = mDevice.createCommandPool(poolCreateInfo);
	}
	void createFrameBuffers() {
		mSwapchainFramebuffer.resize(mSwapchainImages.size());
		for (size_t i = 0; i < mSwapchainImages.size(); i++) {
			vk::FramebufferCreateInfo bufferCreateInfo({}, mRenderPass, 1, &(mSwapchainImageViews[i]), mSwapchainExtent.width, mSwapchainExtent.height, 1);
			mSwapchainFramebuffer[i] = mDevice.createFramebuffer(bufferCreateInfo);
		}
	}
	void createRenderPass() {
		vk::AttachmentDescription colorAttachment({}, mSwapchainFormat, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);
		vk::AttachmentReference colorRef(0, vk::ImageLayout::eColorAttachmentOptimal);

		vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorRef, nullptr, nullptr, 0, nullptr);

		vk::SubpassDependency dependency(VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

		vk::RenderPassCreateInfo renderpassCreateInfo({}, 1, &colorAttachment, 1, &subpass, 1, &dependency);
		mRenderPass = mDevice.createRenderPass(renderpassCreateInfo);
	}
	vk::ShaderModule createShaderModule(std::string filename) {
		std::vector<char> shaderCode = readFile(filename);
		vk::ShaderModuleCreateInfo moduleCreateInfo(vk::ShaderModuleCreateFlags(), shaderCode.size(), (uint32_t*)shaderCode.data());
		return mDevice.createShaderModule(moduleCreateInfo);
	}
	void createGraphicsPipeline() {
		vk::ShaderModule vert = createShaderModule("shaders/vert.spv");
		vk::ShaderModule frag = createShaderModule("shaders/frag.spv");

		vk::PipelineShaderStageCreateInfo vertStageCreateInfo(vk::PipelineShaderStageCreateFlagBits(), vk::ShaderStageFlagBits::eVertex, vert, "main");
		vk::PipelineShaderStageCreateInfo fragStageCreateInfo(vk::PipelineShaderStageCreateFlagBits(), vk::ShaderStageFlagBits::eFragment, frag, "main");

		vk::PipelineShaderStageCreateInfo stageInfos[2] = {
			vertStageCreateInfo, fragStageCreateInfo
		};

		vk::Viewport viewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0f, 1.0f);
		vk::Rect2D scissor({ (uint32_t)0, (uint32_t)0 }, { WINDOW_WIDTH, WINDOW_HEIGHT });
		vk::PipelineColorBlendAttachmentState blendAttachmentState(VK_FALSE, vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
		float blendConstants[] = { 0.0f, 0.0f, 0.0f, 0.0f };

		vk::VertexInputBindingDescription vertBindings = Vertex::getBindingDesc();
		std::vector<vk::VertexInputAttributeDescription> vertAttributes = Vertex::getAttrDesc();

		vk::PipelineVertexInputStateCreateInfo vertexInputCreateInfo(vk::PipelineVertexInputStateCreateFlags(), 1, &vertBindings, vertAttributes.size(), vertAttributes.data());
		vk::PipelineInputAssemblyStateCreateInfo assemblyCreateInfo(vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleList, VK_FALSE);
		vk::PipelineViewportStateCreateInfo viewportStateCreateInfo(vk::PipelineViewportStateCreateFlags(), 1, &viewport, 1, &scissor);
		vk::PipelineRasterizationStateCreateInfo rasterCreateInfo(vk::PipelineRasterizationStateCreateFlags(), VK_FALSE, VK_FALSE, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise, VK_FALSE, 0, 0, 0, 1.0f);
		vk::PipelineMultisampleStateCreateInfo multisampleCreateInfo(vk::PipelineMultisampleStateCreateFlags(), vk::SampleCountFlagBits::e1, VK_FALSE, 1.0f, nullptr, VK_FALSE, VK_FALSE);
		vk::PipelineColorBlendStateCreateInfo blendStateCreateInfo(vk::PipelineColorBlendStateCreateFlagBits(), VK_FALSE, vk::LogicOp::eCopy, 1, &blendAttachmentState, std::array<float, 4>({ 0.0f, 0.0f, 0.0f, 0.0f }));
		vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo(vk::PipelineDynamicStateCreateFlags(), 0, nullptr);

		vk::PipelineLayoutCreateInfo layoutCreateInfo({}, 0, nullptr, 0, nullptr);
		mPipelineLayout = mDevice.createPipelineLayout(layoutCreateInfo);

		vk::GraphicsPipelineCreateInfo pipelineCreateInfo({}, 2, stageInfos, &vertexInputCreateInfo, &assemblyCreateInfo, nullptr, &viewportStateCreateInfo, &rasterCreateInfo, &multisampleCreateInfo, nullptr, &blendStateCreateInfo, &dynamicStateCreateInfo, mPipelineLayout, mRenderPass, 0, {}, -1);
		mPipeline = mDevice.createGraphicsPipeline(nullptr, pipelineCreateInfo);
	}
	vk::SurfaceFormatKHR chooseSwapchainFormat(const SwapchainSupportDetails& details) {
		vk::SurfaceFormatKHR format;
		for (const auto& format : details.formats) {
			if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
				return format;
		}
		return details.formats[0];
	}
	vk::PresentModeKHR choosePresentMode(const SwapchainSupportDetails& details) {
		for (const auto& mode : details.presentModes) {
			if (mode == vk::PresentModeKHR::eMailbox) {
				return mode;
			}
		}

		return vk::PresentModeKHR::eFifo; //Guranteed to be implemented
	}
	vk::Extent2D chooseExtent(const SwapchainSupportDetails& details) {
		if (details.capabilities.currentExtent.width != UINT32_MAX) {
			return details.capabilities.currentExtent;
		}
		else {
			vk::Extent2D extent(WINDOW_WIDTH, WINDOW_HEIGHT);
			extent.width = std::clamp(extent.width, details.capabilities.minImageExtent.width, details.capabilities.maxImageExtent.width);
			extent.height = std::clamp(extent.height, details.capabilities.minImageExtent.height, details.capabilities.maxImageExtent.height);
			return extent;
		}
	}
	void createSwapChain() {
		SwapchainSupportDetails details = querrySwapchainSupportDetails(mPhysDevice, mSurface);

		uint32_t minImageCount = details.capabilities.minImageCount;
		vk::SurfaceFormatKHR surfaceFormat = chooseSwapchainFormat(details);
		vk::Extent2D imageExtent = chooseExtent(details);
		vk::PresentModeKHR presentMode = choosePresentMode(details);
		if (minImageCount + 1 <= details.capabilities.maxImageCount) minImageCount += 1;

		vk::SwapchainCreateInfoKHR swapChainCreateInfo(vk::SwapchainCreateFlagsKHR(), mSurface, 
			minImageCount, surfaceFormat.format, surfaceFormat.colorSpace,
			imageExtent, 1, vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive, 0, nullptr, 
			details.capabilities.currentTransform, vk::CompositeAlphaFlagBitsKHR::eOpaque, presentMode, true, nullptr);
		
		mSwapchain = mDevice.createSwapchainKHR(swapChainCreateInfo);
		mSwapchainImages = mDevice.getSwapchainImagesKHR(mSwapchain);
		mSwapchainFormat = surfaceFormat.format;
		mSwapchainExtent = imageExtent;

		mSwapchainImageViews.reserve(mSwapchainImages.size());
		vk::ComponentMapping compMap(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA);
		vk::ImageSubresourceRange subRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
		for (size_t i = 0; i < mSwapchainImages.size(); i++) {
			vk::ImageViewCreateInfo imageViewCreateInfo(vk::ImageViewCreateFlags(), mSwapchainImages[i], vk::ImageViewType::e2D, mSwapchainFormat, compMap, subRange);
			mSwapchainImageViews.push_back(mDevice.createImageView(imageViewCreateInfo));
		}
	}
	void createSurface() {
		VkSurfaceKHR surf;
		if (glfwCreateWindowSurface(mInstance, mWindow, nullptr, &surf) != VK_SUCCESS) {
			exit(1);
		}
		mSurface = vk::SurfaceKHR(surf);
	}
	void createDevice() {

		//DeviceQueueCreateInfo
		vk::QueueFlags requiredFlags = vk::QueueFlagBits::eGraphics;
		size_t queueFamilyIndex = findQueueFamilies(mPhysDevice, requiredFlags);
		float queuePriorities = 0.0f;
		vk::DeviceQueueCreateInfo deviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), static_cast<uint32_t>(queueFamilyIndex), 1, &queuePriorities);

		//DeviceCreateInfo
		std::vector<const char*> deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		vk::DeviceCreateInfo deviceCreateInfo(vk::DeviceCreateFlags(), 1, &deviceQueueCreateInfo);
		deviceCreateInfo.setEnabledExtensionCount(1);
		deviceCreateInfo.setPpEnabledExtensionNames(deviceExtensions.data());

		mDevice = mPhysDevice.createDevice(deviceCreateInfo);
		mQueue = mDevice.getQueue(queueFamilyIndex, 0);
	}
	size_t findQueueFamilies(vk::PhysicalDevice physicalDevice, vk::QueueFlags requiredFlags) {
		std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();


		for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
			if (physicalDevice.getSurfaceSupportKHR(i, mSurface) && 
				queueFamilyProperties[i].queueFlags & requiredFlags) {
				return i;
			}
		}

		exit(1);
	}
	void pickPhysicalDevice() {
		std::vector<vk::PhysicalDevice> devices = mInstance.enumeratePhysicalDevices();
		

		//lazy device checking, todo: do properly
		for (const auto& dev : devices) {
			std::string requiredExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
			std::vector<vk::ExtensionProperties> availableExtensions = dev.enumerateDeviceExtensionProperties();
			for (const auto& ext : availableExtensions) {
				if (requiredExtension.compare(ext.extensionName) == 0) {
					//important to querry for swap chain support after verifying that the extension is available
					SwapchainSupportDetails details = querrySwapchainSupportDetails(dev, mSurface);
					if (!details.formats.empty() && !details.presentModes.empty()) {
						mPhysDevice = dev;
						return;
					}

				}
			}
		}

		exit(1);
	}
	void createInstance() {

		// Creatig Vulkan Instance with extensions for glfw & validation layer
		vk::ApplicationInfo applicationInfo("Vulkan AppName", 1, "", 1, VK_API_VERSION_1_2);

		//getting required extensions from glfw
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		//creating a list of layers
		std::vector<char const*> instanceLayerNames;
		//TODO: If debug
		instanceLayerNames.push_back("VK_LAYER_KHRONOS_validation");
		std::vector<vk::LayerProperties> instanceLayerProperties = vk::enumerateInstanceLayerProperties();

		if (!checkLayers(instanceLayerNames, instanceLayerProperties)) {
			std::cout << "Set the environment variable VK_LAYER_PATH to point to the location of your layers" << std::endl;
			exit(1);
		}


		vk::InstanceCreateInfo instanceCreateInfo({}, &applicationInfo);
		instanceCreateInfo.enabledExtensionCount = glfwExtensionCount;
		instanceCreateInfo.ppEnabledExtensionNames = glfwExtensions;
		instanceCreateInfo.enabledLayerCount = instanceLayerNames.size();
		instanceCreateInfo.ppEnabledLayerNames = instanceLayerNames.data();

		mInstance = vk::createInstance(instanceCreateInfo);
	}
};