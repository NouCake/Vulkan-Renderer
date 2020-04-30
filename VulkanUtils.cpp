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

	vk::CommandBuffer startSingleUserCmdBuffer(vk::Device device, vk::CommandPool cmdPool) {
		vk::CommandBufferAllocateInfo allocateInfo{ cmdPool, vk::CommandBufferLevel::ePrimary, 1 };
		vk::CommandBuffer tmpBuffer = device.allocateCommandBuffers(allocateInfo)[0];

		tmpBuffer.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
		return tmpBuffer;
	}

	void endSingleUseCmdBuffer(vk::CommandBuffer tmpBuffer, vk::Queue queue) {
		tmpBuffer.end();

		vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &tmpBuffer, 0, nullptr);
		queue.submit(submitInfo, nullptr);
		queue.waitIdle();
	}

	void transitionImageLayout(vk::CommandBuffer cmdBuffer, vk::Device device, vk::CommandPool cmdPool, vk::Queue queue, vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {
		vk::AccessFlags srcAccess;
		vk::AccessFlags dstAccess;
		vk::PipelineStageFlags srcStage;
		vk::PipelineStageFlags dstStage;

		if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
			srcAccess = {};
			dstAccess = vk::AccessFlagBits::eTransferWrite;
			srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
			dstStage = vk::PipelineStageFlagBits::eTransfer;
		} else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
			srcAccess = vk::AccessFlagBits::eTransferWrite;
			dstAccess = vk::AccessFlagBits::eShaderRead;
			srcStage = vk::PipelineStageFlagBits::eTransfer;
			dstStage = vk::PipelineStageFlagBits::eFragmentShader;
		}

		vk::ImageSubresourceRange range{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
		vk::ImageMemoryBarrier barrier{ srcAccess, dstAccess, oldLayout, newLayout, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, image, range };
		cmdBuffer.pipelineBarrier(srcStage, dstStage, {}, {}, {}, barrier);
	}

	void copyBufferToImage(vk::CommandBuffer cmdBuffer, vk::Device device, vk::CommandPool cmdPool, vk::Queue queue, vk::Buffer src, vk::Image dst, uint32_t width, uint32_t height) {

		vk::BufferImageCopy copy;
		copy.bufferOffset = 0;
		copy.bufferRowLength = 0;
		copy.bufferImageHeight = 0;
		copy.imageSubresource = vk::ImageSubresourceLayers{ vk::ImageAspectFlagBits::eColor, 0, 0, 1 };
		copy.imageOffset = vk::Offset3D{ 0, 0, 0 };
		copy.imageExtent = vk::Extent3D{ width, height, 1 };
		cmdBuffer.copyBufferToImage(src, dst, vk::ImageLayout::eTransferDstOptimal, copy);

	}

	vk::Format findSupportedFormat(vk::PhysicalDevice physDevice, const std::vector<vk::Format> condidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
		
		for (const auto& format : condidates) {
			vk::FormatProperties props = physDevice.getFormatProperties(format);

			if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
				return format;
			}

			throw std::runtime_error("failed to find supported format");
		}

	}

	bool hasStencilComponent(vk::Format format) {
		return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
	}

	vk::ImageView createImageView(vk::Device device, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlag) {
		vk::ImageViewCreateInfo createInfo{ {}, image, vk::ImageViewType::e2D, format, {},
			vk::ImageSubresourceRange{ aspectFlag, 0, 1, 0, 1} };
		return device.createImageView(createInfo);
	}

}