#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>
#include <iostream>
#include <optional>

class HelloTriangleApplication {
public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}


private:
	const int WINDOW_WIDTH = 640;
	const int WINDOW_HEIGHT = 480;

	GLFWwindow* mWindow;

	vk::Instance mInstance;
	vk::PhysicalDevice mPhysDevice;
	vk::Device mDevice;
	vk::Queue mQueue;
	vk::SurfaceKHR mSurface;
	vk::SwapchainKHR mSwapchain;
	
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

	void initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		mWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr);

	}

	//Ripped from: https://github.com/KhronosGroup/Vulkan-Hpp/blob/master/samples/EnableValidationWithCallback/EnableValidationWithCallback.cpp
	bool checkLayers(std::vector<char const*> const& layers, std::vector<vk::LayerProperties> const& properties) {
		// return true if all layers are listed in the properties
		return std::all_of(layers.begin(), layers.end(), [&properties](char const* name) {
			return std::find_if(properties.begin(), properties.end(), [&name](vk::LayerProperties const& property) { return strcmp(property.layerName, name) == 0; }) != properties.end();
		});
	}

	void initVulkan() {
		createInstance();
		createSurface();
		pickPhysicalDevice();
		createDevice();
		createSwapChain();
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
	void mainLoop() {
		while (!glfwWindowShouldClose(mWindow)) {
			glfwPollEvents();
		}
	}

	void cleanup() {
		glfwDestroyWindow(mWindow);
		glfwTerminate();
	}
};