#pragma once

#include "core/Transform.h"
#include "core/Component.h"
#include <string>
#include <memory>
#include <vector>
#include <algorithm>

class Camera;

class GameObject {
public:
    GameObject(const std::string& name = "GameObject");
    ~GameObject();

    const std::string& GetName() const { return m_Name; }
    Transform& GetTransform() { return m_Transform; }
    const Transform& GetTransform() const { return m_Transform; }

    bool IsActive() const { return m_Active; }
    void SetActive(bool active) { m_Active = active; }

    template<typename T, typename... Args>
    std::shared_ptr<T> AddComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
        auto comp = std::make_shared<T>(std::forward<Args>(args)...);
        comp->m_GameObject = this;
        m_Components.push_back(comp);
        return comp;
    }

    template<typename T>
    std::shared_ptr<T> GetComponent() const {
        for (auto& comp : m_Components) {
            auto casted = std::dynamic_pointer_cast<T>(comp);
            if (casted) return casted;
        }
        return nullptr;
    }

    template<typename T>
    bool HasComponent() const {
        return GetComponent<T>() != nullptr;
    }

    template<typename T>
    void RemoveComponent() {
        m_Components.erase(
            std::remove_if(m_Components.begin(), m_Components.end(),
                [](const std::shared_ptr<Component>& c) {
                    return std::dynamic_pointer_cast<T>(c) != nullptr;
                }),
            m_Components.end()
        );
    }

    const std::vector<std::shared_ptr<Component>>& GetComponents() const { return m_Components; }

    void Update(float deltaTime);
    void Render(const Camera& camera);

    // Forwards collision events to all components
    void DispatchCollisionEnter(GameObject* other);
    void DispatchCollisionStay(GameObject* other);
    void DispatchCollisionExit(GameObject* other);

protected:
    std::string m_Name;
    Transform m_Transform;
    bool m_Active = true;
    std::vector<std::shared_ptr<Component>> m_Components;
};
