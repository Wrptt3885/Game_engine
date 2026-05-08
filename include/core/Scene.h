#pragma once

#include "core/GameObject.h"
#include <vector>
#include <memory>
#include <string>
#include <functional>

class Camera;
class Light;

class Scene {
public:
    Scene(const std::string& name = "Scene");
    ~Scene();

    const std::string& GetName() const { return m_Name; }

    std::shared_ptr<GameObject> CreateGameObject(const std::string& name = "GameObject");
    void DestroyGameObject(std::shared_ptr<GameObject> obj);

    void SetDestroyCallback(std::function<void(GameObject*)> cb) { m_OnDestroy = std::move(cb); }
    void ClearDestroyCallback() { m_OnDestroy = nullptr; }

    void Update(float deltaTime);
    void Render(const Camera& camera);

    size_t GetGameObjectCount() const { return m_GameObjects.size(); }
    std::shared_ptr<GameObject> GetGameObject(size_t index) {
        return index < m_GameObjects.size() ? m_GameObjects[index] : nullptr;
    }
    std::shared_ptr<const GameObject> GetGameObject(size_t index) const {
        return index < m_GameObjects.size() ? m_GameObjects[index] : nullptr;
    }
    std::shared_ptr<GameObject> FindGameObject(const std::string& name) const;

private:
    std::string m_Name;
    std::vector<std::shared_ptr<GameObject>> m_GameObjects;
    std::function<void(GameObject*)> m_OnDestroy;
};
