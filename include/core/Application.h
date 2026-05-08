#pragma once

#include "core/EditorLayer.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>

class Scene;
class Camera;
class Light;
class GameObject;

class Application {
public:
    Application();
    ~Application();

    void Run();
    void Shutdown();

    GLFWwindow* GetWindow() const { return m_Window; }
    bool IsRunning()  const { return m_Window && !glfwWindowShouldClose(m_Window); }
    bool IsPlaying()  const { return m_IsPlaying; }

    void StartPlay();
    void StopPlay();

private:
    void InitWindow();
    void InitRenderer();
    void InitInput();
    void InitScene();
    void ReconnectSceneReferences();
    void ProcessInput(float deltaTime);
    void Update(float deltaTime);
    void Render();

    GLFWwindow* m_Window;
    float m_LastFrameTime;

    std::shared_ptr<Scene>      m_CurrentScene;
    std::shared_ptr<Camera>     m_Camera;
    std::shared_ptr<Light>      m_Sun;
    std::shared_ptr<GameObject> m_Floor;

    EditorLayer m_Editor;

    glm::vec2   m_LastMousePos  = glm::vec2(0.0f);
    bool        m_RMBWasPressed = false;
    float       m_FlySpeed      = 5.0f;

    bool        m_IsPlaying     = false;
    std::string m_SceneSnapshot;

    glm::vec3   m_CharMoveDir    = glm::vec3(0.0f);
    bool        m_CharJump       = false;
    float       m_CharFacingYaw  = 0.0f;

    std::shared_ptr<GameObject> m_CharacterBody;
    std::shared_ptr<GameObject> m_CharacterHead;
};
