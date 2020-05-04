#include "NouEngine.h"

#include "GraphicsVulkan.h"
#include "Renderer.h"

#include <iostream>
#include <exception>

NouEngine::NouEngine(EngineSettings& settings) :
	mSettings(settings) {

	std::cout << "Initialize NouEngine" << std::endl;

	initGLFW();
	initGFX();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForVulkan(mWindow, true);
}
NouEngine::NouEngine(const NouEngine&) {

	std::cout << "Copyconstruct NouEngine" << std::endl;
}
NouEngine::~NouEngine() {
	std::cout << "Deconstructing NouEngine" << std::endl;

	glfwDestroyWindow(mWindow);
	glfwTerminate();
}

void NouEngine::run() {
	mRunning = true;
	while (mRunning) {
		glfwPollEvents();

		GraphicsVulkan& gfx = (GraphicsVulkan&)*mGfx;
		static Renderer renderer(gfx);

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		mGfx->onFrameStart();

		renderer.drawScene(gfx);

		mGfx->onFrameEnd();
		mRunning = !glfwWindowShouldClose(mWindow);
	}
}

void NouEngine::initGLFW() {
	glfwInit();
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	mWindow = glfwCreateWindow(mSettings.windowWidth, mSettings.windowHeight, "NouEngine with Vulkan", nullptr, nullptr);
}

void NouEngine::initGFX() {
	if (mSettings.api == EngineSettings::RenderAPI::VULKAN) {
		mGfx = new GraphicsVulkan(mWindow);
	} else {
		throw std::exception("This RenderAPI is not supported yet!");
	}
}

NouEngine* NouEngine::createInstance() {
	static bool instanceCreated = false;
	
	if (instanceCreated == true) {
		std::cout << "An instance was already created!\nMultiple instances are not supported!" << std::endl;
		return nullptr;
	}

	EngineSettings settings = {};
	NouEngine* pEngine = new NouEngine(settings);
	
	instanceCreated = true;
	return pEngine;
}