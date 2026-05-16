#include "physics/PhysicsWorld.h"
#include "physics/JoltPhysicsSystem.h"
#include "core/Scene.h"
#include "core/GameObject.h"

// PhysicsWorld delegates to JoltPhysicsSystem (rigid body backend).
// อนาคต: เพิ่ม FluidSystem, SoftBodySystem โดย check PhysicsMaterial::Phase ของแต่ละ object

void PhysicsWorld::Init() {
    JoltPhysicsSystem::Init();
}

void PhysicsWorld::Shutdown() {
    JoltPhysicsSystem::Shutdown();
}

void PhysicsWorld::SyncFromScene(Scene& scene) {
    JoltPhysicsSystem::SyncBodiesFromScene(scene);
    // อนาคต: FluidSystem::SyncFromScene(scene), SoftBodySystem::SyncFromScene(scene)
}

void PhysicsWorld::Update(Scene& scene, float dt) {
    JoltPhysicsSystem::Update(scene, dt);
    // อนาคต: FluidSystem::Update(scene, dt)
}

void PhysicsWorld::RemoveBody(GameObject* go) {
    JoltPhysicsSystem::RemoveBody(go);
    // อนาคต: systems ทุกตัว RemoveBody(go)
}

void PhysicsWorld::ClearBodies() {
    JoltPhysicsSystem::ClearBodies();
    // อนาคต: ทุก system ClearBodies()
}

bool PhysicsWorld::IsReady() const {
    return JoltPhysicsSystem::IsReady();
}

// ---- Character ---------------------------------------------------------------

void PhysicsWorld::CreateCharacter(const glm::vec3& spawnPos) {
    JoltPhysicsSystem::CreateCharacter(spawnPos);
}

void PhysicsWorld::DestroyCharacter() {
    JoltPhysicsSystem::DestroyCharacter();
}

void PhysicsWorld::MoveCharacter(float dt, const glm::vec3& wishDir, bool jump) {
    JoltPhysicsSystem::MoveCharacter(dt, wishDir, jump);
}

glm::vec3 PhysicsWorld::GetCharacterPosition() const {
    return JoltPhysicsSystem::GetCharacterPosition();
}

bool PhysicsWorld::HasCharacter() const {
    return JoltPhysicsSystem::HasCharacter();
}

bool PhysicsWorld::IsCharacterOnGround() const {
    return JoltPhysicsSystem::IsCharacterOnGround();
}

bool PhysicsWorld::RaycastGround(const glm::vec3& from, float maxDist, float& outHitY) const {
    return JoltPhysicsSystem::RaycastGround(from, maxDist, outHitY);
}

void PhysicsWorld::SnapCharacterToGround(float targetY) {
    JoltPhysicsSystem::SnapCharacterToGround(targetY);
}
