#pragma once

#include "Material.h"

#include <chrono>
#include <math.h>

Material::Material(const GraphicsVulkan& gfx, const Renderer& renderer) :
	SURFACE_WIDTH(gfx.SURFACE_WIDTH),
	SURFACE_HEIGHT(gfx.SURFACE_HEIGHT),
	mImage(gfx, "textures/test.png"){
	mGfx = &gfx;
	createStages(gfx.mDevice);
	createDescriptorSetLayout(gfx.mDevice);
	createPipeline(gfx.mDevice, renderer.GetRenderPass(), gfx.SURFACE_WIDTH, gfx.SURFACE_HEIGHT);
	createUniformBuffer(gfx.mDevice, gfx.mPhysicalDevice, gfx.MAX_FRAMES_IN_FLIGHT);
	createSampler(gfx.mDevice);
	createDescriptorSet(gfx.mDevice, renderer.mDescriptorPool);
}

Material::~Material() {
	mGfx->mDevice.destroySampler(mSampler);
	mGfx->mDevice.waitIdle();
	mGfx->mDevice.destroyBuffer(mUniformStagingBuffer);
	mGfx->mDevice.freeMemory(mUniformStagingBufferMemory);

	mGfx->mDevice.destroyDescriptorSetLayout(mDescriptorSetLayout);
	mGfx->mDevice.destroyPipelineLayout(mPipelineLayout);
	mGfx->mDevice.destroyPipeline(mGfxPipeline);
}
void Material::cleanup(const GraphicsVulkan& gfx) {
}

void Material::UpdateUniforms(vk::Device device, vk::CommandBuffer buffer, uint32_t frameIndex) {
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	Uniforms ubo;

	ubo.model = glm::translate(glm::identity<glm::mat4>(), glm::vec3(0.1f * deltaTime, 0.0f, 0.0f));
	ubo.model = glm::identity<glm::mat4>();
	ubo.view = glm::identity<glm::mat4>();
	ubo.proj = glm::identity<glm::mat4>();

	static glm::vec3 camPos(0.0f, 1.0f, 3.0f);
	static glm::vec4 viewDir(0.0f, -1.0f, -2.0f, 0.0f);

	static float* rotations = new float[2]{ 0.0f, 0.0f};
	ImGui::SliderFloat3("Camera Position", &camPos.x, -10, 10);
	ImGui::SliderFloat2("Camera Rotation", rotations, -3.14f, 3.14f);

	glm::vec4 d = glm::rotate(glm::mat4(1.0f), -rotations[0], glm::vec3(0.0f, 1.0f, 0.0f)) * viewDir;
	d = glm::rotate(glm::mat4(1.0f), rotations[1], glm::vec3(1.0f, 0.0f, 0.0f)) * d;

	//ubo.model = glm::rotate(glm::mat4(1.0f), deltaTime * glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.view = glm::lookAt(camPos, camPos + (glm::vec3)d, glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), SURFACE_WIDTH / (float)SURFACE_HEIGHT, 0.01f, 100.0f);
	ubo.proj[1][1] *= -1;

	uint32_t srcOffset = sizeof(Uniforms) * frameIndex;

	Uniforms* data = (Uniforms*)device.mapMemory(mUniformStagingBufferMemory, srcOffset, sizeof(Uniforms));
	memcpy(data, &ubo, sizeof(Uniforms));
	device.unmapMemory(mUniformStagingBufferMemory);


	vk::BufferCopy region{ srcOffset, 0, sizeof(Uniforms) };
	buffer.copyBuffer(mUniformStagingBuffer, mUniformBuffer, region);
}

void Material::Bind(const vk::CommandBuffer& cmdBuffer) {
	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mGfxPipeline);
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mPipelineLayout, 0, mDescriptorSet, {});
}


void Material::createStages(vk::Device device) {
	//There are not cleaned up :^)
	vk::ShaderModule vert = createShaderModule(device, "shaders/vert.spv");
	vk::ShaderModule frag = createShaderModule(device, "shaders/frag.spv");

	mStages = {
		{{}, vk::ShaderStageFlagBits::eVertex, vert, "main"},
		{{}, vk::ShaderStageFlagBits::eFragment, frag, "main"}
	};
}

