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