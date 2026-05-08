#include "physics/PhysicsSystem.h"
#include "core/Scene.h"
#include "core/GameObject.h"
#include "physics/Rigidbody.h"
#include "physics/Collider.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <algorithm>

static bool Overlaps(const AABB& a, const AABB& b) {
    return (a.min.x < b.max.x && a.max.x > b.min.x) &&
           (a.min.y < b.max.y && a.max.y > b.min.y) &&
           (a.min.z < b.max.z && a.max.z > b.min.z);
}

void PhysicsSystem::Update(Scene& scene, float dt) {
    std::vector<std::shared_ptr<GameObject>> objects;
    for (size_t i = 0; i < scene.GetGameObjectCount(); i++) {
        auto obj = scene.GetGameObject(i);
        if (obj && obj->IsActive()) objects.push_back(obj);
    }

    for (size_t i = 0; i < objects.size(); i++) {
        auto colA = objects[i]->GetComponent<Collider>();
        if (!colA) continue;

        for (size_t j = i + 1; j < objects.size(); j++) {
            auto colB = objects[j]->GetComponent<Collider>();
            if (!colB) continue;
            if (colA->isStatic && colB->isStatic) continue;

            AABB a = colA->GetWorldAABB();
            AABB b = colB->GetWorldAABB();
            if (!Overlaps(a, b)) continue;

            // Penetration depth per axis
            float ox = std::min(a.max.x, b.max.x) - std::max(a.min.x, b.min.x);
            float oy = std::min(a.max.y, b.max.y) - std::max(a.min.y, b.min.y);
            float oz = std::min(a.max.z, b.max.z) - std::max(a.min.z, b.min.z);

            // Resolve along axis with smallest penetration (MTV)
            glm::vec3 mtv(0.0f);
            auto& posA = objects[i]->GetTransform().position;
            auto& posB = objects[j]->GetTransform().position;

            if (ox <= oy && ox <= oz) {
                float sign = (posA.x < posB.x) ? -1.0f : 1.0f;
                mtv = glm::vec3(sign * ox, 0.0f, 0.0f);
            } else if (oy <= oz) {
                float sign = (posA.y < posB.y) ? -1.0f : 1.0f;
                mtv = glm::vec3(0.0f, sign * oy, 0.0f);
            } else {
                float sign = (posA.z < posB.z) ? -1.0f : 1.0f;
                mtv = glm::vec3(0.0f, 0.0f, sign * oz);
            }

            auto rbA = objects[i]->GetComponent<Rigidbody>();
            auto rbB = objects[j]->GetComponent<Rigidbody>();
            bool staticA = colA->isStatic || !rbA;
            bool staticB = colB->isStatic || !rbB;

            glm::vec3 n = glm::normalize(mtv);

            if (!staticA && staticB) {
                posA += mtv;
                float vn = glm::dot(rbA->velocity, n);
                if (vn < 0.0f) rbA->velocity -= n * vn;
            } else if (staticA && !staticB) {
                posB -= mtv;
                float vn = glm::dot(rbB->velocity, -n);
                if (vn < 0.0f) rbB->velocity += n * vn;
            } else if (!staticA && !staticB) {
                posA += mtv * 0.5f;
                posB -= mtv * 0.5f;
                // Simple elastic-ish response
                glm::vec3 relVel = rbA->velocity - rbB->velocity;
                float vn = glm::dot(relVel, n);
                if (vn < 0.0f) {
                    rbA->velocity -= n * vn * 0.5f;
                    rbB->velocity += n * vn * 0.5f;
                }
            }
        }
    }
}
