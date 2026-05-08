#pragma once
#include <memory>
#include <glm/glm.hpp>

class Scene;
struct JoltContext;

class JoltPhysicsSystem {
public:
    static void Init();
    static void Shutdown();

    static void SyncBodiesFromScene(Scene& scene);
    static void ClearBodies();
    static void Update(Scene& scene, float dt);

    // Character (Play Mode)
    static void      CreateCharacter(const glm::vec3& spawnPos);
    static void      DestroyCharacter();
    static void      MoveCharacter(float dt, const glm::vec3& wishDir, bool jump);
    static glm::vec3 GetCharacterPosition();
    static bool      HasCharacter();

    static void RemoveBody(class GameObject* go);

    static bool IsReady() { return s_Ctx != nullptr; }

private:
    static std::unique_ptr<JoltContext> s_Ctx;
};
