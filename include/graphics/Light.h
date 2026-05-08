#pragma once
#include "core/Component.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class LightType { Directional, Point };

class Light : public Component {
public:
    LightType type      = LightType::Directional;
    glm::vec3 color     = glm::vec3(1.0f);
    float     intensity = 1.0f;

    // Directional light
    glm::vec3 direction = glm::normalize(glm::vec3(-1.0f, -2.0f, -1.5f));

    // Point light
    float radius = 10.0f;
};
