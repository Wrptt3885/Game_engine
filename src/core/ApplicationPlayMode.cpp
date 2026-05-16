#include "core/Application.h"
#include "core/SceneSerializer.h"
#include "core/Scene.h"
#include "core/Camera.h"
#include "renderer/SkinnedMeshRenderer.h"
#include "renderer/Light.h"
#include "audio/AudioSource.h"
#include "scripting/LuaManager.h"
#include "scripting/LuaScript.h"
#include "editor/UndoManager.h"

#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <iostream>

static const std::string SCENE_PATH_PM = std::string(ASSET_DIR) + "/scenes/MainScene.json";

void Application::StartPlay() {
    UndoManager::Clear();
    m_SceneSnapshot = SceneSerializer::SaveToString(*m_CurrentScene);
    m_IsPlaying = true;
    m_PhysicsWorld.SyncFromScene(*m_CurrentScene);

    // Find first SkinnedMeshRenderer in scene as the player character
    m_CharacterMesh = nullptr;
    for (size_t i = 0; i < m_CurrentScene->GetGameObjectCount(); i++) {
        auto obj = m_CurrentScene->GetGameObject(i);
        if (obj && obj->GetComponent<SkinnedMeshRenderer>()) {
            m_CharacterMesh = obj;
            break;
        }
    }

    glm::vec3 spawnPos;
    if (m_CharacterMesh) {
        glm::vec3 meshPos = m_CharacterMesh->GetTransform().position;
        float yOff = -1.1f;
        if (auto smr = m_CharacterMesh->GetComponent<SkinnedMeshRenderer>())
            yOff = smr->GetPhysicsYOffset();
        // yOff is negative (e.g. -1.1), so meshPos.y - yOff = meshPos.y + 1.1 = capsule centre
        spawnPos = glm::vec3(meshPos.x, meshPos.y - yOff, meshPos.z);
    } else {
        glm::vec3 camPos = m_Camera->GetPosition();
        spawnPos = glm::vec3(camPos.x, std::max(camPos.y, 2.0f), camPos.z);
    }
    m_PhysicsWorld.CreateCharacter(spawnPos);

    // Raycast straight down to find the nearest floor and snap the character
    // there immediately, so it doesn't float when the mesh was placed high.
    {
        // Capsule half-total = halfHeight + radius = 0.8 + 0.3 = 1.1
        constexpr float kCapsuleHalf = 1.1f;
        float floorY = 0.0f;
        // Cast from slightly above spawn (avoids self-intersection with floor at same Y)
        if (m_PhysicsWorld.RaycastGround(
                glm::vec3(spawnPos.x, spawnPos.y + 0.5f, spawnPos.z),
                500.0f, floorY)) {
            float targetY = floorY + kCapsuleHalf;
            if (targetY < spawnPos.y)  // only snap down, never up
                m_PhysicsWorld.SnapCharacterToGround(targetY);
        }
    }

    glm::vec3 fwd = m_Camera->GetLookForward();
    m_CharFacingYaw = (fwd.x != 0.0f || fwd.z != 0.0f)
        ? atan2(fwd.x, fwd.z) : 0.0f;

    m_CurrentScene->SetDestroyCallback([this](GameObject* go) {
        m_PhysicsWorld.RemoveBody(go);
    });

    // Trigger OnPlayStart on every AudioSource
    for (size_t i = 0; i < m_CurrentScene->GetGameObjectCount(); i++) {
        auto obj = m_CurrentScene->GetGameObject(i);
        if (obj)
            if (auto as = obj->GetComponent<AudioSource>()) as->OnPlayStart();
    }

    // Enable Lua play mode and call Awake on every LuaScript
    LuaManager::SetPlayMode(true);
    for (size_t i = 0; i < m_CurrentScene->GetGameObjectCount(); i++) {
        auto obj = m_CurrentScene->GetGameObject(i);
        if (obj)
            if (auto ls = obj->GetComponent<LuaScript>()) ls->Reload();
    }

    std::cout << "[PlayMode] Started\n";
}

void Application::StopPlay() {
    UndoManager::Clear();
    // Stop all audio before restoring scene
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
    auto restored = SceneSerializer::LoadFromString(m_SceneSnapshot);
    if (restored) {
        m_CurrentScene = restored;
        ReconnectSceneReferences();
        m_PhysicsWorld.SyncFromScene(*m_CurrentScene);
    }
    std::cout << "[PlayMode] Stopped\n";
}
