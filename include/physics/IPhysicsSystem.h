#pragma once
#include "physics/PhysicsMaterial.h"
#include <glm/glm.hpp>

class Scene;
class GameObject;

// Interface สำหรับ simulation backends ต่างๆ
// ตอนนี้มีแค่ RigidBodySystem (Jolt)
// อนาคต: FluidSystem (SPH), SoftBodySystem, etc.
class IPhysicsSystem {
public:
    virtual ~IPhysicsSystem() = default;

    virtual void Init()                                  = 0;
    virtual void Shutdown()                              = 0;
    virtual void SyncFromScene(Scene& scene)             = 0;
    virtual void Update(Scene& scene, float dt)          = 0;
    virtual void RemoveBody(GameObject* go)              = 0;
    virtual void ClearBodies()                           = 0;

    // บอกว่า system นี้ handle material phase ไหน
    virtual bool Handles(PhysicsMaterial::Phase phase) const = 0;
};
