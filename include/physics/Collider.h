#pragma once
#include "core/Component.h"
#include <glm/glm.hpp>

struct AABB {
    glm::vec3 min, max;
};

class Collider : public Component {
public:
    glm::vec3 size     = glm::vec3(1.0f);
    glm::vec3 offset   = glm::vec3(0.0f);
    bool      isStatic = false;

    AABB GetWorldAABB() const;
};
