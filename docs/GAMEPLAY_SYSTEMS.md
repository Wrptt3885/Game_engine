# Gameplay Systems

## Scene / GameObject / Component

### Scene
- เก็บ `vector<shared_ptr<GameObject>>`
- `CreateGameObject(name)` → returns shared_ptr
- `DestroyGameObject(obj)` → fires destroy callback แล้ว erase
- `FindGameObject(name)` → search by name
- `SetDestroyCallback` / `ClearDestroyCallback` — ใช้ใน play mode เพื่อ hook PhysicsWorld

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
virtual void OnGUI() {}
virtual void OnCollisionEnter(GameObject* other) {}
virtual void OnCollisionStay(GameObject* other)  {}
virtual void OnCollisionExit(GameObject* other)  {}
```

### Built-in Components

| Component | หน้าที่ |
|---|---|
| `MeshRenderer` | วาด static mesh + material PBR |
| `SkinnedMeshRenderer` | วาด skinned mesh + GPU skinning + animation state machine |
| `Rigidbody` | physics body — mass, material (friction/restitution), gravity, velocity |
| `Collider` | AABB shape ส่งให้ PhysicsWorld |
| `Light` | Directional / Point light |
| `LuaScript` | รัน Lua script; Awake/Update hooks |
| `AudioSource` | เล่นเสียง 2D/3D positional |
| `AudioListener` | กำหนดตำแหน่งหูฟังในโลก |
| `UILabel` | แสดงข้อความ screen-space |
| `UIImage` | แสดงรูปภาพ screen-space |

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

## Physics

Library: **JoltPhysics** (`vendor/JoltPhysics/`)  
Public API: **`PhysicsWorld`** — Application ใช้เพียง PhysicsWorld เท่านั้น (JoltPhysicsSystem เป็น internal implementation)

### PhysicsWorld API

```cpp
m_PhysicsWorld.Init();
m_PhysicsWorld.Shutdown();
m_PhysicsWorld.SyncFromScene(scene);       // build body map at play start
m_PhysicsWorld.Update(scene, dt);          // step + write transforms back
m_PhysicsWorld.RemoveBody(GameObject* go); // called by scene destroy callback
m_PhysicsWorld.ClearBodies();

