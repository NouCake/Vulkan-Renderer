#include "Renderer.h"

//for debug scene
#include "Material.h"
#include "Mesh.h"

void Renderer::drawScene(const GraphicsVulkan& gfx) {
	//DebugScene START

	static Material mat(gfx, *this);
	static Mesh mesh(gfx);

	//DebugScene END

	uint32_t currentSwapchainImageIndex = gfx.GetCurrentSwapchainImageIndex();
	vk::CommandBuffer cmdBuffer = gfx.GetCurrentCommandbuffer();

	mBeginInfo.framebuffer = mFramebuffers[currentSwapchainImageIndex];

	mat.UpdateUniforms(gfx.mDevice, cmdBuffer, gfx.currentFrame);

	cmdBuffer.beginRenderPass(mBeginInfo, vk::SubpassContents::eInline);
	mat.Bind(cmdBuffer);
	mesh.Bind(cmdBuffer);
	cmdBuffer.drawIndexed(mesh.GetIndexCount(), 1, 0, 0, 0);
	cmdBuffer.endRenderPass();
}