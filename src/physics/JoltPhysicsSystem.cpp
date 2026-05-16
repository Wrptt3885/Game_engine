#include "physics/JoltPhysicsSystem.h"
#include "core/Scene.h"
#include "core/GameObject.h"
#include "physics/Collider.h"
#include "physics/Rigidbody.h"

// Must be first Jolt include in every TU
#include <Jolt/Jolt.h>
JPH_SUPPRESS_WARNINGS

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

#include <cstdarg>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

// -----------------------------------------------------------------
// Object layers: STATIC (non-moving) / DYNAMIC (moving)
// -----------------------------------------------------------------
namespace JoltLayers {
    static constexpr JPH::ObjectLayer STATIC  = 0;
    static constexpr JPH::ObjectLayer DYNAMIC = 1;
    static constexpr JPH::uint        NUM     = 2;
}

namespace JoltBPLayers {
    static constexpr JPH::BroadPhaseLayer STATIC {0};
    static constexpr JPH::BroadPhaseLayer DYNAMIC{1};
    static constexpr JPH::uint            NUM    {2};
}

// Maps object layer → broadphase layer
class BPLayerInterface final : public JPH::BroadPhaseLayerInterface {
public:
    BPLayerInterface() {
        m_Map[JoltLayers::STATIC]  = JoltBPLayers::STATIC;
        m_Map[JoltLayers::DYNAMIC] = JoltBPLayers::DYNAMIC;
    }
    JPH::uint            GetNumBroadPhaseLayers()                       const override { return JoltBPLayers::NUM; }
    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer l)         const override { return m_Map[l]; }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer l) const override {
        return (JPH::BroadPhaseLayer::Type)l == 0 ? "STATIC" : "DYNAMIC";
    }
#endif
private:
    JPH::BroadPhaseLayer m_Map[JoltLayers::NUM];
};

// Determines if object layer can collide with a broadphase layer
class ObjVsBPFilter final : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer obj, JPH::BroadPhaseLayer bp) const override {
        if (obj == JoltLayers::STATIC)  return bp == JoltBPLayers::DYNAMIC;
        if (obj == JoltLayers::DYNAMIC) return true;
        return false;
    }
};

// Determines which object layer pairs can collide
class ObjLayerPairFilter final : public JPH::ObjectLayerPairFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override {
        if (a == JoltLayers::STATIC)  return b == JoltLayers::DYNAMIC;
        if (a == JoltLayers::DYNAMIC) return true;
        return false;
    }
};

// -----------------------------------------------------------------
// Collision event — queued from physics threads, dispatched on main
// -----------------------------------------------------------------
struct CollisionEvent {
    enum class Type : uint8_t { Enter, Stay, Exit } type;
    GameObject* a;
    GameObject* b;
};

// ContactListener — called from Jolt worker threads.
// Buffers events into a mutex-protected queue; main thread flushes
// after each physics step and routes them to component callbacks.
class EngineContactListener final : public JPH::ContactListener {
public:
    void OnContactAdded(const JPH::Body& b1, const JPH::Body& b2,
        const JPH::ContactManifold&, JPH::ContactSettings&) override
    {
        Push(CollisionEvent::Type::Enter, b1.GetID(), b2.GetID());
    }

    void OnContactPersisted(const JPH::Body& b1, const JPH::Body& b2,
        const JPH::ContactManifold&, JPH::ContactSettings&) override
    {
        Push(CollisionEvent::Type::Stay, b1.GetID(), b2.GetID());
    }

    void OnContactRemoved(const JPH::SubShapeIDPair& pair) override {
        Push(CollisionEvent::Type::Exit, pair.GetBody1ID(), pair.GetBody2ID());
    }

    // Call once per frame from the main thread after the physics step.
    std::vector<CollisionEvent> Flush() {
        std::lock_guard<std::mutex> lock(m_Mutex);
        std::vector<CollisionEvent> out;
        out.swap(m_Pending);
        return out;
    }

    // Must be called (main thread) whenever the body map is rebuilt.
    void SetReverseMap(const std::unordered_map<uint32_t, GameObject*>* map) {
        m_ReverseMap = map;
    }

private:
    void Push(CollisionEvent::Type type, JPH::BodyID id1, JPH::BodyID id2) {
        if (!m_ReverseMap) return;
        auto it1 = m_ReverseMap->find(id1.GetIndexAndSequenceNumber());
        auto it2 = m_ReverseMap->find(id2.GetIndexAndSequenceNumber());
        if (it1 == m_ReverseMap->end() || it2 == m_ReverseMap->end()) return;
        std::lock_guard<std::mutex> lock(m_Mutex);
        CollisionEvent ev; ev.type = type; ev.a = it1->second; ev.b = it2->second;
        m_Pending.push_back(ev);
    }

