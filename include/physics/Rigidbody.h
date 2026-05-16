#pragma once
#include "core/Component.h"
#include "physics/PhysicsMaterial.h"
#include <glm/glm.hpp>

class Rigidbody : public Component {
public:
    PhysicsMaterial material;           // per-object material: friction, restitution, density, phase, optical
    float     mass       = 1.0f;
    bool      useGravity = true;
    glm::vec3 velocity   = glm::vec3(0.0f);

    void AddImpulse(const glm::vec3& impulse) { velocity += impulse / mass; }
    void Update(float dt) override;
};
