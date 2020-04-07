#pragma once

//#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Graphics.h"

class NouEngine {
public:
    static NouEngine* createInstance();
private:
    struct EngineSettings {
        uint32_t windowWidth = 640;
        uint32_t windowHeight = 480;
        enum RenderAPI { DIRECTX, VULKAN } api = RenderAPI::VULKAN;
    };

public:
    NouEngine(const NouEngine&);
    ~NouEngine();
    void run();

private:
    NouEngine(EngineSettings& settings);

    const EngineSettings mSettings;
    GLFWwindow* mWindow = nullptr;
    Graphics* mGfx = nullptr;

    //runtime
    bool mRunning = false;

private:
    void initGLFW();
    void initGFX();

};