    // Pointer is stable (points into JoltContext) and only written on
    // the main thread between physics steps — safe to read from workers.
    const std::unordered_map<uint32_t, GameObject*>* m_ReverseMap = nullptr;
    std::mutex                   m_Mutex;
    std::vector<CollisionEvent>  m_Pending;
};

// -----------------------------------------------------------------
// Context struct — owns all long-lived Jolt objects
// Destruction order: physicsSystem first (holds refs to filters),
// then jobSystem, tempAlloc, then value-type filters last.
// -----------------------------------------------------------------
struct JoltContext {
    BPLayerInterface                          bpInterface;
    ObjVsBPFilter                             objVsBPFilter;
    ObjLayerPairFilter                        objLayerFilter;
    std::unique_ptr<JPH::TempAllocatorImpl>   tempAlloc;
    std::unique_ptr<JPH::JobSystemThreadPool> jobSystem;
    std::unique_ptr<JPH::PhysicsSystem>       physicsSystem;
    std::unordered_map<GameObject*, JPH::BodyID> bodyMap;
    std::unordered_map<uint32_t, GameObject*>    reverseBodyMap;
    EngineContactListener                        contactListener;
    JPH::Ref<JPH::CharacterVirtual>              character;
};

std::unique_ptr<JoltContext> JoltPhysicsSystem::s_Ctx;

// -----------------------------------------------------------------
// Trace / assert hooks
// -----------------------------------------------------------------
static void JoltTrace(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    std::cout << "[Jolt] " << buf << "\n";
}

#ifdef JPH_ENABLE_ASSERTS
static bool JoltAssertFailed(const char* expr, const char* msg, const char* file, JPH::uint line) {
    std::cerr << file << ":" << line << ": Jolt assert (" << expr << ") " << (msg ? msg : "") << "\n";
    return true; // trigger breakpoint
}
#endif

// -----------------------------------------------------------------
void JoltPhysicsSystem::Init() {
    JPH::RegisterDefaultAllocator();

    JPH::Trace = JoltTrace;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = JoltAssertFailed;)

    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    s_Ctx = std::make_unique<JoltContext>();

    s_Ctx->tempAlloc = std::make_unique<JPH::TempAllocatorImpl>(16 * 1024 * 1024); // 16 MB

    int threads = std::max(1, (int)std::thread::hardware_concurrency() - 1);
    s_Ctx->jobSystem = std::make_unique<JPH::JobSystemThreadPool>(
        JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, threads);

    s_Ctx->physicsSystem = std::make_unique<JPH::PhysicsSystem>();
    s_Ctx->physicsSystem->Init(
        4096, // max bodies
        0,    // body mutex count (0 = auto)
        4096, // max body pairs
        2048, // max contact constraints
        s_Ctx->bpInterface,
        s_Ctx->objVsBPFilter,
        s_Ctx->objLayerFilter);

    s_Ctx->physicsSystem->SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));
    s_Ctx->physicsSystem->SetContactListener(&s_Ctx->contactListener);

    std::cout << "[JoltPhysicsSystem] Initialized (Jolt v"
              << JPH_VERSION_MAJOR << "." << JPH_VERSION_MINOR << "." << JPH_VERSION_PATCH << ")\n";
}

void JoltPhysicsSystem::Shutdown() {
    if (!s_Ctx) return;
    ClearBodies();
    s_Ctx.reset();
    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}

void JoltPhysicsSystem::ClearBodies() {
    if (!s_Ctx) return;
    auto& bi = s_Ctx->physicsSystem->GetBodyInterface();
    for (auto& [go, id] : s_Ctx->bodyMap) {
        bi.RemoveBody(id);
        bi.DestroyBody(id);
    }
    s_Ctx->bodyMap.clear();
    s_Ctx->reverseBodyMap.clear();
}

