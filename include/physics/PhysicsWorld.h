#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <vector>

class Scene;
class GameObject;
class IPhysicsSystem;

// Coordinator สำหรับทุก physics simulation system
// ปัจจุบัน: wrap JoltPhysicsSystem (rigid body)
// อนาคต: route ไปยัง FluidSystem, SoftBodySystem ตาม PhysicsMaterial::Phase
class PhysicsWorld {
public:
    void Init();
    void Shutdown();

    // เรียกตอน StartPlay — sync ทุก object ใน scene เข้า simulation
    void SyncFromScene(Scene& scene);

    // เรียกทุก frame ขณะ play mode
    void Update(Scene& scene, float dt);

    // ลบ body ของ GameObject ออกจากทุก system (ใช้ตอน DestroyGameObject)
    void RemoveBody(GameObject* go);

    // ลบ body ทั้งหมด (ใช้ตอน StopPlay)
    void ClearBodies();

    bool IsReady() const;

    // ---- Character (play mode) -----------------------------------------------
    void      CreateCharacter(const glm::vec3& spawnPos);
    void      DestroyCharacter();
    void      MoveCharacter(float dt, const glm::vec3& wishDir, bool jump);
    glm::vec3 GetCharacterPosition() const;
    bool      HasCharacter() const;
    bool      IsCharacterOnGround() const;
    bool      RaycastGround(const glm::vec3& from, float maxDist, float& outHitY) const;
    void      SnapCharacterToGround(float targetY);

    // ---- Future extensibility ------------------------------------------------
    // เพิ่ม custom system ได้ในอนาคต
    // void RegisterSystem(std::unique_ptr<IPhysicsSystem> system);
};
