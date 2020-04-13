#pragma once

#include <optional>
#include <set>

#include "vulkan/vulkan.hpp"

namespace VulkanUtils {

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice& physDevice, const vk::SurfaceKHR& surface);
	bool checkPhysicalDevice(vk::PhysicalDevice physDevice, vk::SurfaceKHR surface, std::vector<const char*> extensions);
	vk::SurfaceFormatKHR chooseFormat(std::vector<vk::SurfaceFormatKHR> avaibleFormats, vk::Format targetFormat, vk::ColorSpaceKHR targetSpace);
	vk::Extent2D chooseExtent(vk::Extent2D target, vk::Extent2D currentExtent, vk::Extent2D minExtent, vk::Extent2D maxExtent);
	vk::PresentModeKHR choosePresentMode(std::vector<vk::PresentModeKHR> avaiblePresentModes, vk::PresentModeKHR target);
	void createBuffer(const vk::Device& device, const vk::PhysicalDevice& physDevice, uint64_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryProperties, vk::Buffer& buffer, vk::DeviceMemory& memory);
	uint32_t findMemoryType(const vk::PhysicalDevice& physDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties);
	void copyBuffer(const vk::Device& device, const vk::CommandPool& cmdPool, const vk::Queue& queue, vk::Buffer srcBuffer, vk::Buffer dstBuffer, uint32_t size);
	//void copyBuffer(const vk::Device& device, const vk::CommandBuffer& buffer, vk::Buffer srcBuffer, vk::Buffer dstBuffer, uint32_t size, uint32_t offset);
	void createImage(vk::Device device, vk::PhysicalDevice physDevice, vk::Format format, uint32_t width, uint32_t height, vk::ImageUsageFlags usage, vk::Image& outImage, vk::DeviceMemory& outMemory);
	vk::CommandBuffer startSingleUserCmdBuffer(vk::Device device, vk::CommandPool cmdPool);
	void endSingleUseCmdBuffer(vk::CommandBuffer tmpBuffer, vk::Queue queue);
	void transitionImageLayout(vk::Device device, vk::CommandPool cmdPool, vk::Queue queue, vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
}