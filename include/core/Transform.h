#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct Transform {
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    Transform() 
        : position(0.0f, 0.0f, 0.0f), 
          rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)), 
          scale(1.0f, 1.0f, 1.0f) {}

    glm::mat4 GetMatrix() const;
    
    // Helper functions
    void SetPosition(const glm::vec3& pos) { position = pos; }
    void SetRotation(const glm::vec3& eulerAngles);
    void SetScale(const glm::vec3& s) { scale = s; }
};
