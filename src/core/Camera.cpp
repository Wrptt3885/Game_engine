#include "core/Camera.h"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera() 
    : m_Position(0.0f, 0.0f, 3.0f), 
      m_Target(0.0f, 0.0f, 0.0f), 
      m_Up(0.0f, 1.0f, 0.0f),
      m_Yaw(0.0f),
      m_Pitch(0.0f),
      m_Distance(3.0f) {
    
    SetPerspective(45.0f, 800.0f / 600.0f, 0.1f, 100.0f);
    UpdateViewMatrix();
}

Camera::~Camera() {
}

void Camera::SetPerspective(float fov, float aspectRatio, float nearPlane, float farPlane) {
    m_ProjectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}

void Camera::SetOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane) {
    m_ProjectionMatrix = glm::ortho(left, right, bottom, top, nearPlane, farPlane);
}

void Camera::UpdateViewMatrix() {
    m_ViewMatrix = glm::lookAt(m_Position, m_Target, m_Up);
}

void Camera::SyncOrbitState() {
    glm::vec3 dir = m_Position - m_Target;
    m_Distance = glm::length(dir);
    if (m_Distance > 0.0001f) {
        glm::vec3 n = dir / m_Distance;
        m_Pitch = glm::degrees(asin(glm::clamp(n.y, -1.0f, 1.0f)));
        m_Yaw   = glm::degrees(atan2(n.z, n.x));
    }
}

void Camera::SetPosition(const glm::vec3& position) {
    m_Position = position;
    UpdateViewMatrix();
}

void Camera::SetTarget(const glm::vec3& target) {
    m_Target = target;
    UpdateViewMatrix();
}

// ---- Spectator camera ----------------------------------------------------------

glm::vec3 Camera::GetLookForward() const {
    return glm::normalize(glm::vec3(
        cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch)),
        sin(glm::radians(m_Pitch)),
        sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch))
    ));
}

void Camera::SetSpectatorStart(const glm::vec3& pos, const glm::vec3& lookAt) {
    m_Position = pos;
    m_Distance = 1.0f;
    glm::vec3 look = glm::normalize(lookAt - pos);
    m_Pitch = glm::degrees(asin(glm::clamp(look.y, -1.0f, 1.0f)));
    m_Yaw   = glm::degrees(atan2(look.z, look.x));
    m_Target = m_Position + look;
    UpdateViewMatrix();
}

void Camera::LookSpectator(float dyaw, float dpitch) {
    m_Yaw  += dyaw;
    m_Pitch = glm::clamp(m_Pitch + dpitch, -89.0f, 89.0f);
    m_Target = m_Position + GetLookForward();
    UpdateViewMatrix();
}

void Camera::SetFollowCamera(const glm::vec3& target, float distance, float heightOffset) {
    glm::vec3 fwd = GetLookForward();
    m_Position = target - fwd * distance + glm::vec3(0.0f, heightOffset, 0.0f);
    m_Target   = target;
    UpdateViewMatrix();
}

void Camera::MoveSpectator(float forward, float right, float up, float speed) {
    glm::vec3 fwd = GetLookForward();
    glm::vec3 rgt = glm::cross(fwd, glm::vec3(0, 1, 0));
    float rgtLen = glm::length(rgt);
    rgt = (rgtLen > 0.001f) ? rgt / rgtLen : glm::vec3(1, 0, 0);

    m_Position += fwd * (forward * speed)
               + rgt * (right   * speed)
               + glm::vec3(0, 1, 0) * (up * speed);
    m_Target = m_Position + fwd;
    UpdateViewMatrix();
}

// ---- Legacy orbit --------------------------------------------------------------

glm::vec3 Camera::GetForward() const {
    glm::vec3 f(m_Target.x - m_Position.x, 0.0f, m_Target.z - m_Position.z);
    float len = glm::length(f);
    return len > 0.0001f ? f / len : glm::vec3(0.0f, 0.0f, -1.0f);
}

glm::vec3 Camera::GetRight() const {
    return glm::normalize(glm::cross(GetForward(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

void Camera::Follow(const glm::vec3& newTarget) {
    glm::vec3 delta = newTarget - m_Target;
    m_Target   += delta;
    m_Position += delta;
    UpdateViewMatrix();
}

void Camera::Rotate(float yaw, float pitch) {
    m_Yaw += yaw;
    m_Pitch += pitch;

    // Clamp pitch to avoid gimbal lock
    if (m_Pitch > 89.0f) m_Pitch = 89.0f;
    if (m_Pitch < -89.0f) m_Pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    direction.y = sin(glm::radians(m_Pitch));
    direction.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    
    m_Position = m_Target + glm::normalize(direction) * m_Distance;
    UpdateViewMatrix();
}

void Camera::Pan(const glm::vec3& offset) {
    m_Position += offset;
    m_Target += offset;
    UpdateViewMatrix();
}

void Camera::Zoom(float factor) {
    m_Distance *= factor;
    
    glm::vec3 direction = glm::normalize(m_Position - m_Target);
    m_Position = m_Target + direction * m_Distance;
    UpdateViewMatrix();
}
