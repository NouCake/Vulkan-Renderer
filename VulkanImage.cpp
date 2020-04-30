#include "VulkanImage.h"

#define STB_IMAGE_IMPLEMENTATION
#include "STBI/stb_image.h"

VulkanImage::VulkanImage(const GraphicsVulkan& gfx, std::string filename) {
	loadImageData(filename);
	createImage(gfx.mDevice, gfx.mPhysicalDevice, vk::Format::eR8G8B8A8Unorm);
	fillImageWithData(gfx.mDevice, gfx.mPhysicalDevice, gfx.mCommandPool, gfx.mGfxQueue);
	createImageView(gfx.mDevice, vk::Format::eR8G8B8A8Unorm);
}

VulkanImage::~VulkanImage() {
	//destroy Image View
	//destroy Image
	//free mImageMemory
}

void VulkanImage::createImage(vk::Device device, vk::PhysicalDevice physDevice, vk::Format format) {

	VulkanUtils::createImage(device, physDevice, format, mImgWidth, mImgHeight,
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, mImage, mImageMemory);
}

void VulkanImage::loadImageData(std::string filename) {
	int imgWidth;
	int imgHeight;
	int imgChannels;

	mImgData = stbi_load(filename.c_str(), &imgWidth, &imgHeight, &imgChannels, STBI_rgb_alpha);
	mImgByteSize = imgWidth * imgHeight * 4;

	mImgWidth = imgWidth;
	mImgHeight = imgHeight;
}

void VulkanImage::fillImageWithData(vk::Device device, vk::PhysicalDevice physDevice, vk::CommandPool cmdPool, vk::Queue queue) {
	vk::Buffer tmpBuffer;
	vk::DeviceMemory tmpMemory;
	VulkanUtils::createBuffer(device, physDevice, mImgByteSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, tmpBuffer, tmpMemory);

	void* data = device.mapMemory(tmpMemory, 0, mImgByteSize);
	memcpy(data, mImgData, mImgByteSize);
	device.unmapMemory(tmpMemory);
	free(mImgData);

	vk::CommandBuffer cmdBuffer = VulkanUtils::startSingleUserCmdBuffer(device, cmdPool);
	VulkanUtils::transitionImageLayout(cmdBuffer, device, cmdPool, queue, mImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
	VulkanUtils::copyBufferToImage(cmdBuffer, device, cmdPool, queue, tmpBuffer, mImage, mImgWidth, mImgHeight);
	VulkanUtils::transitionImageLayout(cmdBuffer, device, cmdPool, queue, mImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
	VulkanUtils::endSingleUseCmdBuffer(cmdBuffer, queue);

	device.destroyBuffer(tmpBuffer);
	device.freeMemory(tmpMemory);
}