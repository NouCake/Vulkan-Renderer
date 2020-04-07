#include "VulkanUtils.h"

#include <algorithm>

namespace VulkanUtils {
	QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice& physDevice, const vk::SurfaceKHR& surface) {
		QueueFamilyIndices indices;

		auto properties = physDevice.getQueueFamilyProperties();
		for (int i = 0; i < properties.size(); ++i) {
			if (properties[i].queueFlags | vk::QueueFlagBits::eGraphics) {
				indices.graphicsFamily = i;
			}

			if (physDevice.getSurfaceSupportKHR(i, surface)) {
				indices.presentFamily = i;
			}

			if (indices.isComplete()) {
				return indices;
			}
		}

		return indices;
	}

	bool checkPhysicalDevice(vk::PhysicalDevice physDevice, vk::SurfaceKHR surface, std::vector<const char*> extensions) {
		QueueFamilyIndices indices = findQueueFamilies(physDevice, surface);
		bool supportsQueueFamilies = indices.isComplete();

		std::vector<vk::ExtensionProperties> avaibleExtensions = physDevice.enumerateDeviceExtensionProperties();
		std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());
		for (const auto& e : avaibleExtensions) {
			requiredExtensions.erase(e.extensionName);
		}
		bool supportsExtensions = requiredExtensions.empty();

		bool supportsSurface = !physDevice.getSurfaceFormatsKHR(surface).empty() &&
			!physDevice.getSurfacePresentModesKHR(surface).empty();


		return supportsQueueFamilies & supportsSurface && supportsExtensions;
	}

	vk::SurfaceFormatKHR chooseFormat(std::vector<vk::SurfaceFormatKHR> avaibleFormats, vk::Format targetFormat, vk::ColorSpaceKHR targetSpace) {
		for (const auto& format : avaibleFormats) {
			if (format.format == targetFormat && format.colorSpace == targetSpace) {
				return targetFormat;
			}
		}
		return avaibleFormats[0];
	}

	vk::Extent2D chooseExtent(vk::Extent2D target, vk::Extent2D currentExtent, vk::Extent2D minExtent, vk::Extent2D maxExtent) {
		if (currentExtent.width != UINT32_MAX) {
			return currentExtent;
		}
		vk::Extent2D extent(target.width, target.height);
		extent.width = std::clamp(extent.width, minExtent.width, maxExtent.width);
		extent.height = std::clamp(extent.height, minExtent.height, maxExtent.height);
		return extent;
	}

	vk::PresentModeKHR choosePresentMode(std::vector<vk::PresentModeKHR> avaiblePresentModes, vk::PresentModeKHR target) {
		for (const auto& mode : avaiblePresentModes) {
			if (mode == target) return target;
		}
		return avaiblePresentModes[0];
	}

	void createBuffer(const vk::Device& device, const vk::PhysicalDevice& physDevice, uint64_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryProperties,
		vk::Buffer& buffer, vk::DeviceMemory& memory) {

		vk::BufferCreateInfo bufferCreateInfo{ {}, size, usage, vk::SharingMode::eExclusive };
		buffer = device.createBuffer(bufferCreateInfo);

		vk::MemoryRequirements requirements = device.getBufferMemoryRequirements(buffer);
		vk::MemoryAllocateInfo allocateInfo{ requirements.size, findMemoryType(physDevice, requirements.memoryTypeBits, memoryProperties) };
		memory = device.allocateMemory(allocateInfo);
		device.bindBufferMemory(buffer, memory, 0);
	}

	uint32_t findMemoryType(const vk::PhysicalDevice& physDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
		vk::PhysicalDeviceMemoryProperties memProps = physDevice.getMemoryProperties();

		for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
			if (typeFilter & (1 << i) && (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		return 0;
	}


	void copyBuffer(const vk::Device& device, const vk::CommandPool& cmdPool, const vk::Queue& queue, vk::Buffer srcBuffer, vk::Buffer dstBuffer, uint32_t size) {
		vk::CommandBufferAllocateInfo allocateInfo{ cmdPool, vk::CommandBufferLevel::ePrimary, 1 };
		vk::CommandBuffer tmpBuffer = device.allocateCommandBuffers(allocateInfo)[0];

		vk::BufferCopy region{ 0, 0, size };

		tmpBuffer.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
		tmpBuffer.copyBuffer(srcBuffer, dstBuffer, region);
		tmpBuffer.end();

		vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &tmpBuffer, 0, nullptr);
		queue.submit(submitInfo, nullptr);
		queue.waitIdle();
	}

	/*
	void copyBuffer(const vk::Device& device, const vk::CommandBuffer& buffer, vk::Buffer srcBuffer, vk::Buffer dstBuffer, uint32_t size, uint32_t srcOffset, uint32_t dstOffset) {
		vk::BufferCopy region{ srcOffset, dstOffset, size };
		buffer.copyBuffer(srcBuffer, dstBuffer, region);
	}
	*/
	
	void createImage(vk::Device device, vk::PhysicalDevice physDevice, vk::Format format, uint32_t width, uint32_t height, vk::ImageUsageFlags usage, vk::Image& outImage, vk::DeviceMemory& outMemory) {
		vk::ImageCreateInfo info{ {}, vk::ImageType::e2D, format, {width, height, 1}, 1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, usage, vk::SharingMode::eExclusive, 0, nullptr, vk::ImageLayout::eUndefined };
		outImage = device.createImage(info);
		vk::MemoryRequirements req = device.getImageMemoryRequirements(outImage);
		uint32_t imageSize = req.size;
		vk::MemoryAllocateInfo allocInfo{ imageSize, findMemoryType(physDevice, req.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal) };
		outMemory = device.allocateMemory(allocInfo);
		device.bindImageMemory(outImage, outMemory, 0);
	}

}