void JoltPhysicsSystem::SyncBodiesFromScene(Scene& scene) {
    if (!s_Ctx) return;
    ClearBodies();

    auto& bi = s_Ctx->physicsSystem->GetBodyInterface();
    int count = 0;

    for (size_t i = 0; i < scene.GetGameObjectCount(); i++) {
        auto go = scene.GetGameObject(i);
        if (!go) continue;

        auto col = go->GetComponent<Collider>();
        if (!col) continue;

        auto rb  = go->GetComponent<Rigidbody>();
        const Transform& t = go->GetTransform();

        // Half-extents from collider size
        glm::vec3 half = col->size * 0.5f;
        JPH::BoxShapeSettings shapeSettings(JPH::Vec3(half.x, half.y, half.z));
        auto shapeResult = shapeSettings.Create();
        if (shapeResult.HasError()) {
            std::cerr << "[Jolt] Shape error for '" << go->GetName() << "': "
                      << shapeResult.GetError() << "\n";
            continue;
        }

        // World position = transform + collider offset
        glm::vec3 wp = t.position + col->offset;
        JPH::RVec3 pos(wp.x, wp.y, wp.z);
        JPH::Quat  rot(t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w);

        bool isStatic = col->isStatic || !rb;
        JPH::EMotionType motionType = isStatic ? JPH::EMotionType::Static
                                                : JPH::EMotionType::Dynamic;
        JPH::ObjectLayer layer = isStatic ? JoltLayers::STATIC : JoltLayers::DYNAMIC;

        JPH::BodyCreationSettings bcs(shapeResult.Get(), pos, rot, motionType, layer);

        if (rb && !isStatic) {
            bcs.mGravityFactor = rb->useGravity ? 1.0f : 0.0f;
            bcs.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
            bcs.mMassPropertiesOverride.mMass = rb->mass;
            bcs.mLinearVelocity = JPH::Vec3(rb->velocity.x, rb->velocity.y, rb->velocity.z);
            bcs.mFriction    = rb->material.friction;
            bcs.mRestitution = rb->material.restitution;
        }

        JPH::BodyID id = bi.CreateAndAddBody(bcs, JPH::EActivation::Activate);
        if (id.IsInvalid()) {
            std::cerr << "[Jolt] Failed to create body for '" << go->GetName() << "'\n";
            continue;
        }

        s_Ctx->bodyMap[go.get()]                             = id;
        s_Ctx->reverseBodyMap[id.GetIndexAndSequenceNumber()] = go.get();
        count++;
    }

    s_Ctx->contactListener.SetReverseMap(&s_Ctx->reverseBodyMap);
    std::cout << "[JoltPhysicsSystem] Registered " << count << " bodies\n";

    // Ensure static bodies are indexed in the broadphase tree for immediate raycast queries.
    s_Ctx->physicsSystem->OptimizeBroadPhase();
}

void JoltPhysicsSystem::Update(Scene& scene, float dt) {
    if (!s_Ctx) return;

    s_Ctx->physicsSystem->Update(dt, 1, s_Ctx->tempAlloc.get(), s_Ctx->jobSystem.get());

    // Copy dynamic body transforms back to GameObjects
    auto& bi = s_Ctx->physicsSystem->GetBodyInterface();
    for (auto& [go, id] : s_Ctx->bodyMap) {
        if (bi.GetMotionType(id) != JPH::EMotionType::Dynamic) continue;

        JPH::RVec3 pos = bi.GetPosition(id);
        JPH::Quat  rot = bi.GetRotation(id);

        auto col = go->GetComponent<Collider>();
        glm::vec3 offset = col ? col->offset : glm::vec3(0.0f);

        Transform& t = go->GetTransform();
        t.position = glm::vec3((float)pos.GetX(), (float)pos.GetY(), (float)pos.GetZ()) - offset;
        t.rotation = glm::quat(rot.GetW(), rot.GetX(), rot.GetY(), rot.GetZ());
    }

    // Dispatch collision events collected during the physics step
    for (auto& ev : s_Ctx->contactListener.Flush()) {
        if (!ev.a || !ev.b) continue;
        switch (ev.type) {
            case CollisionEvent::Type::Enter:
                ev.a->DispatchCollisionEnter(ev.b);
                ev.b->DispatchCollisionEnter(ev.a);
                break;
            case CollisionEvent::Type::Stay:
                ev.a->DispatchCollisionStay(ev.b);
                ev.b->DispatchCollisionStay(ev.a);
                break;
            case CollisionEvent::Type::Exit:
                ev.a->DispatchCollisionExit(ev.b);
                ev.b->DispatchCollisionExit(ev.a);
                break;
        }
    }

    (void)scene;
}

// -----------------------------------------------------------------
// Character (Play Mode)
// -----------------------------------------------------------------

