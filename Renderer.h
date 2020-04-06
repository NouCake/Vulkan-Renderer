#pragma once

#include "GraphicsVulkan.h"

//VulkanRenderer
class Renderer {

public:
	Renderer(const GraphicsVulkan& gfx) {
		createPipeline(gfx.mDevice, gfx.mSwapchainFormat);
		createBuffers(gfx.mDevice, gfx.mCommandPool, gfx.SURFACE_WIDTH, gfx.SURFACE_HEIGHT, gfx.SWAPCHAIN_SIZE, gfx.mSwapchainImageViews);

		createBeginInfo(gfx.SURFACE_WIDTH, gfx.SURFACE_HEIGHT);
	}

	/* Buffer has to be started recording */
	void drawScene(const GraphicsVulkan& gfx);

	vk::RenderPass GetRenderPass() const {
		return mRenderpass;
	}
	
private:

	//Init
	void createPipeline(vk::Device device, vk::Format swapchainFormat) {
		vk::AttachmentDescription colorAttachment{ {}, swapchainFormat, vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR };
		vk::AttachmentReference colorReference{ 0, vk::ImageLayout::eColorAttachmentOptimal };

		vk::SubpassDescription subpass{ {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorReference };
		vk::SubpassDependency dependency{ VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite };

		vk::RenderPassCreateInfo createInfo{ {}, 1, &colorAttachment, 1, &subpass, 1, &dependency };
		mRenderpass = device.createRenderPass(createInfo);
	}
	void createBeginInfo(uint32_t width, uint32_t height) {
		mClearValues = { std::array<float, 4>({ 0.0f, 0.25f, 0.8f, 1.0f }) };
		vk::Rect2D extent = vk::Rect2D({ (uint32_t)0, (uint32_t)0 }, { width, height });
		mBeginInfo = { mRenderpass, {}, extent, 1, &mClearValues };
	}
	void createBuffers(vk::Device device, vk::CommandPool pool, uint32_t surfaceWidth, uint32_t surfaceHeight, uint32_t swapchainSize, std::vector<vk::ImageView> swapchainImageViews) {
		mCommandBuffers.resize(swapchainSize);
		vk::CommandBufferAllocateInfo allocateInfo(pool, vk::CommandBufferLevel::ePrimary, swapchainSize);
		mCommandBuffers = device.allocateCommandBuffers(allocateInfo);


		mFramebuffers.resize(swapchainSize);
		for (size_t i = 0; i < swapchainSize; i++) {
			vk::FramebufferCreateInfo bufferCreateInfo{ {}, mRenderpass, 1, &swapchainImageViews[i], surfaceWidth, surfaceHeight, 1 };
			mFramebuffers[i] = device.createFramebuffer(bufferCreateInfo);
		}
	}

private:
	vk::RenderPass mRenderpass;
	std::vector<vk::CommandBuffer> mCommandBuffers;
	std::vector<vk::Framebuffer> mFramebuffers;
	//needz
	vk::ClearValue mClearValues;
	vk::RenderPassBeginInfo mBeginInfo;

	//runtime
	vk::CommandBuffer currentCmdBuffer;
};