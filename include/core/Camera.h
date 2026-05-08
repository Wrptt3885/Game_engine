#pragma once

#include <glm/glm.hpp>

class Camera {
public:
    Camera();
    ~Camera();

    void SetPerspective(float fov, float aspectRatio, float nearPlane, float farPlane);
    void SetOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane);

    const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
    const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
    glm::mat4 GetViewProjectionMatrix() const { return m_ProjectionMatrix * m_ViewMatrix; }

    void SetPosition(const glm::vec3& position);
    void SetTarget(const glm::vec3& target);
    void SetUp(const glm::vec3& up) { m_Up = up; UpdateViewMatrix(); }

    const glm::vec3& GetPosition() const { return m_Position; }
    const glm::vec3& GetTarget()   const { return m_Target; }
    const glm::vec3& GetUp()       const { return m_Up; }
    glm::vec3 GetForward() const; // XZ-projected, toward target
    glm::vec3 GetRight()   const; // XZ-projected, rightward

    // Spectator (free-fly) controls
    void SetSpectatorStart(const glm::vec3& pos, const glm::vec3& lookAt);
    void LookSpectator(float dyaw, float dpitch);
    void MoveSpectator(float forward, float right, float up, float speed);
    glm::vec3 GetLookForward() const;

    // Follow camera: orbit behind 'target' at 'distance', 'heightOffset' above.
    // Uses current yaw/pitch (mouse-controlled) without corrupting them.
    void SetFollowCamera(const glm::vec3& target, float distance, float heightOffset);

    // Legacy orbit controls (kept for compatibility)
    void  Rotate(float yaw, float pitch);
    void  Pan(const glm::vec3& offset);
    void  Zoom(float factor);
    void  Follow(const glm::vec3& target);
    float GetDistance() const { return m_Distance; }

private:
    void UpdateViewMatrix();
    void SyncOrbitState();

    glm::mat4 m_ProjectionMatrix;
    glm::mat4 m_ViewMatrix;

    glm::vec3 m_Position;
    glm::vec3 m_Target;
    glm::vec3 m_Up;

    float m_Yaw;
    float m_Pitch;
    float m_Distance;
};
