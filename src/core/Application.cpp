#include "core/Application.h"
#include "audio/AudioManager.h"
#include "scripting/LuaManager.h"
#include <GLFW/glfw3.h>
#include <iostream>

#ifdef USE_DX11_BACKEND
#  include "rhi/dx11/DX11Context.h"
#endif

Application::Application() : m_Window(nullptr), m_LastFrameTime(0.0f) {
    InitWindow();
    InitRenderer();
    InitInput();
    m_Editor.Init(m_Window);   // after Input so ImGui chains onto Input's GLFW callbacks
    m_PhysicsWorld.Init();
    AudioManager::Init();
    LuaManager::Init();
    InitScene();
    m_PhysicsWorld.SyncFromScene(*m_CurrentScene);
}

Application::~Application() {
    Shutdown();
}

void Application::Run() {
    if (!IsRunning()) {
        std::cerr << "Cannot run: window not initialized\n";
        return;
    }
    m_LastFrameTime = (float)glfwGetTime();

    while (IsRunning()) {
        float now = (float)glfwGetTime();
        float dt  = now - m_LastFrameTime;
        if (dt > 0.1f) dt = 0.1f;
        m_LastFrameTime = now;

        ProcessInput(dt);
        Update(dt);
        Render();

#ifdef USE_DX11_BACKEND
        DX11Context::Present(true);
#else
        glfwSwapBuffers(m_Window);
#endif
        glfwPollEvents();
    }
}
