#include "core/Application.h"
#include "core/SceneSerializer.h"
#include "core/Scene.h"
#include "core/GameObject.h"
#include "renderer/Renderer.h"
#include "platform/Input.h"
#include "core/Camera.h"
#include "graphics/Light.h"
#include "physics/JoltPhysicsSystem.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>

#ifdef USE_DX11_BACKEND
#  include "rhi/dx11/DX11Context.h"
#endif

static const std::string SCENE_PATH = std::string(ASSET_DIR) + "/scenes/MainScene.json";

// ---- ProcessInput -----------------------------------------------------------

void Application::ProcessInput(float deltaTime) {
    Input::Update();
    if (!m_Window) return;

    if (glfwGetKey(m_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        if (m_IsPlaying) StopPlay();
        else             glfwSetWindowShouldClose(m_Window, true);
    }
    if (Input::IsKeyDown(GLFW_KEY_F5) && m_CurrentScene)
        SceneSerializer::Save(*m_CurrentScene, SCENE_PATH);
    if (Input::IsKeyDown(GLFW_KEY_F11))
        m_Editor.ToggleFullscreen();

    float scroll = Input::ConsumeScrollDelta();
    if (scroll != 0.0f) {
        m_FlySpeed *= (scroll > 0 ? 1.3f : 0.77f);
        m_FlySpeed = glm::clamp(m_FlySpeed, 0.5f, 200.0f);
    }

    bool rmb = Input::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)
               && !m_Editor.WantsCaptureMouse();
    glm::vec2 mousePos = Input::GetMousePosition();
    if (rmb) {
        if (!m_RMBWasPressed) {
            m_LastMousePos = mousePos;
            glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glm::vec2 delta = mousePos - m_LastMousePos;
            m_Camera->LookSpectator(delta.x * 0.12f, -delta.y * 0.12f);
        }
    } else if (m_RMBWasPressed) {
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    m_LastMousePos  = mousePos;
    m_RMBWasPressed = rmb;

    if (!m_Editor.WantsCaptureKeyboard()) {
        if (m_IsPlaying) {
            glm::vec3 fwd = m_Camera->GetLookForward();
            fwd.y = 0.0f;
            if (glm::length(fwd) > 0.001f) fwd = glm::normalize(fwd);
            glm::vec3 right = glm::normalize(glm::cross(fwd, glm::vec3(0.0f, 1.0f, 0.0f)));

            m_CharMoveDir = glm::vec3(0.0f);
            if (Input::IsKeyPressed(GLFW_KEY_W)) m_CharMoveDir += fwd;
            if (Input::IsKeyPressed(GLFW_KEY_S)) m_CharMoveDir -= fwd;
            if (Input::IsKeyPressed(GLFW_KEY_A)) m_CharMoveDir -= right;
            if (Input::IsKeyPressed(GLFW_KEY_D)) m_CharMoveDir += right;
            if (glm::length(m_CharMoveDir) > 0.001f)
                m_CharMoveDir = glm::normalize(m_CharMoveDir);
            m_CharJump = Input::IsKeyDown(GLFW_KEY_SPACE);
        } else {
            float speed = m_FlySpeed * deltaTime;
            if (Input::IsKeyPressed(GLFW_KEY_LEFT_SHIFT)) speed *= 3.0f;

            float fwd = 0, rgt = 0, up = 0;
            if (Input::IsKeyPressed(GLFW_KEY_W))           fwd += 1;
            if (Input::IsKeyPressed(GLFW_KEY_S))           fwd -= 1;
            if (Input::IsKeyPressed(GLFW_KEY_A))           rgt -= 1;
            if (Input::IsKeyPressed(GLFW_KEY_D))           rgt += 1;
            if (Input::IsKeyPressed(GLFW_KEY_E) ||
                Input::IsKeyPressed(GLFW_KEY_SPACE))        up  += 1;
            if (Input::IsKeyPressed(GLFW_KEY_Q) ||
                Input::IsKeyPressed(GLFW_KEY_LEFT_CONTROL)) up  -= 1;
            if (fwd || rgt || up)
                m_Camera->MoveSpectator(fwd, rgt, up, speed);
        }
    }
}

// ---- Update -----------------------------------------------------------------

void Application::Update(float deltaTime) {
    if (!m_IsPlaying && m_Floor) {
        glm::vec3 cam = m_Camera->GetPosition();
        m_Floor->GetTransform().position.x = cam.x;
        m_Floor->GetTransform().position.z = cam.z;
    }

    if (m_Editor.ConsumePlayRequest()) StartPlay();
    if (m_Editor.ConsumeStopRequest()) StopPlay();

    if (m_IsPlaying && JoltPhysicsSystem::HasCharacter()) {
        JoltPhysicsSystem::MoveCharacter(deltaTime, m_CharMoveDir, m_CharJump);
        m_CharJump = false;

        glm::vec3 charPos = JoltPhysicsSystem::GetCharacterPosition();
        if (glm::length(m_CharMoveDir) > 0.001f)
            m_CharFacingYaw = atan2(m_CharMoveDir.x, m_CharMoveDir.z);
        glm::quat facing = glm::angleAxis(m_CharFacingYaw, glm::vec3(0.0f, 1.0f, 0.0f));

        if (m_CharacterBody) {
            m_CharacterBody->GetTransform().position = charPos + glm::vec3(0.0f, -0.3f, 0.0f);
            m_CharacterBody->GetTransform().rotation = facing;
        }
        if (m_CharacterHead) {
            m_CharacterHead->GetTransform().position = charPos + glm::vec3(0.0f, 0.75f, 0.0f);
            m_CharacterHead->GetTransform().rotation = facing;
        }
        m_Camera->SetFollowCamera(charPos + glm::vec3(0.0f, 0.8f, 0.0f), 5.0f, 1.5f);
    }

    if (m_CurrentScene) {
        m_CurrentScene->Update(deltaTime);
        if (m_IsPlaying) JoltPhysicsSystem::Update(*m_CurrentScene, deltaTime);
    }
}

// ---- Render -----------------------------------------------------------------

void Application::Render() {
    if (m_CurrentScene && m_Sun)
        Renderer::ShadowPass(*m_CurrentScene, m_Sun->direction);
#ifdef USE_DX11_BACKEND
    if (m_CurrentScene)
        Renderer::PointShadowPass(*m_CurrentScene);
#endif

    int w, h;
    glfwGetFramebufferSize(m_Window, &w, &h);
    if (w <= 0 || h <= 0) return;

    m_Camera->SetPerspective(45.0f, (float)w / h,
                             0.1f, std::max(500.0f, m_Camera->GetDistance() * 5.0f));

#ifdef USE_DX11_BACKEND
    if (m_CurrentScene) Renderer::GBufferPass(*m_CurrentScene, *m_Camera, w, h);
    Renderer::SSAOPass(*m_Camera, w, h);
    Renderer::SSAOBlurPass();
#endif

    Renderer::BeginHDRPass(w, h);
    Renderer::Clear();
    Renderer::DrawSkybox(*m_Camera, m_Sun ? m_Sun->direction : glm::vec3(0.0f, -1.0f, 0.0f));
    if (m_CurrentScene) m_CurrentScene->Render(*m_Camera);
    Renderer::EndHDRPass();
#ifdef USE_DX11_BACKEND
    Renderer::BloomPass(w, h);
#endif
    Renderer::ApplyTonemap();

    m_Editor.BeginFrame();
    if (m_CurrentScene)
        m_Editor.Render(*m_CurrentScene, *m_Camera, SCENE_PATH, m_IsPlaying);
    m_Editor.EndFrame();
}
