#pragma once

#include "GraphicsVulkan.h"

//VulkanRenderer
class Renderer {
	friend class Material;

public:
	Renderer(const GraphicsVulkan& gfx) {
		mGfx = &gfx;
		createDepthBuffer(gfx.mDevice, gfx.mPhysicalDevice, gfx.SURFACE_WIDTH, gfx.SURFACE_HEIGHT);
		createRenderPass(gfx.mDevice, gfx.mSwapchainFormat);
		createBuffers(gfx.mDevice, gfx.SURFACE_WIDTH, gfx.SURFACE_HEIGHT, gfx.SWAPCHAIN_SIZE, gfx.mSwapchainImageViews);
		createDescriptorPool(gfx.mDevice);

		createBeginInfo(gfx.SURFACE_WIDTH, gfx.SURFACE_HEIGHT);
	}
	~Renderer() {
		mGfx->mDevice.waitIdle();
		mGfx->mDevice.destroyDescriptorPool(mDescriptorPool);
		for (auto fb : mFramebuffers) mGfx->mDevice.destroyFramebuffer(fb);
		mGfx->mDevice.destroyRenderPass(mRenderpass);
	}
	Renderer(const Renderer&) = delete;
	Renderer& operator= (const Renderer&) = delete;

	/* Buffer has to be started recording */
	void drawScene(const GraphicsVulkan& gfx);

	vk::RenderPass GetRenderPass() const {
		return mRenderpass;
	}
	
private:

	//Init
	void createRenderPass(vk::Device device, vk::Format swapchainFormat) {
		vk::AttachmentDescription colorAttachment{ {}, swapchainFormat, vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR };
		vk::AttachmentReference colorReference{ 0, vk::ImageLayout::eColorAttachmentOptimal };

		vk::AttachmentDescription depthAttachment{ {}, mDepthFormat, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare, 
			vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal };
		vk::AttachmentReference depthReference{ 1, vk::ImageLayout::eDepthStencilAttachmentOptimal };

		vk::SubpassDescription subpass{ {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorReference, nullptr, &depthReference };
		vk::SubpassDependency dependency{ VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite };

		std::vector<vk::AttachmentDescription> attachments = {colorAttachment, depthAttachment};
		vk::RenderPassCreateInfo createInfo{ {}, (uint32_t)attachments.size(), attachments.data(), 1, &subpass, 1, &dependency };
		mRenderpass = device.createRenderPass(createInfo);
	}
	void createBeginInfo(uint32_t width, uint32_t height) {
		mClearValues.resize(2);
		mClearValues[0].color = vk::ClearColorValue{ std::array<float, 4>{0.0f, 0.25f, 0.8f, 1.0f} };
		mClearValues[1].depthStencil = vk::ClearDepthStencilValue{ 1.0f, 0 };
		vk::Rect2D extent = vk::Rect2D({ (uint32_t)0, (uint32_t)0 }, { width, height });
		mBeginInfo = vk::RenderPassBeginInfo{ mRenderpass, {}, extent, (uint32_t)mClearValues.size(), mClearValues.data() };
	}
	void createBuffers(vk::Device device, uint32_t surfaceWidth, uint32_t surfaceHeight, uint32_t swapchainSize, std::vector<vk::ImageView> swapchainImageViews) {
		mFramebuffers.resize(swapchainSize);
		for (size_t i = 0; i < swapchainSize; i++) {
			std::vector<vk::ImageView> attachments = { swapchainImageViews[i], mDepthImageView };
			vk::FramebufferCreateInfo bufferCreateInfo{ {}, mRenderpass, (uint32_t)attachments.size(), attachments.data(), surfaceWidth, surfaceHeight, 1 };
			mFramebuffers[i] = device.createFramebuffer(bufferCreateInfo);
		}
	}
	void createDescriptorPool(vk::Device device) {
		//Gather Data about all Materials that the renderer can use
		vk::DescriptorPoolSize poolSizeUniforms{ vk::DescriptorType::eUniformBuffer, 1 };
		vk::DescriptorPoolSize poolSizeSampler{ vk::DescriptorType::eCombinedImageSampler, 1 };
		std::vector<vk::DescriptorPoolSize> pools({ poolSizeUniforms, poolSizeSampler });
		vk::DescriptorPoolCreateInfo poolCreateInfo{ {}, 1, 2, pools.data()};
		mDescriptorPool = device.createDescriptorPool(poolCreateInfo);
	}
	void createDepthBuffer(vk::Device device, vk::PhysicalDevice physDevice, uint32_t surfaceWidth, uint32_t surfaceHeight) {
		mDepthFormat = VulkanUtils::findSupportedFormat(physDevice, { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint }, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
		VulkanUtils::createImage(device, physDevice, mDepthFormat, surfaceWidth, surfaceHeight, vk::ImageUsageFlagBits::eDepthStencilAttachment, mDepthImage, mDepthImageMemory);
		mDepthImageView = VulkanUtils::createImageView(device, mDepthImage, mDepthFormat, vk::ImageAspectFlagBits::eDepth);
	}

private:
	vk::RenderPass mRenderpass;
	std::vector<vk::Framebuffer> mFramebuffers;

	vk::Format mDepthFormat;
	vk::Image mDepthImage;
	vk::DeviceMemory mDepthImageMemory;
	vk::ImageView mDepthImageView;

	vk::DescriptorPool mDescriptorPool;
	
	const GraphicsVulkan* mGfx;
	//needz
	std::vector<vk::ClearValue> mClearValues;
	vk::RenderPassBeginInfo mBeginInfo;
};