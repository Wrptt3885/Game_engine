#include "core/GameObject.h"
#include "core/Camera.h"

GameObject::GameObject(const std::string& name)
    : m_Name(name), m_Active(true) {
}

GameObject::~GameObject() {
}

void GameObject::Update(float deltaTime) {
    for (auto& comp : m_Components)
        comp->Update(deltaTime);
}

void GameObject::Render(const Camera& camera) {
    for (auto& comp : m_Components)
        comp->Render(camera);
}

void GameObject::DispatchCollisionEnter(GameObject* other) {
    for (auto& comp : m_Components) comp->OnCollisionEnter(other);
}

void GameObject::DispatchCollisionStay(GameObject* other) {
    for (auto& comp : m_Components) comp->OnCollisionStay(other);
}

void GameObject::DispatchCollisionExit(GameObject* other) {
    for (auto& comp : m_Components) comp->OnCollisionExit(other);
}
