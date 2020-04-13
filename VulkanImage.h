#pragma once

#include "GraphicsVulkan.h"

#define STB_IMAGE_IMPLEMENTATION
#include "STBI/stb_image.h"

class VulkanImage {

public:
	VulkanImage(const GraphicsVulkan& gfx, std::string filename) {
		loadImageData(filename);
		createImage(gfx.mDevice);
		fillImageWithData(gfx.mDevice, gfx.mPhysicalDevice);
	};
	VulkanImage(const VulkanImage&) = delete;
	~VulkanImage();
private:
	void createImage(vk::Device device, vk::PhysicalDevice physDevice, vk::Format format) {

		vk::ImageCreateInfo imageCreateInfo{ {}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Srgb, 
			{mImgWidth, mImgHeight, 1 }, 1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eLinear, 
			vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::SharingMode::eExclusive, 
			0, nullptr, vk::ImageLayout::eUndefined };
		mImage  = device.createImage(imageCreateInfo);

		VulkanUtils::createImage(device, physDevice, format, mImgWidth, mImgHeight, 
			vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, mImage, mImageMemory);
	}

	void loadImageData(std::string filename) {
		int imgWidth;
		int imgHeight;
		int imgChannels;

		mImgData = stbi_load(filename.c_str(), &imgWidth, &imgHeight, &imgChannels, STBI_rgb_alpha);
		mImgByteSize = imgWidth * imgHeight * 4;

		mImgWidth = imgWidth;
		mImgHeight = imgHeight;
	}

	void fillImageWithData(vk::Device device, vk::PhysicalDevice physDevice) {
		vk::Buffer tmpBuffer;
		vk::DeviceMemory tmpMemory;
		VulkanUtils::createBuffer(device, physDevice, mImgByteSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, tmpBuffer, tmpMemory);

		void* data = device.mapMemory(tmpMemory, 0, mImgByteSize);
		memcpy(data, mImgData, mImgByteSize);
		device.unmapMemory(tmpMemory);
		free(mImgData);
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