void JoltPhysicsSystem::CreateCharacter(const glm::vec3& spawnPos) {
    if (!s_Ctx) return;

    // Capsule: halfHeight=0.8, radius=0.3 → total height 2.2 units
    auto shapeResult = JPH::CapsuleShapeSettings(0.8f, 0.3f).Create();
    if (shapeResult.HasError()) {
        std::cerr << "[Jolt] Character shape error: " << shapeResult.GetError() << "\n";
        return;
    }

    JPH::CharacterVirtualSettings settings;
    settings.mMaxSlopeAngle     = JPH::DegreesToRadians(45.0f);
    settings.mShape             = shapeResult.Get();
    settings.mSupportingVolume  = JPH::Plane(JPH::Vec3::sAxisY(), -0.3f);

    JPH::RVec3 pos(spawnPos.x, spawnPos.y, spawnPos.z);
    s_Ctx->character = new JPH::CharacterVirtual(
        &settings, pos, JPH::Quat::sIdentity(), s_Ctx->physicsSystem.get());

    std::cout << "[JoltPhysicsSystem] Character created at ("
              << spawnPos.x << ", " << spawnPos.y << ", " << spawnPos.z << ")\n";
}

void JoltPhysicsSystem::DestroyCharacter() {
    if (s_Ctx) s_Ctx->character = nullptr;
}

bool JoltPhysicsSystem::HasCharacter() {
    return s_Ctx && s_Ctx->character != nullptr;
}

bool JoltPhysicsSystem::IsCharacterOnGround() {
    if (!HasCharacter()) return false;
    return s_Ctx->character->GetGroundState() ==
           JPH::CharacterBase::EGroundState::OnGround;
}

bool JoltPhysicsSystem::RaycastGround(const glm::vec3& from, float maxDist, float& outHitY) {
    if (!s_Ctx) return false;

    // Shoot a ray straight down from 'from' by maxDist units.
    JPH::RRayCast ray{
        JPH::RVec3(from.x, from.y, from.z),
        JPH::Vec3(0.0f, -maxDist, 0.0f)
    };

    JPH::RayCastResult hit;
    // Default-constructed filters accept all broadphase layers and object layers.
    bool found = s_Ctx->physicsSystem->GetNarrowPhaseQuery()
        .CastRay(ray, hit, JPH::BroadPhaseLayerFilter{},
                             JPH::ObjectLayerFilter{});

    if (found) {
        // Hit point Y = from.y + direction.y * fraction = from.y - maxDist * fraction
        outHitY = from.y - maxDist * hit.mFraction;
        return true;
    }
    return false;
}

void JoltPhysicsSystem::SnapCharacterToGround(float targetY) {
    if (!HasCharacter()) return;
    JPH::RVec3 p = s_Ctx->character->GetPosition();
    s_Ctx->character->SetPosition(JPH::RVec3(p.GetX(), targetY, p.GetZ()));
}

glm::vec3 JoltPhysicsSystem::GetCharacterPosition() {
    if (!HasCharacter()) return glm::vec3(0.0f);
    JPH::RVec3 p = s_Ctx->character->GetPosition();
    return glm::vec3((float)p.GetX(), (float)p.GetY(), (float)p.GetZ());
}

void JoltPhysicsSystem::RemoveBody(GameObject* go) {
    if (!s_Ctx || !go) return;
    auto it = s_Ctx->bodyMap.find(go);
    if (it == s_Ctx->bodyMap.end()) return;
    JPH::BodyID id = it->second;
    auto& bi = s_Ctx->physicsSystem->GetBodyInterface();
    bi.RemoveBody(id);
    bi.DestroyBody(id);
    s_Ctx->reverseBodyMap.erase(id.GetIndexAndSequenceNumber());
    s_Ctx->bodyMap.erase(it);
}

void JoltPhysicsSystem::MoveCharacter(float dt, const glm::vec3& wishDir, bool jump) {
    if (!HasCharacter()) return;
    auto& ch = *s_Ctx->character;

    const float walkSpeed = 5.0f;
    const float jumpSpeed = 6.0f;

    JPH::Vec3 curVel  = ch.GetLinearVelocity();
    bool onGround = ch.GetGroundState() == JPH::CharacterBase::EGroundState::OnGround;

    // Horizontal: override each frame from input
    float velY = onGround ? 0.0f : curVel.GetY();
    if (onGround && jump) velY = jumpSpeed;

    ch.SetLinearVelocity(JPH::Vec3(
        wishDir.x * walkSpeed,
        velY,
        wishDir.z * walkSpeed));

    // ExtendedUpdate handles gravity, step-up, stick-to-floor
    JPH::CharacterVirtual::ExtendedUpdateSettings ext;
    JPH::BroadPhaseLayerFilter bpFilter;
    JPH::ObjectLayerFilter     objFilter;
    JPH::BodyFilter            bodyFilter;
    JPH::ShapeFilter           shapeFilter;

    ch.ExtendedUpdate(dt,
        JPH::Vec3(0.0f, -9.81f, 0.0f),
        ext,
        bpFilter, objFilter, bodyFilter, shapeFilter,
        *s_Ctx->tempAlloc);
}