m_PhysicsWorld.CreateCharacter(spawnPos);
m_PhysicsWorld.MoveCharacter(dt, wishDir, jump);
m_PhysicsWorld.GetCharacterPosition();     // returns capsule CENTER (not foot)
m_PhysicsWorld.HasCharacter();
```

### PhysicsMaterial

ฝังอยู่ใน `Rigidbody::material` — กำหนด physical + optical properties ของ object:

```cpp
struct PhysicsMaterial {
    float density      = 1000.0f;
    float friction     = 0.2f;
    float restitution  = 0.0f;
    float hardness     = 1.0f;
    float ior          = 1.0f;      // index of refraction
    float transparency = 0.0f;
    float absorbance   = 0.0f;
    Phase phase        = Solid;     // Solid | Liquid | Gas
};
```

**Presets**: Default, Metal, Wood, Rubber, Ice, Glass, Water, Stone — เลือกได้จาก Inspector dropdown

### Rigidbody Fields

| Field | Default | หมายเหตุ |
|---|---|---|
| `material` | Default preset | PhysicsMaterial (friction, restitution, ฯลฯ) |
| `mass` | 1.0f | kg |
| `useGravity` | true | |
| `velocity` | vec3(0) | read/write |

### Character Capsule
- `halfHeight=0.8`, `radius=0.3` → total height 2.2 units
- Standing on Y=0 → `GetCharacterPosition().y = 1.1`
- Visual body offset: `charPos - vec3(0, 0.3, 0)` (sits on ground)
- Visual head offset: `charPos + vec3(0, 0.75, 0)`

### Body Lifecycle (Play Mode)
1. `StartPlay` → `SyncFromScene` (rebuild all bodies)
2. `Scene::DestroyGameObject` → auto calls `RemoveBody` (via callback)
3. `StopPlay` → clear callback → `ClearBodies` → restore scene

### Collision Events
Jolt ContactListener → `GameObject::DispatchCollisionEnter/Stay/Exit` → ส่งต่อไปทุก Component

---

## Audio

Library: **miniaudio** (`vendor/miniaudio.h`, single-header)

### AudioManager (static)

```cpp
AudioManager::Init();
AudioManager::Shutdown();
AudioManager::SetListenerPosition(pos, forward, up);
AudioManager::GetEngine();   // ma_engine*
AudioManager::IsReady();
```

`Init()` สร้าง `ma_engine` ที่ใช้ default device  
`SetListenerPosition()` ส่งต่อไป `ma_engine_listener_set_position/direction/world_up`

### AudioSource Component

Fields:
| Field | Default | หมายเหตุ |
|---|---|---|
| `clipPath` | "" | path to audio file (.wav/.mp3/.ogg/.flac) |
| `volume` | 1.0f | 0–2 |
| `pitch` | 1.0f | 0.5–2 |
| `loop` | false | |
| `playOnStart` | false | เล่นทันทีเมื่อ play mode เริ่ม |
| `spatial` | true | 3D positional audio |
| `minDistance` | 1.0f | ระยะที่เสียงดังสุด |
| `maxDistance` | 50.0f | ระยะที่เสียงเบาสุด |

API:
```cpp
as->Play();    // load lazily แล้วเล่น (seek to 0 ก่อน)
as->Pause();
as->Stop();    // stop + seek to frame 0
as->IsPlaying() const;
as->OnPlayStart();  // เรียกโดย Application ตอน StartPlay (ถ้า playOnStart=true)
```

`Update(dt)` — sync `ma_sound_set_position` กับ Transform ทุก frame (เฉพาะ spatial=true)

### AudioListener Component

`Update(dt)` — ดึง forward/up จาก `Transform::rotation` quaternion แล้วเรียก `AudioManager::SetListenerPosition`

**Fallback**: ถ้าไม่มี AudioListener component ที่ active, Application ใช้ camera position + forward แทน

### 3D Spatial Audio

miniaudio ใช้ distance attenuation linear:
- เสียงดังสุดที่ `minDistance`
- เสียงเบาสุดที่ `maxDistance`
- `ma_sound_set_position` อัปเดตทุก frame

---

## Lua Scripting

Library: **sol2** + **Lua 5.4** (ผ่าน FetchContent)

### LuaScript Component

```cpp
ls->scriptPath = "assets/scripts/myscript.lua";
ls->Reload();   // hot-reload (re-execute script)
```

Hooks ที่ Lua script implement ได้:
```lua
function Awake()  end   -- เรียกครั้งเดียวตอน play start
function Update(dt) end  -- เรียกทุก frame ขณะ play mode
```

### Bindings ที่มีให้ใน Lua

**Transform:**
```lua
self.transform.position      -- vec3 read/write
self.transform.rotation      -- quat read/write
self.transform.scale         -- vec3 read/write
```

**Input:**
```lua
Input.IsKeyPressed(key)          -- GLFW key constant
Input.IsMouseButtonPressed(btn)
```

**GameObject:**
```lua
GameObject.Find("name")         -- returns GameObject or nil
GameObject.Destroy(obj)
obj:SetActive(bool)
```

**Time:**
```lua
Time.deltaTime
Time.totalTime
```

### Play Mode Control

`LuaManager::SetPlayMode(true/false)` — enable/disable script execution  
`LuaManager::ReloadAll()` — เรียก Awake() บนทุก LuaScript (ตอน StartPlay)

---

## Skeletal Animation

### SkinnedMeshRenderer

```cpp
smr->SetSkeleton(skel);                        // Skeleton + inverse bind matrices
smr->SetClips(clips);                          // vector<AnimationClip>; clip 0 = current
smr->PlayClip(int index);                      // switch by index
smr->PlayClip(const string& name);             // switch by name
smr->AddClipsFromFile(path, srcSkel, clips);   // append + remap joints by name
smr->EvaluateRules(moving, sprinting, jumping); // auto-switch via AnimRule table
smr->GetBoneMatrices();                        // const ref — used by shadow pass
smr->GetPhysicsYOffset();                      // vertical offset mesh→capsule center (default −1.1)
```

### AnimRule (state machine)

Top-to-bottom evaluation, first match wins:

| Trigger | Condition |
|---|---|
| Idle | not moving, not jumping (fallback) |
| Moving | moving, not sprinting |
| Sprinting | moving + shift |
| Jumping | space pressed |

Fallback: ถ้าไม่มี rule match → play clip 0

### Keyframe Interpolation

- Translation / Scale → `SampleVec3` — linear lerp ระหว่าง keyframes
- Rotation → `SampleQuat` — slerp ระหว่าง keyframes
- Animation เวลา loop: `fmod(t, duration)`

### Shadow Pass Skinning

`RendererShadow` อัปโหลด bone matrices (`u_BoneMatrices[100]`, `u_UseSkinning`) ให้ depth shader — ทั้ง OpenGL (`depth.vert`) และ DX11 (`depth.hlsl`) support skinning

---

## Asset Import

### OBJ

```cpp
auto objects = OBJLoader::Load("path/to/model.obj", scene);
```
- สร้าง 1 GameObject
- คำนวณ tangent + bitangent ต่อ triangle (Gram-Schmidt)

### GLTF/GLB

```cpp
auto objects = GLTFLoader::Import("path/to/model.glb", scene);
auto mesh    = GLTFLoader::LoadMesh("path/to/model.gltf::2");
```

รองรับ:
- Node hierarchy + TRS transforms
- PBR: baseColorFactor, metallic, roughness, baseColorTexture, normalTexture
- Tangents (จาก GLTF attribute หรือ Gram-Schmidt fallback)
- Embedded GLB textures → export ไป `assets/textures/gltf_cache/`
- Index types: UNSIGNED_BYTE, UNSIGNED_SHORT, UNSIGNED_INT
- Skeletal animation + skinning

### FBX

```cpp
auto objects = FBXLoader::Import("path/to/model.fbx", scene);
auto mesh    = FBXLoader::LoadMesh("path/to/model.fbx::0");
auto data    = FBXLoader::LoadSkinnedMesh("path/to/model.fbx::0");
auto bundle  = FBXLoader::LoadClipsFromFile("path/to/anim.fbx");
```

Library: **ufbx** (ดาวน์โหลดผ่าน CMake FetchContent, compiled as C)

- Y-up right-handed + unit normalize ไป meters
- Skinned mesh: reads skin deformer clusters → Skeleton + clips sampled 30fps
- Textures: relative path ก่อน แล้ว absolute path ที่เก็บใน FBX

---

## In-Game UI

Renders ผ่าน **ImGui background draw list** — แสดงบน 3D viewport โดยตรง

### UILayer

`Scene::GetUILayer()` — เก็บ `vector<shared_ptr<UIElement>>`

### UIElement (base)
- `pos` (screen coords), `anchor` (9 points), `visible`
- Anchor ใช้คำนวณ `pos` สัมพัทธ์กับขอบหน้าจอ

### UILabel
```cpp
label->text     = "Score: 0";
label->fontSize = 24.0f;
label->color    = {1,1,1,1};
label->centered = true;
label->anchor   = UIAnchor::TopCenter;
label->offset   = {0, 20};
```

### UIImage
```cpp
img->texture = Texture::Create("assets/textures/hud.png");
img->size    = {128, 128};
img->tint    = {1,1,1,1};
img->anchor  = UIAnchor::BottomRight;
img->offset  = {-10, -10};
```

### OnGUI Hook
`Component::OnGUI()` — เรียกหลัง 3D render ทุก frame (ใช้ ImGui draw list วาดใส่ background layer)

---

## Scene Serialization

Format: JSON (`nlohmann/json`)

```cpp
SceneSerializer::Save(scene, filepath);
auto scene = SceneSerializer::Load(filepath);
```

Path: `assets/scenes/MainScene.json`

Serialized components:
| Component | Fields |
|---|---|
| Transform | position, rotation (quat), scale |
| MeshRenderer | meshPath, material (albedo, roughness, metallic, textures, worldUV) |
| SkinnedMeshRenderer | meshPath |
| Rigidbody | mass, useGravity, velocity, material (friction, restitution, density, hardness, ior, transparency, absorbance, phase) |
| Collider | size, offset, isStatic |
| Light | type, color, intensity, direction, radius |
| LuaScript | scriptPath |
| AudioSource | clipPath, volume, pitch, loop, playOnStart, spatial, minDistance, maxDistance |
| AudioListener | (no fields) |
| UILabel | text, fontSize, color, centered, anchor, offset |
| UIImage | texturePath, size, tint, anchor, offset |

Asset paths เก็บ relative to `ASSET_DIR`  
GLTF/FBX meshes: `filepath::N` (N = primitive index)

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

## Play Mode

```
StartPlay:
  1. snapshot scene → JSON string
  2. PhysicsWorld::SyncFromScene (rebuild all bodies)
  3. Find first SkinnedMeshRenderer → m_CharacterMesh
  4. CreateCharacter (at character mesh position)
  5. RaycastGround → SnapCharacterToGround (prevent floating)
  6. register destroy callback → PhysicsWorld::RemoveBody
  7. LuaManager::SetPlayMode(true) + ReloadAll() (= Awake on every LuaScript)
  8. OnPlayStart on every AudioSource (playOnStart=true → Play())

StopPlay:
  1. Stop all AudioSource components
  2. LuaManager::SetPlayMode(false)
  3. DestroyCharacter
  4. ClearDestroyCallback
  5. PhysicsWorld::ClearBodies
  6. restore scene from JSON snapshot
  7. PhysicsWorld::SyncFromScene (restored scene)
```

ลำดับสำคัญ:
- Clear callback **ก่อน** ClearBodies (ป้องกัน dangling pointer)
- Audio stop **ก่อน** restore scene (ป้องกัน ma_sound ชี้ไป unloaded object)
