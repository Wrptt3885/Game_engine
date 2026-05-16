#include "core/Application.h"
#include "core/SceneSerializer.h"
#include "core/Scene.h"
#include "core/GameObject.h"
#include "renderer/Renderer.h"
#include "renderer/SkinnedMeshRenderer.h"
#include "platform/Input.h"
#include "core/Camera.h"
#include "renderer/Light.h"
#include "audio/AudioManager.h"
#include "audio/AudioListener.h"
#include "scripting/LuaManager.h"
#include "ui/UICanvas.h"
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
    if (Input::IsKeyDown(GLFW_KEY_F6))
        Renderer::ReloadShaders();
    if (Input::IsKeyDown(GLFW_KEY_F11))
        m_Editor.ToggleFullscreen();

    float scroll = Input::ConsumeScrollDelta();
    if (scroll != 0.0f) {
        m_FlySpeed *= (scroll > 0 ? 1.3f : 0.77f);
        m_FlySpeed = glm::clamp(m_FlySpeed, 0.5f, 200.0f);
    }

    bool rmb = Input::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)
               && !m_Editor.WantsCaptureMouseForCamera();
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

    // Always clear movement each frame so animation returns to idle if keyboard is captured
    if (m_IsPlaying) {
        m_CharMoveDir   = glm::vec3(0.0f);
        m_CharSprinting = false;
    }

    if (!m_Editor.WantsCaptureKeyboard()) {
        if (m_IsPlaying) {
            glm::vec3 fwd = m_Camera->GetLookForward();
            fwd.y = 0.0f;
            if (glm::length(fwd) > 0.001f) fwd = glm::normalize(fwd);
            glm::vec3 right = glm::normalize(glm::cross(fwd, glm::vec3(0.0f, 1.0f, 0.0f)));

            if (Input::IsKeyPressed(GLFW_KEY_W)) m_CharMoveDir += fwd;
            if (Input::IsKeyPressed(GLFW_KEY_S)) m_CharMoveDir -= fwd;
            if (Input::IsKeyPressed(GLFW_KEY_A)) m_CharMoveDir -= right;
            if (Input::IsKeyPressed(GLFW_KEY_D)) m_CharMoveDir += right;
            if (glm::length(m_CharMoveDir) > 0.001f)
                m_CharMoveDir = glm::normalize(m_CharMoveDir);
            m_CharJump      = Input::IsKeyDown(GLFW_KEY_SPACE);
            m_CharSprinting = Input::IsKeyPressed(GLFW_KEY_LEFT_SHIFT);
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
    // Process deferred scene transitions (safe point — outside Lua/physics/render)
    if (m_PendingPop && !m_SceneStack.empty()) {
        std::string prev = m_SceneStack.back();
        m_SceneStack.pop_back();
        m_PendingPop  = false;
        m_PendingLoad = false;
        DoSceneTransition(prev);
        return;
    }
    if (m_PendingLoad) {
        std::string path = m_PendingPath;
        m_PendingLoad = false;
        DoSceneTransition(path);
        return;
    }

    if (!m_IsPlaying && m_Floor) {
        glm::vec3 cam = m_Camera->GetPosition();
        m_Floor->GetTransform().position.x = cam.x;
        m_Floor->GetTransform().position.z = cam.z;
    }

    if (m_Editor.ConsumePlayRequest()) StartPlay();
    if (m_Editor.ConsumeStopRequest()) StopPlay();

    if (m_IsPlaying && m_PhysicsWorld.HasCharacter()) {
        if (m_CharJump) m_CharJumpTimer = 0.6f;
        m_CharJumpTimer = std::max(0.0f, m_CharJumpTimer - deltaTime);
        bool wasJumping = m_CharJumpTimer > 0.0f;

        m_PhysicsWorld.MoveCharacter(deltaTime, m_CharMoveDir, m_CharJump);
        m_CharJump = false;

        glm::vec3 charPos = m_PhysicsWorld.GetCharacterPosition();
        if (glm::length(m_CharMoveDir) > 0.001f)
            m_CharFacingYaw = atan2(m_CharMoveDir.x, m_CharMoveDir.z);
        glm::quat facing = glm::angleAxis(m_CharFacingYaw, glm::vec3(0.0f, 1.0f, 0.0f));

        if (m_CharacterMesh) {
            float yOff = -1.1f;
            auto smr = m_CharacterMesh->GetComponent<SkinnedMeshRenderer>();
            if (smr) {
                yOff = smr->GetPhysicsYOffset();
                smr->EvaluateRules(glm::length(m_CharMoveDir) > 0.001f,
                                   m_CharSprinting, wasJumping);
            }
            m_CharacterMesh->GetTransform().position = charPos + glm::vec3(0.0f, yOff, 0.0f);
            m_CharacterMesh->GetTransform().rotation = facing;
        }
        m_Camera->SetFollowCamera(charPos + glm::vec3(0.0f, 0.8f, 0.0f), 5.0f, 1.5f);
    }

    LuaManager::SetScene(m_CurrentScene.get());
    LuaManager::SetTime(deltaTime);

    if (m_CurrentScene) {
        m_CurrentScene->Update(deltaTime);
        if (m_IsPlaying) m_PhysicsWorld.Update(*m_CurrentScene, deltaTime);
    }

    // Update audio listener — prefer AudioListener component, fall back to camera
    {
        bool listenerSet = false;
        if (m_CurrentScene) {
            for (size_t i = 0; i < m_CurrentScene->GetGameObjectCount(); i++) {
                auto obj = m_CurrentScene->GetGameObject(i);
                if (obj && obj->IsActive() && obj->GetComponent<AudioListener>()) {
                    // AudioListener::Update already calls SetListenerPosition
                    listenerSet = true;
                    break;
                }
            }
        }
        if (!listenerSet && m_Camera) {
            AudioManager::SetListenerPosition(
                m_Camera->GetPosition(),
                m_Camera->GetLookForward(),
                glm::vec3(0.0f, 1.0f, 0.0f));
        }
    }
}

// ---- Render -----------------------------------------------------------------

void Application::Render() {
    int winW, winH;
    glfwGetFramebufferSize(m_Window, &winW, &winH);
    if (winW <= 0 || winH <= 0) return;

    // When the scene viewport panel is active, the scene is rendered into
    // an offscreen texture sized to the panel; otherwise it covers the
    // whole window backbuffer.
    bool useViewport = m_Editor.IsSceneViewportEnabled()
                       && m_Editor.GetSceneViewportWidth()  > 0
                       && m_Editor.GetSceneViewportHeight() > 0;
    int sceneW = useViewport ? m_Editor.GetSceneViewportWidth()  : winW;
    int sceneH = useViewport ? m_Editor.GetSceneViewportHeight() : winH;

    if (useViewport) Renderer::ResizeViewport(sceneW, sceneH);

    m_Camera->SetPerspective(45.0f, (float)sceneW / (float)sceneH,
                             0.1f, std::max(500.0f, m_Camera->GetDistance() * 5.0f));

    // Shadow passes need an up-to-date camera projection so the ortho box
    // can fit the frustum.
    if (m_CurrentScene)
        Renderer::ShadowPass(*m_CurrentScene, *m_Camera);
#ifdef USE_DX11_BACKEND
    if (m_CurrentScene)
        Renderer::PointShadowPass(*m_CurrentScene);
#endif

#ifdef USE_DX11_BACKEND
    if (m_CurrentScene) Renderer::GBufferPass(*m_CurrentScene, *m_Camera, sceneW, sceneH);
    Renderer::SSAOPass(*m_Camera, sceneW, sceneH);
    Renderer::SSAOBlurPass();
#endif

    Renderer::BeginHDRPass(sceneW, sceneH);
    Renderer::Clear();
    Renderer::DrawSkybox(*m_Camera, m_Sun ? m_Sun->direction : glm::vec3(0.0f, -1.0f, 0.0f));
    if (m_CurrentScene) m_CurrentScene->Render(*m_Camera);
    Renderer::EndHDRPass();
#ifdef USE_DX11_BACKEND
    Renderer::BloomPass(sceneW, sceneH);
#endif

    if (useViewport) {
        Renderer::ApplyTonemapToViewport();
        Renderer::ClearBackbuffer(winW, winH);
    } else {
        Renderer::ApplyTonemap();
    }

    m_Editor.BeginFrame();
    UICanvas::BeginFrame((float)winW, (float)winH);
    if (m_CurrentScene) {
        m_CurrentScene->OnGUI();
        m_CurrentScene->GetUILayer().Render();
    }
    if (m_CurrentScene)
        m_Editor.Render(*m_CurrentScene, *m_Camera, SCENE_PATH, m_IsPlaying);
    m_Editor.EndFrame();
}
