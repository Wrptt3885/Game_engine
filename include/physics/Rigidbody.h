#pragma once
#include "core/Component.h"
#include <glm/glm.hpp>

class Rigidbody : public Component {
public:
    float     mass        = 1.0f;
    bool      useGravity  = true;
    float     friction    = 0.2f;
    float     restitution = 0.0f;
    glm::vec3 velocity    = glm::vec3(0.0f);

    void AddImpulse(const glm::vec3& impulse) { velocity += impulse / mass; }
    void Update(float dt) override;
};