void Material::createPipeline(vk::Device device, vk::RenderPass renderpass, uint32_t width, uint32_t height) {
	vk::Viewport viewport{ 0, 0, (float)width, (float)height, 0.0f, 1.0f };
	vk::Rect2D scissor{ { (uint32_t)0, (uint32_t)0 }, { width, height } };
	vk::PipelineColorBlendAttachmentState blendAttachmentState{ VK_FALSE, vk::BlendFactor::eOne, vk::BlendFactor::eZero,
		vk::BlendOp::eAdd, vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
	vk::VertexInputBindingDescription vertBindings = Vertex::getBindingDesc();
	std::vector<vk::VertexInputAttributeDescription> vertAttributes = Vertex::getAttrDesc();

	vk::PipelineVertexInputStateCreateInfo vertexInputCreateInfo{ {}, 1, &vertBindings, (uint32_t)vertAttributes.size(), vertAttributes.data() };
	vk::PipelineInputAssemblyStateCreateInfo assemblyCreateInfo{ {}, vk::PrimitiveTopology::eTriangleList, VK_FALSE };
	vk::PipelineViewportStateCreateInfo viewportStateCreateInfo{ {}, 1, &viewport, 1, &scissor };
	vk::PipelineRasterizationStateCreateInfo rasterCreateInfo{ {}, VK_FALSE, VK_FALSE, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise, VK_FALSE, 0, 0, 0, 1.0f };
	vk::PipelineMultisampleStateCreateInfo multisampleCreateInfo{ {}, vk::SampleCountFlagBits::e1, VK_FALSE, 1.0f, nullptr, VK_FALSE, VK_FALSE };
	vk::PipelineDepthStencilStateCreateInfo depthCreateInfo{ {}, VK_TRUE, VK_TRUE, vk::CompareOp::eLess, VK_FALSE, VK_FALSE, {}, {}, 0.0f, 1.0f };
	vk::PipelineColorBlendStateCreateInfo blendStateCreateInfo{ {}, VK_FALSE, vk::LogicOp::eCopy, 1, &blendAttachmentState, std::array<float, 4>({ 0.0f, 0.0f, 0.0f, 0.0f }) };
	vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo{ {}, 0, nullptr };

	vk::PipelineLayoutCreateInfo layoutCreateInfo{ {}, 1, &mDescriptorSetLayout, 0, nullptr };
	mPipelineLayout = device.createPipelineLayout(layoutCreateInfo);

	vk::GraphicsPipelineCreateInfo pipelineCreateInfo{ {}, (uint32_t)mStages.size(), mStages.data(), &vertexInputCreateInfo, &assemblyCreateInfo, nullptr, &viewportStateCreateInfo,
		&rasterCreateInfo,&multisampleCreateInfo, &depthCreateInfo, &blendStateCreateInfo,&dynamicStateCreateInfo, mPipelineLayout, renderpass, 0, {}, -1 };
	mGfxPipeline = device.createGraphicsPipeline({}, pipelineCreateInfo);
}

void Material::createDescriptorSetLayout(vk::Device device) {
	std::vector<vk::DescriptorSetLayoutBinding> bindings{
		vk::DescriptorSetLayoutBinding { 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr },
		vk::DescriptorSetLayoutBinding { 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr }
	};
	
	vk::DescriptorSetLayoutCreateInfo layoutCreateInfo({}, (uint32_t)bindings.size(), bindings.data());
	mDescriptorSetLayout = device.createDescriptorSetLayout(layoutCreateInfo);
}
void Material::createUniformBuffer(vk::Device device, vk::PhysicalDevice physDevice, uint32_t maxInFlight) {
	//Buffer alignement!!
	VulkanUtils::createBuffer(device, physDevice, sizeof(Uniforms) * maxInFlight, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, mUniformStagingBuffer, mUniformStagingBufferMemory);
	VulkanUtils::createBuffer(device, physDevice, sizeof(Uniforms), vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal, mUniformBuffer, mUniformBufferMemory);
}
void Material::createDescriptorSet(vk::Device device, vk::DescriptorPool pool) {
	vk::DescriptorSetAllocateInfo allocateInfo(pool, 1, &mDescriptorSetLayout);
	mDescriptorSet = device.allocateDescriptorSets(allocateInfo)[0];

	vk::DescriptorBufferInfo bufferInfo{ mUniformBuffer, 0, sizeof(Uniforms) };
	vk::DescriptorImageInfo imageInfo{ mSampler, mImage.GetImageView(), vk::ImageLayout::eShaderReadOnlyOptimal };
	std::vector<vk::WriteDescriptorSet> writes = {
		vk::WriteDescriptorSet{ mDescriptorSet, 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &bufferInfo, nullptr },
		vk::WriteDescriptorSet{ mDescriptorSet, 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &imageInfo, nullptr, nullptr }
	};
	device.updateDescriptorSets(writes, nullptr);
}