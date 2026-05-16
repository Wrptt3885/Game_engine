#include "physics/Collider.h"
#include "core/GameObject.h"

AABB Collider::GetWorldAABB() const {
    glm::vec3 center = GetGameObject()->GetTransform().position + offset;
    return { center - size * 0.5f, center + size * 0.5f };
}
