#pragma once

#include "GraphicsVulkan.h"

#include "STBI/stb_image.h"

class VulkanImage {

public:
	VulkanImage(const GraphicsVulkan& gfx, std::string filename);
	VulkanImage(const VulkanImage&) = delete;
	VulkanImage& operator=(const VulkanImage&) = delete;
	~VulkanImage();

	vk::ImageView GetImageView() {
		return mImageView;
	}

private:
	void createImage(vk::Device device, vk::PhysicalDevice physDevice, vk::Format format);
	void loadImageData(std::string filename);
	void fillImageWithData(vk::Device device, vk::PhysicalDevice physDevice, vk::CommandPool cmdPool, vk::Queue queue);
	void createImageView(vk::Device device, vk::Format format) {
		vk::ImageViewCreateInfo createInfo{ {}, mImage, vk::ImageViewType::e2D, format, {},
			vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1} };
		mImageView = device.createImageView(createInfo);

	}
private:
	vk::ImageView mImageView;
	vk::Image mImage;
	vk::DeviceMemory mImageMemory;

	uint32_t mImgWidth;
	uint32_t mImgHeight;
	stbi_uc* mImgData;
	uint32_t mImgByteSize;

};