#pragma once

#include "Imgui/imgui.h"
#include "Imgui/imgui_impl_glfw.h"
#include "Imgui/imgui_impl_vulkan.h"

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
		initImgui(gfx.mInstance, gfx.mPhysicalDevice, gfx.mDevice, gfx.mQueueFamilyIndices.graphicsFamily.value(), 
			gfx.mGfxQueue, gfx.SWAPCHAIN_SIZE, gfx.mSwapchainFormat, gfx.mCommandPool, gfx.SURFACE_WIDTH, gfx.SURFACE_HEIGHT, gfx.mSwapchainImageViews);

		createBeginInfo(gfx.SURFACE_WIDTH, gfx.SURFACE_HEIGHT);
	}
	~Renderer() {
		mGfx->mDevice.waitIdle();
		mGfx->mDevice.destroyDescriptorPool(mDescriptorPool);
		mGfx->mDevice.destroyDescriptorPool(mImguiDescriptorPool);
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
			vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal }; //not the last renderpass anymore, so finalLayout does not have to be presentKHR
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
		vk::DescriptorPoolCreateInfo poolCreateInfo{ {}, 1, (uint32_t)pools.size(), pools.data()};
		mDescriptorPool = device.createDescriptorPool(poolCreateInfo);
	}
	void createDepthBuffer(vk::Device device, vk::PhysicalDevice physDevice, uint32_t surfaceWidth, uint32_t surfaceHeight) {
		mDepthFormat = VulkanUtils::findSupportedFormat(physDevice, { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint }, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
		VulkanUtils::createImage(device, physDevice, mDepthFormat, surfaceWidth, surfaceHeight, vk::ImageUsageFlagBits::eDepthStencilAttachment, mDepthImage, mDepthImageMemory);
		mDepthImageView = VulkanUtils::createImageView(device, mDepthImage, mDepthFormat, vk::ImageAspectFlagBits::eDepth);
	}

	//Dear ImGui
	void initImgui(vk::Instance instance, vk::PhysicalDevice physDevice, vk::Device device, uint32_t queueFamily, vk::Queue queue, uint32_t swapchainSize,
		vk::Format swapchainFormat, vk::CommandPool cmdPool, uint32_t surfaceWidth, uint32_t surfaceHeight, std::vector<vk::ImageView> swapchainImageViews) {
		// DescriptorPool //
		//What the hell is this overkill pool?!
		std::vector<vk::DescriptorPoolSize> poolSizes = {
			vk::DescriptorPoolSize{ vk::DescriptorType::eSampler, 1000 },
			vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler, 1000 },
			vk::DescriptorPoolSize{ vk::DescriptorType::eSampledImage, 1000 },
			vk::DescriptorPoolSize{ vk::DescriptorType::eStorageImage, 1000 },
			vk::DescriptorPoolSize{ vk::DescriptorType::eUniformTexelBuffer, 1000 },
			vk::DescriptorPoolSize{ vk::DescriptorType::eStorageTexelBuffer, 1000 },
			vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBuffer, 1000 },
			vk::DescriptorPoolSize{ vk::DescriptorType::eStorageBuffer, 1000 },
			vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBufferDynamic, 1000 },
			vk::DescriptorPoolSize{ vk::DescriptorType::eStorageBufferDynamic, 1000 },
			vk::DescriptorPoolSize{ vk::DescriptorType::eInputAttachment, 1000 }
		};
		vk::DescriptorPoolCreateInfo poolCreateInfo{ {}, 1000 * (uint32_t)poolSizes.size(), (uint32_t)poolSizes.size(), poolSizes.data() };
		mImguiDescriptorPool = device.createDescriptorPool(poolCreateInfo);

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = instance;
		init_info.PhysicalDevice = physDevice;
		init_info.Device = device;
		init_info.QueueFamily = queueFamily;
		init_info.Queue = queue;
		init_info.PipelineCache = nullptr;
		init_info.DescriptorPool = mImguiDescriptorPool;
		init_info.Allocator = nullptr;
		init_info.MinImageCount = swapchainSize; //apparently not used, but checked
		init_info.ImageCount = swapchainSize;
		init_info.CheckVkResultFn = [](VkResult err) {
			//Not checking any erros, deal with it
		};
		

		// RenderPass //
		vk::AttachmentDescription colorAttachment{ {}, swapchainFormat, vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR }; //not the last renderpass anymore, so finalLayout does not have to be presentKHR
		vk::AttachmentReference colorReference{ 0, vk::ImageLayout::eColorAttachmentOptimal };

		vk::SubpassDescription subpass{ {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorReference, nullptr, nullptr };
		vk::SubpassDependency dependency{ VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlags{}, vk::AccessFlagBits::eColorAttachmentWrite };

		vk::RenderPassCreateInfo renderpassCreateInfo{ {}, 1, &colorAttachment, 1, &subpass, 1, &dependency };
		mImguiRenderpass = device.createRenderPass(renderpassCreateInfo);

		ImGui_ImplVulkan_Init(&init_info, mImguiRenderpass);

		vk::CommandBuffer tmpCmdBuffer = VulkanUtils::startSingleUserCmdBuffer(device, cmdPool);
		ImGui_ImplVulkan_CreateFontsTexture(tmpCmdBuffer);
		VulkanUtils::endSingleUseCmdBuffer(tmpCmdBuffer, queue);


		mImguiFramebuffers.resize(swapchainSize);
		for (size_t i = 0; i < swapchainSize; i++) {
			vk::FramebufferCreateInfo bufferCreateInfo{ {}, mImguiRenderpass, 1, &swapchainImageViews[i], surfaceWidth, surfaceHeight, 1};
			mImguiFramebuffers[i] = device.createFramebuffer(bufferCreateInfo);
		}

	}

private:
	vk::RenderPass mRenderpass;
	std::vector<vk::Framebuffer> mFramebuffers;

	vk::Format mDepthFormat;
	vk::Image mDepthImage;
	vk::DeviceMemory mDepthImageMemory;
	vk::ImageView mDepthImageView;

	vk::DescriptorPool mDescriptorPool;

	vk::RenderPass mImguiRenderpass;
	vk::DescriptorPool mImguiDescriptorPool;
	std::vector<vk::Framebuffer> mImguiFramebuffers;

	const GraphicsVulkan* mGfx;
	//needz
	std::vector<vk::ClearValue> mClearValues;
	vk::RenderPassBeginInfo mBeginInfo;
};