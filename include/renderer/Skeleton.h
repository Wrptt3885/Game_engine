#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct Joint {
    std::string name;
    int         parentIndex       = -1;
    glm::mat4   inverseBindMatrix = glm::mat4(1.0f);
};

struct Skeleton {
    std::vector<Joint> joints;
};
