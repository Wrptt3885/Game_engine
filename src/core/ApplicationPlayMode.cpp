#include "core/Application.h"
#include "core/SceneSerializer.h"
#include "core/Scene.h"
#include "core/Camera.h"
#include "renderer/OBJLoader.h"
#include "renderer/MeshRenderer.h"
#include "graphics/Material.h"
#include "graphics/Light.h"
#include "physics/JoltPhysicsSystem.h"

#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <iostream>

static const std::string SCENE_PATH_PM = std::string(ASSET_DIR) + "/scenes/MainScene.json";

void Application::StartPlay() {
    m_SceneSnapshot = SceneSerializer::SaveToString(*m_CurrentScene);
    m_IsPlaying = true;
    JoltPhysicsSystem::SyncBodiesFromScene(*m_CurrentScene);

    glm::vec3 camPos  = m_Camera->GetPosition();
    glm::vec3 spawnPos(camPos.x, std::max(camPos.y, 2.0f), camPos.z);
    JoltPhysicsSystem::CreateCharacter(spawnPos);

    // Visible character: body (blue) + head (skin)
    auto cubeMesh = OBJLoader::Load(ASSET_DIR "/models/cube.obj");

    m_CharacterBody = m_CurrentScene->CreateGameObject("__CharBody");
    {
        auto mr = m_CharacterBody->AddComponent<MeshRenderer>();
        mr->SetMesh(cubeMesh);
        Material mat;
        mat.albedo    = glm::vec3(0.25f, 0.50f, 0.92f);
        mat.roughness = 0.8f;
        mr->SetMaterial(mat);
        m_CharacterBody->GetTransform().scale = glm::vec3(0.6f, 1.6f, 0.6f);
    }

    m_CharacterHead = m_CurrentScene->CreateGameObject("__CharHead");
    {
        auto mr = m_CharacterHead->AddComponent<MeshRenderer>();
        mr->SetMesh(cubeMesh);
        Material mat;
        mat.albedo    = glm::vec3(0.90f, 0.76f, 0.65f);
        mat.roughness = 0.9f;
        mr->SetMaterial(mat);
        m_CharacterHead->GetTransform().scale = glm::vec3(0.5f, 0.5f, 0.5f);
    }

    glm::vec3 fwd = m_Camera->GetLookForward();
    m_CharFacingYaw = (fwd.x != 0.0f || fwd.z != 0.0f)
        ? atan2(fwd.x, fwd.z) : 0.0f;

    m_CurrentScene->SetDestroyCallback([](GameObject* go) {
        JoltPhysicsSystem::RemoveBody(go);
    });

    std::cout << "[PlayMode] Started\n";
}

void Application::StopPlay() {
    JoltPhysicsSystem::DestroyCharacter();
    if (m_CurrentScene) m_CurrentScene->ClearDestroyCallback();
    JoltPhysicsSystem::ClearBodies();
    m_Editor.ClearSelection();
    m_CharacterBody = nullptr;
    m_CharacterHead = nullptr;

    auto restored = SceneSerializer::LoadFromString(m_SceneSnapshot);
    if (restored) {
        m_CurrentScene = restored;
        ReconnectSceneReferences();
        JoltPhysicsSystem::SyncBodiesFromScene(*m_CurrentScene);
    }
    m_IsPlaying = false;
    std::cout << "[PlayMode] Stopped\n";
}
