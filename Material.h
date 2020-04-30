#pragma once

#include "GraphicsVulkan.h"
#include "Renderer.h"
#include "VulkanImage.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <fstream>

//VulkanMaterial
class Material {
private:
	struct Uniforms{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};
	struct Vertex {
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 uv;

		static vk::VertexInputBindingDescription getBindingDesc() {
			vk::VertexInputBindingDescription desc(0, sizeof(Vertex), vk::VertexInputRate::eVertex);
			return desc;
		}

		static std::vector<vk::VertexInputAttributeDescription> getAttrDesc() {
			std::vector<vk::VertexInputAttributeDescription> desc = {
				vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
				vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
				vk::VertexInputAttributeDescription{ 2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv) }
			};
			return desc;
		}
	};

public:
	Material(const GraphicsVulkan& gfx, const Renderer& renderer);
	~Material();
	Material(const Material&) = delete;
	Material& operator= (const Material&) = delete;

	void cleanup(const GraphicsVulkan& gfx);
	void UpdateUniforms(vk::Device device, vk::CommandBuffer buffer, uint32_t frameIndex);
	void Bind(const vk::CommandBuffer& cmdBuffer);

private:
	void createStages(vk::Device device);
	void createPipeline(vk::Device device, vk::RenderPass renderpass, uint32_t width, uint32_t height);
	void createDescriptorSetLayout(vk::Device device);
	void createUniformBuffer(vk::Device device, vk::PhysicalDevice physDevice, uint32_t maxInFlight);
	void createDescriptorSet(vk::Device device, vk::DescriptorPool pool);
	void createSampler(vk::Device device) {
		vk::SamplerCreateInfo createInfo{ {}, vk::Filter::eNearest, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, 0, VK_FALSE, 1, VK_FALSE, vk::CompareOp::eAlways, 0, 0, vk::BorderColor::eIntOpaqueBlack, VK_FALSE};
		mSampler = device.createSampler(createInfo);
	}

private:
	vk::DescriptorSetLayout mDescriptorSetLayout;
	vk::PipelineLayout mPipelineLayout;
	vk::Pipeline mGfxPipeline;
	std::vector<vk::PipelineShaderStageCreateInfo> mStages;
	vk::Buffer mUniformBuffer;
	vk::DeviceMemory mUniformBufferMemory;
	vk::DescriptorSet mDescriptorSet;

	vk::Buffer mUniformStagingBuffer;
	vk::DeviceMemory mUniformStagingBufferMemory;

	vk::Sampler mSampler;

	VulkanImage mImage;
	const GraphicsVulkan* mGfx;

	const uint32_t SURFACE_WIDTH;
	const uint32_t SURFACE_HEIGHT;

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