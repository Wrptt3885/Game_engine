#include "core/Transform.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

glm::mat4 Transform::GetMatrix() const {
    glm::mat4 mat = glm::mat4_cast(rotation);
    mat[3][0] = position.x;
    mat[3][1] = position.y;
    mat[3][2] = position.z;
    
    return glm::scale(mat, scale);
}

void Transform::SetRotation(const glm::vec3& eulerAngles) {
    rotation = glm::quat(eulerAngles);
}
