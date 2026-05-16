#include "core/Application.h"
#include "renderer/Renderer.h"
#include "platform/Input.h"
#include "audio/AudioManager.h"
#include "scripting/LuaManager.h"
#include "core/Scene.h"
#include "core/Camera.h"
#include "core/GameObject.h"
#include "renderer/OBJLoader.h"
#include "renderer/MeshRenderer.h"
#include "renderer/Texture.h"
#include "renderer/Light.h"
#include "physics/Rigidbody.h"
#include "physics/Collider.h"
#include <GLFW/glfw3.h>
#include <iostream>

#ifdef USE_DX11_BACKEND
#  include "rhi/dx11/DX11Context.h"
#  include <windows.h>
#  define GLFW_EXPOSE_NATIVE_WIN32
#  include <GLFW/glfw3native.h>
#else
#  include <glad/gl.h>
#endif

void Application::InitWindow() {
    m_Window = nullptr;
    if (!glfwInit()) { std::cerr << "Failed to initialize GLFW\n"; return; }

    glfwSetErrorCallback([](int e, const char* d) {
        std::cerr << "GLFW Error (" << e << "): " << d << "\n";
    });

#ifdef USE_DX11_BACKEND
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_Window = glfwCreateWindow(800, 600, "MyEngine (DX11)", nullptr, nullptr);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    m_Window = glfwCreateWindow(800, 600, "MyEngine (OpenGL)", nullptr, nullptr);
#endif

    if (!m_Window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return;
    }

#ifdef USE_DX11_BACKEND
    HWND hwnd = glfwGetWin32Window(m_Window);
    if (!DX11Context::Init(hwnd, 800, 600)) {
        std::cerr << "Failed to initialize DX11 context\n";
        return;
    }
    glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow*, int w, int h) {
        if (w > 0 && h > 0) DX11Context::ResizeBuffers(w, h);
    });
#else
    glfwMakeContextCurrent(m_Window);
    glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow*, int w, int h) {
        glViewport(0, 0, w, h);
    });
    glfwSwapInterval(1);
#endif
    glfwShowWindow(m_Window);
}

void Application::InitRenderer() { Renderer::Init(); }

void Application::InitInput() { if (m_Window) Input::Init(m_Window); }

void Application::InitScene() {
    m_Camera = std::make_shared<Camera>();
    m_Camera->SetPerspective(45.0f, 800.0f / 600.0f, 0.1f, 500.0f);
    m_Camera->SetSpectatorStart(glm::vec3(0.0f, 3.0f, 8.0f), glm::vec3(0.0f, 0.5f, 0.0f));

    m_CurrentScene = std::make_shared<Scene>("MainScene");

    auto cube = m_CurrentScene->CreateGameObject("Cube");
    cube->GetTransform().position = glm::vec3(0.0f, 3.0f, 0.0f);
    auto mr = cube->AddComponent<MeshRenderer>();
    mr->SetMesh(OBJLoader::Load(ASSET_DIR "/models/cube.obj"));
    mr->SetTexture(Texture::Create(ASSET_DIR "/textures/cube.png"));
    auto rb = cube->AddComponent<Rigidbody>();
    rb->useGravity = true;
    cube->AddComponent<Collider>()->size = glm::vec3(1.0f);

    m_Floor = m_CurrentScene->CreateGameObject("Floor");
    m_Floor->GetTransform().position = glm::vec3(0.0f);
    auto floorMr = m_Floor->AddComponent<MeshRenderer>();
    floorMr->SetMesh(OBJLoader::Load(ASSET_DIR "/models/plane.obj"));
    Material floorMat;
    floorMat.texture      = Texture::Create(
        ASSET_DIR "/textures/marble_cliff_03_4k.blend/textures/marble_cliff_03_diff_4k.jpg");
    floorMat.worldSpaceUV = true;
    floorMat.worldUVTile  = 0.1f;
    floorMr->SetMaterial(floorMat);
    auto floorCol        = m_Floor->AddComponent<Collider>();
    floorCol->size       = glm::vec3(1000.0f, 0.2f, 1000.0f);
    floorCol->offset     = glm::vec3(0.0f, -0.1f, 0.0f);
    floorCol->isStatic   = true;

    auto sunObj  = m_CurrentScene->CreateGameObject("Sun");
    m_Sun        = sunObj->AddComponent<Light>();
    m_Sun->type      = LightType::Directional;
    m_Sun->direction = glm::normalize(glm::vec3(-1.0f, -2.0f, -1.5f));
    m_Sun->color     = glm::vec3(1.0f, 0.95f, 0.8f);
    m_Sun->intensity = 3.14159f;

    auto fillObj  = m_CurrentScene->CreateGameObject("FillLight");
    auto fill     = fillObj->AddComponent<Light>();
    fill->type      = LightType::Directional;
    fill->direction = glm::normalize(glm::vec3(1.0f, -0.5f, 1.0f));
    fill->color     = glm::vec3(0.4f, 0.5f, 0.7f);
    fill->intensity = 1.26f;

    m_CurrentScenePath = std::string(ASSET_DIR) + "/scenes/MainScene.json";
    ReconnectSceneReferences();
}

void Application::ReconnectSceneReferences() {
    m_Floor = m_CurrentScene->FindGameObject("Floor");
    auto sunObj = m_CurrentScene->FindGameObject("Sun");
    m_Sun = sunObj ? sunObj->GetComponent<Light>() : nullptr;
}

void Application::Shutdown() {
    m_Editor.Shutdown();
    Renderer::Shutdown();
    LuaManager::Shutdown();
    AudioManager::Shutdown();
    m_PhysicsWorld.Shutdown();
#ifdef USE_DX11_BACKEND
    DX11Context::Shutdown();
#endif
    if (m_Window) { glfwDestroyWindow(m_Window); m_Window = nullptr; }
    glfwTerminate();
}
