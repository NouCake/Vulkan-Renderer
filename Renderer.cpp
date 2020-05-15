#include "Renderer.h"

//for debug scene
#include "Material.h"
#include "Mesh.h"

void Renderer::drawScene(const GraphicsVulkan& gfx) {
	//DebugScene START

	static Material mat(gfx, *this);
	static std::vector<Mesh*> meshes;
	if (meshes.size() == 0) {
		Assimp::Importer imp;
		const aiScene* scene = imp.ReadFile("Resources/sponza.obj", aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs);
		meshes.resize(scene->mNumMeshes);
		for (int i = 0; i < scene->mNumMeshes; i++) {
			meshes[i] = new Mesh(gfx, scene->mMeshes[i]);
		}
	}


	//DebugScene END

	uint32_t currentSwapchainImageIndex = gfx.GetCurrentSwapchainImageIndex();
	vk::CommandBuffer cmdBuffer = gfx.GetCurrentCommandbuffer();

	mBeginInfo.framebuffer = mFramebuffers[currentSwapchainImageIndex];

	mat.UpdateUniforms(gfx.mDevice, cmdBuffer, gfx.currentFrame);

	cmdBuffer.beginRenderPass(mBeginInfo, vk::SubpassContents::eInline);
	mat.Bind(cmdBuffer);
	for (Mesh* m : meshes) {
		m->Bind(cmdBuffer);
		cmdBuffer.drawIndexed(m->GetIndexCount(), 1, 0, 0, 0);
	}
	cmdBuffer.endRenderPass();

	ImGui::Render();

	cmdBuffer.beginRenderPass(vk::RenderPassBeginInfo{ mImguiRenderpass, mImguiFramebuffers[currentSwapchainImageIndex], mBeginInfo.renderArea, 1, mBeginInfo.pClearValues }, vk::SubpassContents::eInline);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
	cmdBuffer.endRenderPass();
}