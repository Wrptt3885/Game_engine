#pragma once
#include <glm/glm.hpp>

struct DirLightData {
    glm::vec3 direction;
    glm::vec3 color;
    float     intensity;
};

struct PointLightData {
    glm::vec3 position;
    glm::vec3 color;
    float     intensity;
    float     radius;
};
