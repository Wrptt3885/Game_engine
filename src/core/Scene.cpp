#include "core/Scene.h"
#include "core/Camera.h"
#include "graphics/Light.h"
#include "graphics/LightData.h"
#include "renderer/Renderer.h"
#include <algorithm>
#include <iostream>

Scene::Scene(const std::string& name) : m_Name(name) {}

Scene::~Scene() {
    m_GameObjects.clear();
}

std::shared_ptr<GameObject> Scene::CreateGameObject(const std::string& name) {
    auto obj = std::make_shared<GameObject>(name);
    m_GameObjects.push_back(obj);
    return obj;
}

std::shared_ptr<GameObject> Scene::FindGameObject(const std::string& name) const {
    for (auto& obj : m_GameObjects)
        if (obj && obj->GetName() == name) return obj;
    return nullptr;
}

void Scene::DestroyGameObject(std::shared_ptr<GameObject> obj) {
    auto it = std::find(m_GameObjects.begin(), m_GameObjects.end(), obj);
    if (it == m_GameObjects.end()) return;
    if (m_OnDestroy) m_OnDestroy(obj.get());
    m_GameObjects.erase(it);
}

void Scene::Update(float deltaTime) {
    for (auto& obj : m_GameObjects) {
        if (obj && obj->IsActive())
            obj->Update(deltaTime);
    }
}

void Scene::OnGUI() {
    for (auto& obj : m_GameObjects) {
        if (obj && obj->IsActive())
            obj->OnGUI();
    }
}

void Scene::Render(const Camera& camera) {
    std::vector<DirLightData> dirLights;
    std::vector<PointLightData> pointLights;

    for (auto& obj : m_GameObjects) {
        if (!obj || !obj->IsActive()) continue;
        auto light = obj->GetComponent<Light>();
        if (!light) continue;

        if (light->type == LightType::Directional) {
            dirLights.push_back({ light->direction, light->color, light->intensity });
        } else {
            pointLights.push_back({
                obj->GetTransform().position,
                light->color,
                light->intensity,
                light->radius
            });
        }
    }

    Renderer::BeginScene(camera, dirLights, pointLights);

    for (auto& obj : m_GameObjects) {
        if (obj && obj->IsActive())
            obj->Render(camera);
    }
}
