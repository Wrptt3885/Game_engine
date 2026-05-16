#include "core/Application.h"
#include "core/Scene.h"
#include "core/SceneSerializer.h"
#include "audio/AudioSource.h"
#include "scripting/LuaManager.h"
#include "editor/UndoManager.h"
#include <iostream>

// ---- Public API (deferred — safe to call from Lua mid-frame) ----------------

void Application::LoadScene(const std::string& path) {
    m_SceneStack.clear();
    m_PendingLoad = true;
    m_PendingPath = path;
}

void Application::PushScene(const std::string& path) {
    if (!m_CurrentScenePath.empty())
        m_SceneStack.push_back(m_CurrentScenePath);
    m_PendingLoad = true;
    m_PendingPath = path;
}

void Application::PopScene() {
    if (!m_SceneStack.empty())
        m_PendingPop = true;
}

// ---- Actual transition (called at start of next frame) ----------------------

void Application::DoSceneTransition(const std::string& path) {
    UndoManager::Clear();
    bool wasPlaying = m_IsPlaying;

    if (wasPlaying) {
        // Stop audio
        if (m_CurrentScene) {
            for (size_t i = 0; i < m_CurrentScene->GetGameObjectCount(); i++) {
                auto obj = m_CurrentScene->GetGameObject(i);
                if (obj)
                    if (auto as = obj->GetComponent<AudioSource>()) as->Stop();
            }
        }
        LuaManager::SetPlayMode(false);
        m_PhysicsWorld.DestroyCharacter();
        if (m_CurrentScene) m_CurrentScene->ClearDestroyCallback();
        m_PhysicsWorld.ClearBodies();
        m_Editor.ClearSelection();
        m_CharacterMesh = nullptr;
        m_IsPlaying = false;
    }

    auto newScene = SceneSerializer::Load(path);
    if (!newScene) {
        std::cerr << "[Scene] Failed to load: " << path << "\n";
        return;
    }

    m_CurrentScenePath = path;
    m_CurrentScene     = newScene;
    ReconnectSceneReferences();
    LuaManager::SetScene(m_CurrentScene.get());

    std::cout << "[Scene] Loaded: " << path << "\n";

    if (wasPlaying)
        StartPlay();
}
