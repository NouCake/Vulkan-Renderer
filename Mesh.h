#pragma once

#include "GraphicsVulkan.h"
#include "VulkanUtils.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Mesh {
private:
	const vk::MemoryPropertyFlags HOST_LOCAL = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
	struct Vertex {
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 uv;
	};
	 
public:
	Mesh(const GraphicsVulkan& gfx) {
		mGfx = &gfx;
		loadMesh();
		createBuffers(gfx.mDevice, gfx.mPhysicalDevice);
		fillBuffers(gfx.mDevice, gfx.mPhysicalDevice, gfx.mCommandPool, gfx.mGfxQueue);
	}
	~Mesh() {
		mGfx->mDevice.waitIdle();
		mGfx->mDevice.destroyBuffer(mVertexBuffer);
		mGfx->mDevice.destroyBuffer(mIndexBuffer);
		mGfx->mDevice.freeMemory(mVertexBufferMemory);
		mGfx->mDevice.freeMemory(mIndexBufferMemory);
	}
	Mesh(const Mesh&) = delete;
	Mesh& operator= (const Mesh&) = delete;

	void Bind(const vk::CommandBuffer& cmdBuffer) {
		cmdBuffer.bindVertexBuffers(0, mVertexBuffer, (uint64_t)0 );
		cmdBuffer.bindIndexBuffer(mIndexBuffer, 0, vk::IndexType::eUint16);
	}

	uint32_t GetIndexCount() {
		return mIndexData.size();
	}

private:
	void loadMesh() {
		mIndexData = {
			4, 0, 1,
			4, 1, 2,
			4, 2, 3,
			4, 3, 0,

			5+4, 5+0, 5+1,
			5+4, 5+1, 5+2,
			5+4, 5+2, 5+3,
			5+4, 5+3, 5+0,
		};

		mVertexData = {
			{{-0.5f, 0.25f, -0.5f}, {1.0f, 0.0f, 0.0f},	{1.0f, 1.0f}},
			{{ 0.5f, 0.25f, -0.5f}, {1.0f, 0.0f, 1.0f},	{0.0f, 1.0f}},
			{{ 0.5f, 0.25f,	0.5f }, {1.0f, 1.0f, 0.0f},	{0.0f, 0.0f}},
			{{-0.5f, 0.25f,  0.5f }, {1.0f, 1.0f, 1.0f},	{1.0f, 0.0f}},
			{{ 0.0f, 0.25f,  0.0f }, {0.5f, 0.5f, 0.5f},	{0.5f, 0.5f}},

			{{-0.5f, 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f},	{1.0f, 1.0f}},
			{{ 0.5f, 0.0f, -0.5f}, {1.0f, 0.0f, 1.0f},	{0.0f, 1.0f}},
			{{ 0.5f, 0.0f,	0.5f }, {1.0f, 1.0f, 0.0f},	{0.0f, 0.0f}},
			{{-0.5f, 0.0f,  0.5f }, {1.0f, 1.0f, 1.0f},	{1.0f, 0.0f}},
			{{ 0.0f, 0.0f,  0.0f }, {0.5f, 0.5f, 0.5f},	{0.5f, 0.5f}},
		};
	}
	void createBuffers(vk::Device device, vk::PhysicalDevice physDevice) {
		uint32_t vertexDataSize = sizeof(Vertex) * mVertexData.size();
		uint32_t indexDataSize = sizeof(uint16_t) * mIndexData.size();

		VulkanUtils::createBuffer(device, physDevice, vertexDataSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal, mVertexBuffer, mVertexBufferMemory);
		VulkanUtils::createBuffer(device, physDevice, indexDataSize, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal, mIndexBuffer, mIndexBufferMemory);
	}
	void fillBuffers(vk::Device device, vk::PhysicalDevice physDevice, vk::CommandPool pool, vk::Queue queue) {
		uint32_t vertexDataSize = sizeof(Vertex) * mVertexData.size();
		uint32_t indexDataSize = sizeof(uint16_t) * mIndexData.size();

		vk::Buffer tmpBuffer;
		vk::DeviceMemory tmpMemory;

		VulkanUtils::createBuffer(device, physDevice, vertexDataSize, vk::BufferUsageFlagBits::eTransferSrc, HOST_LOCAL, tmpBuffer, tmpMemory);
		void* data = device.mapMemory(tmpMemory, 0, vertexDataSize);
		memcpy(data, mVertexData.data(), vertexDataSize);
		device.unmapMemory(tmpMemory);

		VulkanUtils::copyBuffer(device, pool, queue, tmpBuffer, mVertexBuffer, vertexDataSize);

		//Not creating new buffer, because indexdatasize should be less than vertexdatasize
		if (indexDataSize > vertexDataSize) throw std::runtime_error("There was some lazy assertion that is not fullfilled: vertex <  index. have fun finding out what that means!!");
		data = device.mapMemory(tmpMemory, 0, indexDataSize);
		memcpy(data, mIndexData.data(), indexDataSize);
		device.unmapMemory(tmpMemory);

		VulkanUtils::copyBuffer(device, pool, queue, tmpBuffer, mIndexBuffer, indexDataSize);
	}

private:
	vk::Buffer mVertexBuffer;
	vk::Buffer mIndexBuffer;

	vk::DeviceMemory mVertexBufferMemory;
	vk::DeviceMemory mIndexBufferMemory;

	std::vector<Vertex> mVertexData;
	std::vector<uint16_t> mIndexData;

	const GraphicsVulkan* mGfx;

};