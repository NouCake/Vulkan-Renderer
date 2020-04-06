#pragma once

#include "GraphicsVulkan.h"
#include "Renderer.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <fstream>

//VulkanMaterial
class Material {
private:
	struct Vertex {
		glm::vec2 pos;
		glm::vec3 color;

		static vk::VertexInputBindingDescription getBindingDesc() {
			vk::VertexInputBindingDescription desc(0, sizeof(Vertex), vk::VertexInputRate::eVertex);
			return desc;
		}

		static std::vector<vk::VertexInputAttributeDescription> getAttrDesc() {
			std::vector<vk::VertexInputAttributeDescription> desc = {
				vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos)),
				vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color))
			};
			return desc;
		}
	};

public:
	Material(const GraphicsVulkan& gfx, const Renderer& renderer) {
		createStages(gfx.mDevice);
		createDescriptorSetLayout(gfx.mDevice);
		createPipeline(gfx.mDevice, renderer.GetRenderPass(), gfx.SURFACE_WIDTH, gfx.SURFACE_HEIGHT);
	};
	void cleanup(const GraphicsVulkan& gfx) {
		//Clean Modules smhow
		gfx.mDevice.destroyPipeline(mGfxPipeline);
		gfx.mDevice.destroyDescriptorSetLayout(mDescriptorSetLayout);
	}

	void Bind(const vk::CommandBuffer& cmdBuffer) {
		cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mGfxPipeline);
	}

private:
	void createStages(vk::Device device) {
		vk::ShaderModule vert = createShaderModule(device, "shaders/vert.spv");
		vk::ShaderModule frag = createShaderModule(device, "shaders/frag.spv");

		mStages = {
			{{}, vk::ShaderStageFlagBits::eVertex, vert, "main"},
			{{}, vk::ShaderStageFlagBits::eFragment, frag, "main"}
		};
	}

	void createPipeline(vk::Device device, vk::RenderPass renderpass, uint32_t width, uint32_t height) {

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
		vk::PipelineColorBlendStateCreateInfo blendStateCreateInfo{ {}, VK_FALSE, vk::LogicOp::eCopy, 1, &blendAttachmentState, std::array<float, 4>({ 0.0f, 0.0f, 0.0f, 0.0f }) };
		vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo{ {}, 0, nullptr };

		vk::PipelineLayoutCreateInfo layoutCreateInfo({}, 0, nullptr, 0, nullptr);
		mPipelineLayout = device.createPipelineLayout(layoutCreateInfo);

		vk::GraphicsPipelineCreateInfo pipelineCreateInfo{ {}, (uint32_t)mStages.size(), mStages.data(), &vertexInputCreateInfo, &assemblyCreateInfo, nullptr, &viewportStateCreateInfo,
			& rasterCreateInfo,& multisampleCreateInfo, nullptr,& blendStateCreateInfo,& dynamicStateCreateInfo, mPipelineLayout, renderpass, 0, {}, -1 };
		mGfxPipeline = device.createGraphicsPipeline({}, pipelineCreateInfo);
	}

	void createDescriptorSetLayout(vk::Device device) {
		vk::DescriptorSetLayoutBinding uniformBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr);

		vk::DescriptorSetLayoutCreateInfo layoutCreateInfo({}, 1, &uniformBinding);
		mDescriptorSetLayout = device.createDescriptorSetLayout(layoutCreateInfo);
	}

private:
	vk::DescriptorSetLayout mDescriptorSetLayout;
	vk::PipelineLayout mPipelineLayout;
	vk::Pipeline mGfxPipeline;
	std::vector<vk::PipelineShaderStageCreateInfo> mStages;

private:
	vk::ShaderModule createShaderModule(vk::Device device, std::string filename) {
		std::vector<char> shaderCode = readFile(filename);
		vk::ShaderModuleCreateInfo moduleCreateInfo(vk::ShaderModuleCreateFlags(), shaderCode.size(), (uint32_t*)shaderCode.data());
		return device.createShaderModule(moduleCreateInfo);
	};
	static std::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::binary | std::ios::ate);

		if (file) {
			size_t fileSize = file.tellg();
			file.seekg(0);

			std::vector<char> buffer(fileSize);
			file.read(buffer.data(), fileSize);
			file.close();

			return buffer;
		}

		throw std::runtime_error("Failed to open");
	};
};