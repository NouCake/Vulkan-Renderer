#include "GraphicsVulkan.h"

#include <vector>
#include <iostream>

GraphicsVulkan::GraphicsVulkan(GLFWwindow* window) {
	createInstance();
	createSurface(window);
	pickPhysicalDevice();
	createDevice();
	createSwapchain();
	createCommandpool();
	createCommandbuffers();
	createSyncObjects();
}

GraphicsVulkan::~GraphicsVulkan() {
	mDevice.freeCommandBuffers(mCommandPool, mCommandBuffers);
	mDevice.destroyCommandPool(mCommandPool);
	mDevice.destroy();

	mInstance.destroySurfaceKHR(mSurface);
	mInstance.destroy();
}

void GraphicsVulkan::onFrameStart() {
	mDevice.waitForFences(mFlightFence[currentFrame].get(), VK_TRUE, UINT64_MAX);
	mDevice.resetFences(mFlightFence[currentFrame].get());
	currentFbIndex = mDevice.acquireNextImageKHR(mSwapchain, UINT64_MAX, mImageAquiredSemaphores[currentFrame].get(), nullptr).value;
	
	mCommandBuffers[currentFbIndex].begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
}

void GraphicsVulkan::onFrameEnd() {
	mCommandBuffers[currentFbIndex].end();

	commitCommandBuffer(mCommandBuffers[currentFbIndex], mImageAquiredSemaphores[currentFrame].get(), mRenderFinishedSemaphores[currentFrame].get());
	vk::PresentInfoKHR presentInfo{ 1, &mRenderFinishedSemaphores[currentFrame].get(), 1, &mSwapchain, &currentFbIndex };
	mPresentQueue.presentKHR(presentInfo);

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void GraphicsVulkan::commitCommandBuffer(const vk::CommandBuffer& cmdBuffer, const vk::Semaphore& submitWait, const vk::Semaphore& submitFinish) {
	vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	vk::SubmitInfo submitInfo(1, &submitWait, &waitStage, 1, &cmdBuffer, 1, &submitFinish);
	mGfxQueue.submit(submitInfo, mFlightFence[currentFrame].get());
}

void GraphicsVulkan::createInstance() {
	vk::ApplicationInfo appInfo("NouEngine");
	appInfo.apiVersion = VK_API_VERSION_1_2;


	vk::InstanceCreateInfo instanceCreateInfo{ {}, &appInfo, (uint32_t)mInstanceLayers.size(), mInstanceLayers.data(),
		(uint32_t)mInstanceExtensions.size(), mInstanceExtensions.data() };
	mInstance = vk::createInstance(instanceCreateInfo);
}

void GraphicsVulkan::createSurface(GLFWwindow* window) {
	VkSurfaceKHR surface;
	VkResult result = glfwCreateWindowSurface(mInstance, window, nullptr, &surface);
	if (result != VK_SUCCESS) throw std::runtime_error("Could not create Surface with GLFW");
	mSurface = vk::SurfaceKHR(surface);
	glfwGetWindowSize(window, &SURFACE_WIDTH, &SURFACE_HEIGHT);
}

void GraphicsVulkan::pickPhysicalDevice() {
	auto physDevices = mInstance.enumeratePhysicalDevices();
	for (const auto& device : physDevices) {
		if (VulkanUtils::checkPhysicalDevice(device, mSurface, mDeviceExtensions)) {
			mPhysicalDevice = device;
			return;
		}
	}
	throw std::exception("There is no suitable PhysicalDevice");
}

std::vector<vk::DeviceQueueCreateInfo> GraphicsVulkan::createQueueCreateInfos() {
	std::vector<vk::DeviceQueueCreateInfo> createInfos;

	mQueueFamilyIndices = VulkanUtils::findQueueFamilies(mPhysicalDevice, mSurface);;
	std::set<uint32_t> uniqueFamilies = { mQueueFamilyIndices.graphicsFamily.value(), mQueueFamilyIndices.presentFamily.value() };
	for (uint32_t i : uniqueFamilies) {
		float priorities = 1;
		createInfos.push_back(vk::DeviceQueueCreateInfo{ {}, i, 1, &priorities});
	}

	return createInfos;
}

void GraphicsVulkan::createDevice() {
	std::vector<vk::DeviceQueueCreateInfo> queueInfos = createQueueCreateInfos();;
	vk::DeviceCreateInfo deviceInfo{ {}, (uint32_t)queueInfos.size(), queueInfos.data(), 
		(uint32_t) mDeviceLayers.size(), mDeviceLayers.data(),
		(uint32_t) mDeviceExtensions.size(), mDeviceExtensions.data(),
		nullptr };
	mDevice = mPhysicalDevice.createDevice(deviceInfo);
	mGfxQueue = mDevice.getQueue(mQueueFamilyIndices.graphicsFamily.value(), 0);
	mPresentQueue = mDevice.getQueue(mQueueFamilyIndices.presentFamily.value(), 0);
}

void GraphicsVulkan::createSwapchain() {
	vk::SurfaceCapabilitiesKHR capabilities = mPhysicalDevice.getSurfaceCapabilitiesKHR(mSurface);

	uint32_t minImageCount = capabilities.minImageCount;
	if (minImageCount + 1 <= capabilities.maxImageCount) minImageCount++;
	vk::SurfaceFormatKHR surfaceFormat = VulkanUtils::chooseFormat(mPhysicalDevice.getSurfaceFormatsKHR(mSurface), vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear);
	vk::Extent2D imageExtent = VulkanUtils::chooseExtent({ (uint32_t)SURFACE_WIDTH, (uint32_t)SURFACE_HEIGHT },
		capabilities.currentExtent, capabilities.minImageExtent, capabilities.maxImageExtent);
	mSwapchainFormat = surfaceFormat.format;

	std::cout << "Creating Swapchain with format " << (uint32_t)surfaceFormat.format << " and colorspace " << (uint32_t)surfaceFormat.colorSpace << std::endl;
	std::cout << "Creating Surface with size " << imageExtent.width << " | " << imageExtent.height << std::endl;

	vk::SwapchainCreateInfoKHR createInfo{ {}, mSurface, minImageCount, mSwapchainFormat, surfaceFormat.colorSpace, imageExtent,
		1, vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive };
	createInfo.preTransform = capabilities.currentTransform;
	createInfo.presentMode = VulkanUtils::choosePresentMode(mPhysicalDevice.getSurfacePresentModesKHR(mSurface), vk::PresentModeKHR::eMailbox);
	createInfo.clipped = true;
	
	mSwapchain = mDevice.createSwapchainKHR(createInfo);
	mSwapchainImages = mDevice.getSwapchainImagesKHR(mSwapchain);
	SWAPCHAIN_SIZE = mSwapchainImages.size();

	vk::ComponentMapping compMap{ vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA };
	vk::ImageSubresourceRange subRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
	for (size_t i = 0; i < SWAPCHAIN_SIZE; i++) {
		vk::ImageViewCreateInfo imageViewCreateInfo{ vk::ImageViewCreateFlags(), mSwapchainImages[i], vk::ImageViewType::e2D, mSwapchainFormat, compMap, subRange };
		mSwapchainImageViews.push_back(mDevice.createImageView(imageViewCreateInfo));
	}

}

void GraphicsVulkan::createCommandpool() {
	vk::CommandPoolCreateInfo poolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, mQueueFamilyIndices.graphicsFamily.value());
	mCommandPool = mDevice.createCommandPool(poolCreateInfo);
}

void GraphicsVulkan::createCommandbuffers() {
	vk::CommandBufferAllocateInfo allocateInfo(mCommandPool, vk::CommandBufferLevel::ePrimary, SWAPCHAIN_SIZE);
	mCommandBuffers = mDevice.allocateCommandBuffers(allocateInfo);
}

void GraphicsVulkan::createSyncObjects() {
	mImageAquiredSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	mRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	mFlightFence.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		mImageAquiredSemaphores[i] = mDevice.createSemaphoreUnique(vk::SemaphoreCreateInfo());
		mRenderFinishedSemaphores[i] = mDevice.createSemaphoreUnique(vk::SemaphoreCreateInfo());
		mFlightFence[i] = mDevice.createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
	}
}