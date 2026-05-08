# Gameplay Systems

## Scene / GameObject / Component

### Scene
- เก็บ `vector<shared_ptr<GameObject>>`
- `CreateGameObject(name)` → returns shared_ptr
- `DestroyGameObject(obj)` → fires destroy callback แล้ว erase
- `FindGameObject(name)` → search by name
- `SetDestroyCallback` / `ClearDestroyCallback` — ใช้ใน play mode เพื่อ hook Jolt

### GameObject
- มี `Transform` + `vector<shared_ptr<Component>>`
- `SetActive(bool)` — inactive objects ไม่ถูก Update/Render
- Templated API:
  ```cpp
  auto rb = obj->AddComponent<Rigidbody>();
  auto mr = obj->GetComponent<MeshRenderer>();
  bool has = obj->HasComponent<Collider>();
  obj->RemoveComponent<Light>();
  ```
- Forwards collision events ไปทุก Component

### Component (base class)

```cpp
virtual void Update(float deltaTime) {}
virtual void Render(const Camera& camera) {}
virtual void OnCollisionEnter(GameObject* other) {}
virtual void OnCollisionStay(GameObject* other)  {}
virtual void OnCollisionExit(GameObject* other)  {}
```

### Built-in Components

| Component | Fields หลัก |
|---|---|
| `MeshRenderer` | mesh, material (albedo, roughness, metallic, texture, normalMap) |
| `Rigidbody` | mass, useGravity, friction, restitution, velocity |
| `Collider` | size (AABB half-extents), offset, isStatic |
| `Light` | type (Directional/Point), color, intensity, direction, radius |

---

## Transform

```cpp
struct Transform {
    glm::vec3 position = {0,0,0};
    glm::quat rotation = identity;
    glm::vec3 scale    = {1,1,1};

    glm::mat4 GetMatrix() const;  // TRS matrix
};
```

---

## Physics (Jolt)

Library: JoltPhysics (`vendor/JoltPhysics/`)

### Key API

```cpp
JoltPhysicsSystem::Init();
JoltPhysicsSystem::SyncBodiesFromScene(scene);   // build body map at play start
JoltPhysicsSystem::Update(scene, dt);            // step + write transforms back
JoltPhysicsSystem::ClearBodies();
JoltPhysicsSystem::RemoveBody(GameObject* go);   // called by scene destroy callback

JoltPhysicsSystem::CreateCharacter(spawnPos);
JoltPhysicsSystem::MoveCharacter(dt, wishDir, jump);
JoltPhysicsSystem::GetCharacterPosition();       // returns capsule CENTER (not foot)
JoltPhysicsSystem::HasCharacter();
```

### Character Capsule
- `halfHeight=0.8`, `radius=0.3` → total height 2.2 units
- Standing on Y=0 → `GetCharacterPosition().y = 1.1`
- Visual body offset: `charPos - vec3(0, 0.3, 0)` (sits on ground)
- Visual head offset: `charPos + vec3(0, 0.75, 0)`

### Body Lifecycle (Play Mode)
1. `StartPlay` → `SyncBodiesFromScene` (rebuild all bodies)
2. `Scene::DestroyGameObject` → auto calls `RemoveBody` (via callback)
3. `StopPlay` → clear callback → `ClearBodies` → restore scene

### Collision Events
Jolt ContactListener → `GameObject::DispatchCollisionEnter/Stay/Exit` → ส่งต่อไปทุก Component

---

## Camera

Mode: **free spectator** (ไม่ใช่ orbit)

```cpp
// State
glm::vec3 m_Position;
float     m_Yaw, m_Pitch;
glm::vec3 m_Target;  // always = position + GetLookForward()
```

### Methods
```cpp
void LookSpectator(float dyaw, float dpitch);
void MoveSpectator(float fwd, float rgt, float up, float speed);
void SetFollowCamera(glm::vec3 target, float distance, float height); // play mode
void SetPerspective(float fov, float aspect, float near, float far);
glm::vec3 GetLookForward() const; // vec3(cos(yaw)*cos(pitch), sin(pitch), sin(yaw)*cos(pitch))
```

### Controls (Editor Mode)
- RMB + drag → look around (`delta * 0.12`)
- WASD / E+Space / Q+Ctrl → move
- Scroll → fly speed (×1.3 / ×0.77, clamp 0.5–200)
- Shift → speed ×3

### Controls (Play Mode)
- WASD → move character (relative to camera heading, Y=0 projected)
- Space → jump
- RMB + drag → look around

---

## Input

```cpp
Input::Update();                        // call once per frame
Input::IsKeyPressed(GLFW_KEY_W);       // held down
Input::IsKeyDown(GLFW_KEY_F5);         // pressed this frame only
Input::IsMouseButtonPressed(int btn);
Input::GetMousePosition();             // vec2 screen coords
Input::ConsumeScrollDelta();           // returns + clears scroll accumulator
```

GLFW callbacks เซ็ตใน `Input::Init(window)` — ต้อง Init หลัง GLFW window สร้างแล้ว

---

## Scene Serialization

Format: JSON (`nlohmann/json`)

```cpp
SceneSerializer::Save(scene, filepath);          // F5 หรือ Editor > Save
auto scene = SceneSerializer::Load(filepath);    // returns shared_ptr<Scene>
```

Path: `assets/scenes/MainScene.json`

Serialized components: Transform, MeshRenderer (mesh + material + textures), Rigidbody, Collider, Light

Asset paths เก็บ relative to `ASSET_DIR`

GLTF meshes: `filepath::N` (N = primitive index)

---

## GLTF/GLB Import

```cpp
auto objects = GLTFLoader::Import("path/to/model.glb", scene);
auto mesh    = GLTFLoader::LoadMesh("path/to/model.gltf::2");
```

รองรับ:
- Node hierarchy + TRS transforms
- PBR: baseColorFactor, metallic, roughness, baseColorTexture, normalTexture
- Tangents (from GLTF attribute หรือ Gram-Schmidt fallback)
- Embedded GLB textures → export ไป `assets/textures/gltf_cache/`
- Index types: UNSIGNED_BYTE, UNSIGNED_SHORT, UNSIGNED_INT

---

## Play Mode

```
StartPlay:
  1. snapshot scene → JSON string
  2. SyncBodiesFromScene
  3. CreateCharacter (near camera)
  4. spawn __CharBody + __CharHead GameObjects
  5. register destroy callback

StopPlay:
  1. DestroyCharacter
  2. ClearDestroyCallback
  3. ClearBodies
  4. restore scene from snapshot
  5. SyncBodiesFromScene (restored scene)
```

ใน StopPlay ลำดับสำคัญ — clear callback ก่อน ClearBodies เสมอ (ป้องกัน dangling pointer)
