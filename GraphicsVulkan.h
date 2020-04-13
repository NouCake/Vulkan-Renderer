#pragma once
#include "Graphics.h"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "vulkan/vulkan.hpp"

#include "VulkanUtils.h"

class GraphicsVulkan : public Graphics {
	friend class Renderer;
	friend class Material;
	friend class Mesh;
	friend class VulkanImage;

public:
	GraphicsVulkan(GLFWwindow*);
	GraphicsVulkan(const GraphicsVulkan&) = delete;
	GraphicsVulkan& operator=(const GraphicsVulkan&) = delete;
	~GraphicsVulkan();

	void onFrameStart() override;
	void onFrameEnd() override;

	vk::CommandBuffer GetCurrentCommandbuffer() const {
		return mCommandBuffers[currentFbIndex];
	};

	uint32_t GetCurrentSwapchainImageIndex() const {
		return currentFbIndex;
	};

private:
	//Vulkan
	vk::Instance mInstance; //needs cleanup
	vk::SurfaceKHR mSurface; //needs cleanup
	VulkanUtils::QueueFamilyIndices mQueueFamilyIndices;
	vk::PhysicalDevice mPhysicalDevice;
	vk::Device mDevice; //needs cleanup
	vk::Queue mGfxQueue;
	vk::Queue mPresentQueue;

	//Swapchain
	vk::SwapchainKHR mSwapchain; //needs cleanup
	vk::Format mSwapchainFormat;
	//vk::Extent2D mSwapchainExtent;
	std::vector<vk::Image> mSwapchainImages;
	std::vector<vk::ImageView> mSwapchainImageViews; //needs cleanup

	//Commands
	vk::CommandPool mCommandPool; //needs cleanup
	std::vector<vk::CommandBuffer> mCommandBuffers; //needs cleanup

	//SyncObjects
	std::vector<vk::UniqueSemaphore> mImageAquiredSemaphores;
	std::vector<vk::UniqueSemaphore> mRenderFinishedSemaphores;
	std::vector<vk::UniqueFence> mFlightFence;

	//Extensions & Layers
	std::vector<const char*> mInstanceLayers = { "VK_LAYER_LUNARG_standard_validation" };
	std::vector<const char*> mInstanceExtensions = { "VK_KHR_surface", "VK_KHR_win32_surface" };
	std::vector<const char*> mDeviceLayers;
	std::vector<const char*> mDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	//runtime variables
	uint32_t currentFrame = 0;
	uint32_t currentFbIndex = 0;

	//consts
	int SURFACE_WIDTH;
	int SURFACE_HEIGHT;
	int SWAPCHAIN_SIZE;
	const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
private:
	void commitCommandBuffer(const vk::CommandBuffer& cmdBuffer, const vk::Semaphore& submitWait, const vk::Semaphore& submitFinish);

	//Init functions
	void createInstance();
	void createSurface(GLFWwindow*);
	void pickPhysicalDevice();
	std::vector<vk::DeviceQueueCreateInfo> createQueueCreateInfos();
	void createDevice();
	void createSwapchain();
	void createCommandpool();
	void createCommandbuffers();
	void createSyncObjects();
	//void createFramebuffers();
};