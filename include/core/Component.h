#pragma once

class GameObject;
class Camera;

class Component {
public:
    virtual ~Component() = default;

    virtual void Update(float deltaTime) {}
    virtual void Render(const Camera& camera) {}

    // Collision callbacks — override in subclasses to react to physics contacts.
    // 'other' is the other GameObject involved in the contact.
    virtual void OnCollisionEnter(GameObject* other) {}
    virtual void OnCollisionStay(GameObject* other)  {}
    virtual void OnCollisionExit(GameObject* other)  {}

    GameObject* GetGameObject() const { return m_GameObject; }

private:
    friend class GameObject;
    GameObject* m_GameObject = nullptr;
};
