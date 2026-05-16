# MyEngine — CLAUDE.md

Quick-start reference for AI-assisted development on this project. Read this before touching any code.

---

## Build System

**Toolchain**: CMake + MinGW-w64 (MSYS2)  
**C++ standard**: C++17  
**Build directory**: `build/`

### Build commands (PowerShell from repo root)

```powershell
# CRITICAL: prepend MSYS2 to PATH first — Git's mingw64 conflicts and causes STATUS_DLL_NOT_FOUND
$env:PATH = "C:\msys64\mingw64\bin;C:\msys64\usr\bin;" + $env:PATH

# OpenGL backend (default)
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
mingw32-make -j8

# DX11 backend
cmake -G "MinGW Makefiles" -DUSE_DX11=ON -DCMAKE_BUILD_TYPE=Release ..
mingw32-make -j8
```

> **Important**: CMake uses `file(GLOB_RECURSE)` to collect sources. Adding a new `.cpp` file requires re-running `cmake ..` before `mingw32-make` or it won't be compiled.

### Key compile definitions
| Definition | Value | Purpose |
|---|---|---|
| `SHADER_DIR` | `<repo>/shaders` | Absolute path to shaders at runtime |
| `ASSET_DIR` | `<repo>/assets` | Absolute path to assets at runtime |
| `USE_DX11_BACKEND` | (defined when USE_DX11=ON) | Switches all `#ifdef` blocks |
| `GLM_FORCE_DEPTH_ZERO_TO_ONE` | (DX11 only) | GLM projection uses [0,1] depth range |
| `GLM_ENABLE_EXPERIMENTAL` | always | Enables glm/gtx extensions |

**Never** use `/dev/null` — this is Windows. Use `$null` or `NUL` in PowerShell.

---

## Directory Structure

```
Game_engine/
├── CMakeLists.txt
├── CLAUDE.md               ← this file
├── include/                ← all headers (no .cpp here)
│   ├── core/               Application, Scene, GameObject, Component, Transform,
│   │                       Camera, SceneSerializer
│   ├── editor/             EditorLayer
│   ├── audio/              AudioManager, AudioSource, AudioListener
│   ├── physics/            PhysicsWorld, PhysicsMaterial, IPhysicsSystem,
│   │                       JoltPhysicsSystem, Rigidbody, Collider
│   ├── renderer/           Renderer, Mesh, MeshRenderer, SkinnedMeshRenderer,
│   │                       Skeleton, AnimationClip, Shader, Texture, Material,
│   │                       Light, LightData, ShadowMap, Skybox,
│   │                       OBJLoader, GLTFLoader, FBXLoader
│   ├── scripting/          LuaManager, LuaScript
│   ├── ui/                 UICanvas, UILayer, UIElement, UILabel, UIImage, UITypes
│   ├── platform/           Input
│   └── rhi/
│       ├── RHI.h           GraphicsAPI enum + static Init/GetAPI
│       ├── dx11/           DX11Context, DX11Mesh, DX11Shader, DX11ShadowMap,
│       │                   DX11Skybox, DX11Texture
│       └── opengl/         GLMesh, GLShader, GLTexture
├── src/
│   ├── main.cpp            entry point
│   ├── core/               Application*.cpp, Scene, Camera, GameObject,
│   │                       SceneSerializer, Transform
│   ├── editor/             EditorLayer, EditorMenuBar, EditorHierarchy,
│   │                       EditorInspector, EditorGizmo, EditorDialogs
│   ├── audio/              AudioManager, AudioSource, AudioListener, audio_impl.cpp
│   ├── physics/            JoltPhysicsSystem, PhysicsWorld, Rigidbody, Collider
│   ├── renderer/           Renderer*.cpp, OBJLoader, GLTFLoader, FBXLoader,
│   │                       MeshRenderer, SkinnedMeshRenderer, ShadowMap, Skybox
│   ├── scripting/          LuaManager, LuaScript
│   ├── ui/                 UICanvas, UILayer, UILabel, UIImage
│   ├── rhi/dx11/           DX11Context, DX11Mesh, DX11Shader, DX11ShadowMap,
│   │                       DX11Skybox, DX11Texture
│   ├── rhi/opengl/         GLMesh, GLShader, GLTexture
│   └── stb_image.cpp       STB_IMAGE_IMPLEMENTATION + STB_IMAGE_WRITE_IMPLEMENTATION
├── shaders/
│   ├── standard/           default.vert / default.frag   (OpenGL main pass)
│   ├── depth/              depth.vert / depth.frag        (OpenGL shadow pass)
│   ├── skybox/             skybox.vert / skybox.frag      (OpenGL skybox)
│   └── dx11/               standard.hlsl, depth.hlsl, skybox.hlsl
├── assets/
│   ├── models/             cube.obj, plane.obj, ...
│   ├── textures/
│   │   └── gltf_cache/     auto-exported PNG textures from embedded GLB imports
│   └── scenes/             MainScene.json (serialized via F5)
└── vendor/
    ├── imgui/              Dear ImGui
    ├── imguizmo/           ImGuizmo
    ├── tinygltf/           tiny_gltf.h (header-only GLTF/GLB parser)
    ├── miniaudio.h         single-header audio library (miniaudio)
    ├── stb_image.h
    ├── stb_image_write.h   used by GLTFLoader to export embedded GLB textures
    └── (ufbx downloaded via FetchContent → build/_deps/ufbx-src/ufbx.h)
```

---

## Architecture

### Startup sequence

```
main.cpp
  └─ Application()
       ├─ InitWindow()        GLFW window; DX11: also DX11Context::Init(hwnd)
       ├─ InitRenderer()      Renderer::Init() — shaders, shadow map, skybox, RS states
       ├─ InitInput()         Input::Init(window) — installs GLFW callbacks
       ├─ m_Editor.Init()     ImGui init (must come after Input so callbacks chain)
       ├─ m_PhysicsWorld.Init()
       ├─ AudioManager::Init()
       ├─ LuaManager::Init()
       └─ InitScene()         create Camera, Scene, default GameObjects
```

### Frame loop

```
Run()
  ProcessInput(dt)   → Input::Update, WASD/mouse-look, scroll speed, F5/F11
  Update(dt)         → floor follows camera XZ (edit only), play/stop requests,
                        character movement → m_PhysicsWorld.MoveCharacter/Update,
                        audio listener sync (AudioListener component or camera fallback)
  Render()           → ShadowPass → Clear → DrawSkybox → Scene::Render
                        → Editor::BeginFrame/Render/EndFrame
  DX11Context::Present / glfwSwapBuffers
  glfwPollEvents
```

### Scene / GameObject / Component

- `Scene` owns `vector<shared_ptr<GameObject>>`
- `GameObject` owns `vector<shared_ptr<Component>>` + `Transform`
- `Component` base: `virtual void Update(float dt)`, `virtual void Render(Camera&)`
- Templated helpers: `AddComponent<T>()`, `GetComponent<T>()`, `HasComponent<T>()`
- `Scene::DestroyGameObject` fires an optional `m_OnDestroy` callback before erasing — used in play mode to remove Jolt bodies
- `Scene::SetDestroyCallback` / `ClearDestroyCallback` — wired in `StartPlay` / `StopPlay`

### RHI / Factory pattern

Abstract classes (`Mesh`, `Shader`, `Texture`) have static `Create()` factories:

```cpp
auto mesh    = Mesh::Create(vertices, indices);
auto shader  = Shader::Create(vsPath, fsPath);
auto texture = Texture::Create(imagePath);
auto tex     = Texture::CreateFromMemory(w, h, rgbaPtr, "name"); // from pixel data
```

All DX11 implementations live in `rhi/dx11/`, OpenGL in `rhi/opengl/`.

### Rendering pipeline

1. **Shadow pass** (`Renderer::ShadowPass`) — 2048×2048 depth texture; shadow RS uses `CULL_FRONT` to prevent peter-panning
2. **Main pass** (`Scene::Render → Renderer::BeginScene + DrawMesh`) — PBR-lite: hemisphere ambient + Blinn-Phong; up to 4 directional + 8 point lights; 16-sample Poisson PCF shadows; normal mapping; world-space UV tiling
3. **Skybox** (`Renderer::DrawSkybox`) — procedural gradient sky; depth = LESS_EQUAL

---

## Camera

**Mode**: free spectator (not orbit).

| State | Meaning |
|---|---|
| `m_Position` | world position |
| `m_Yaw` / `m_Pitch` | look-direction angles |
| `m_Target` | = `m_Position + GetLookForward()` — always recomputed |

**Controls**:
- RMB + drag → `LookSpectator(dx*0.12, -dy*0.12)`
- WASD / Space+E / Ctrl+Q → `MoveSpectator`
- Scroll wheel → `m_FlySpeed` (×1.3 / ×0.77, clamped 0.5–200); Shift = ×3
- F5 → save scene; F11 → fullscreen; Escape → stop play / quit

**Play mode camera** — third-person follow via `Camera::SetFollowCamera(eye, distance, height)`.

`GetLookForward()`:
```cpp
vec3(cos(yaw)*cos(pitch),  sin(pitch),  sin(yaw)*cos(pitch))
```

> Do **not** call `SyncOrbitState()` from `SetPosition` or `SetTarget` — it corrupts spectator yaw/pitch by computing angles from the orbit direction (backward vector).

---

## Physics (Jolt)

Powered by **JoltPhysics** (`vendor/JoltPhysics/`).

### Architecture

```
PhysicsWorld        — coordinator; the only public API callers should use
  └─ JoltPhysicsSystem  — Jolt rigid body backend (static class)
PhysicsMaterial     — per-object material: friction, restitution, density,
                       hardness, ior, transparency, absorbance, Phase (Solid/Liquid/Gas)
IPhysicsSystem      — interface for future backends (FluidSystem, SoftBodySystem)
```

`Application` holds `PhysicsWorld m_PhysicsWorld` — all physics calls go through it.

### Key API (via PhysicsWorld)
```cpp
m_PhysicsWorld.Init();
m_PhysicsWorld.SyncFromScene(scene);      // build body map from Collider/Rigidbody
m_PhysicsWorld.Update(scene, dt);         // step + write transforms back
m_PhysicsWorld.ClearBodies();
m_PhysicsWorld.RemoveBody(GameObject* go);
m_PhysicsWorld.CreateCharacter(spawnPos);
m_PhysicsWorld.MoveCharacter(dt, wishDir, jump);
m_PhysicsWorld.GetCharacterPosition();
m_PhysicsWorld.RaycastGround(from, maxDist, outHitY);
m_PhysicsWorld.SnapCharacterToGround(targetY);
```

### Rigidbody fields
| Field | Default | Notes |
|---|---|---|
| `material.friction` | 0.5f | 0–1 (was `friction` flat field) |
| `material.restitution` | 0.0f | 0–1 bounciness (was `restitution` flat field) |
| `material.density` | 1000.0f | kg/m³ |
| `material.phase` | Solid | routes to correct simulation backend |
| `mass` | 1.0f | kg — direct field |
| `useGravity` | true | direct field |
| `velocity` | vec3(0) | direct field |

Inspector shows preset dropdown (Default/Metal/Wood/Rubber/Ice/Glass/Water/Stone) and Optical section (IOR, transparency, absorbance, phase).

### Character capsule geometry
Jolt `CharacterVirtual`: `halfHeight=0.8`, `radius=0.3` → total height 2.2 units.  
`GetCharacterPosition()` returns the **capsule center** (1.1 above ground when standing), not the foot.  
`yOff = smr->GetPhysicsYOffset()` (default −1.1); spawn position = `meshPos.y - yOff` = `meshPos.y + 1.1`.

### Body lifecycle
- `SyncFromScene` rebuilds the entire body map from scratch (called at play start).
- `Scene::DestroyGameObject` calls `m_PhysicsWorld.RemoveBody` automatically in play mode (via destroy callback).
- `StopPlay` clears the callback **before** `ClearBodies` to avoid iterating stale pointers.

---

## Play Mode

Toggled with the Play/Stop button in the editor menu bar, or **Escape** key.

**StartPlay** (`ApplicationPlayMode.cpp`):
1. Snapshots scene to JSON string
2. `m_PhysicsWorld.SyncFromScene` (also calls `OptimizeBroadPhase` internally)
3. Finds first `SkinnedMeshRenderer` in scene → `m_CharacterMesh` (player visual)
4. Creates `CharacterVirtual` capsule at character mesh position
5. Raycasts straight down 500 units → `SnapCharacterToGround` to prevent floating
6. Registers destroy callback so `DestroyGameObject` → `m_PhysicsWorld.RemoveBody`
7. Calls `AudioSource::OnPlayStart()` on every object (loads + auto-plays if `playOnStart`)
8. Enables Lua play mode + calls `Reload()` (= Awake) on every `LuaScript`

**StopPlay**:
1. Stops all AudioSources
2. Disables Lua play mode, destroys character, clears callback, clears all bodies
3. Restores scene from JSON snapshot + re-syncs physics

**Play controls**: WASD = move character (relative to camera heading); Space = jump; RMB + drag = look around. Hierarchy delete / Inspector editing disabled during play.

---

## Audio (miniaudio)

`vendor/miniaudio.h` — single-header library. Implementation TU: `src/audio/audio_impl.cpp`.

### Key classes
| Class | File | Responsibility |
|---|---|---|
| `AudioManager` | `include/audio/AudioManager.h` | Init/Shutdown `ma_engine`; update listener transform |
| `AudioSource` | `include/audio/AudioSource.h` | Component: clip path, volume, pitch, loop, spatial 3D |
| `AudioListener` | `include/audio/AudioListener.h` | Component: syncs GameObject transform → listener each frame |

### AudioSource fields
| Field | Default | Notes |
|---|---|---|
| `clipPath` | "" | absolute path to .wav/.mp3/.ogg/.flac |
| `volume` | 1.0f | 0–1 |
| `pitch` | 1.0f | 0.1–4 |
| `loop` | false | |
| `playOnStart` | false | auto-play when StartPlay() |
| `spatial` | true | 3D positional attenuation |
| `minDistance` | 1.0f | full volume within this radius |
| `maxDistance` | 50.0f | silent beyond this |

### API
```cpp
as->Play();   // load (lazy) + seek to 0 + start
as->Pause();  // stop without seeking
as->Stop();   // stop + seek to 0
as->IsPlaying();
```

### 3D audio
- `AudioSource::Update()` syncs `ma_sound_set_position` every frame when `spatial=true`
- Listener: if scene has an `AudioListener` component on any active object, its `Update()` calls `AudioManager::SetListenerPosition`. If none exists, camera position + forward is used as fallback.
- Rolloff model: linear (`ma_sound_set_rolloff(1.0f)`)

### Lifecycle in play mode
- `StartPlay` → `OnPlayStart()` on all AudioSources (loads sound; plays if `playOnStart`)
- `StopPlay` → `Stop()` on all AudioSources before scene restore

---

## Skeletal Animation

`SkinnedMeshRenderer` (`include/renderer/SkinnedMeshRenderer.h`) handles GPU skinning + animation state.

### Key API
```cpp
smr->SetSkeleton(skel);               // shared_ptr<Skeleton> — joint hierarchy + IBMs
smr->SetClips(clips);                 // vector<AnimationClip> — sets clip 0 as current
smr->PlayClip(int index);             // switch clip by index
smr->PlayClip(const string& name);    // switch clip by name
smr->AddClipsFromFile(path, srcSkel, clips); // append clips with joint remapping
smr->EvaluateRules(moving, sprinting, jumping); // auto-switch via AnimRule table
smr->GetBoneMatrices();               // const ref — used by shadow pass
smr->GetPhysicsYOffset();             // vertical offset mesh→capsule center (default −1.1)
```

### AnimRule
Table-driven animation state machine. Rules are evaluated top-to-bottom; first match wins.
| Trigger | Condition |
|---|---|
| `Idle` | not moving, not jumping (acts as fallback) |
| `Moving` | moving, not sprinting |
| `Sprinting` | moving + shift |
| `Jumping` | space pressed |

### Shadow pass skinning
`RendererShadow.cpp` checks each object for `SkinnedMeshRenderer` and uploads bone matrices to the depth shader (`u_BoneMatrices[100]`, `u_UseSkinning`). Both OpenGL (`depth.vert`) and DX11 (`depth.hlsl`) support skinning.

### Vertex attributes for skinning
| Location | Semantic | Type |
|---|---|---|
| 5 | `aBoneIds` | `uvec4` / `BLENDINDICES` |
| 6 | `aBoneWeights` | `vec4` / `BLENDWEIGHT` |

---

## FBX Import

`FBXLoader` in `include/renderer/FBXLoader.h` + `src/renderer/FBXLoader.cpp` using **ufbx** (downloaded via CMake FetchContent, compiled as C from `build/_deps/ufbx-src/ufbx.c`).

```cpp
auto objects = FBXLoader::Import("path/to/model.fbx", scene);
auto mesh    = FBXLoader::LoadMesh("path/to/model.fbx::0");
auto data    = FBXLoader::LoadSkinnedMesh("path/to/model.fbx::0");
auto bundle  = FBXLoader::LoadClipsFromFile("path/to/anim.fbx");
```

- Mesh path format: same `filepath::N` convention as GLTF (N = mesh node index in traversal order)
- Loads with Y-up right-handed coordinate system + unit normalisation to meters
- Skinned mesh: reads skin deformer clusters → Skeleton + animation clips sampled at 30 fps
- Textures: tries relative path first, then absolute path stored in FBX file
- Import dialog supports `.fbx`; "Add Clip" dialog also supports `.fbx`

---

## GLTF / GLB Import

`GLTFLoader` in `include/renderer/GLTFLoader.h` + `src/renderer/GLTFLoader.cpp` using `vendor/tinygltf/tiny_gltf.h`.

```cpp
auto objects = GLTFLoader::Import("path/to/model.glb", scene);
auto mesh    = GLTFLoader::LoadMesh("path/to/model.gltf::2");
```

### Mesh path format
Each imported primitive stores its path as `filepath::N` where N is the primitive index in traversal order. `SceneSerializer` splits on `::` to reload the correct mesh.

### Embedded textures (GLB)
When a GLB contains embedded images, `GLTFLoader` writes them to `assets/textures/gltf_cache/<name>.png` on first import. This gives them a stable file path so serialization round-trips correctly.

### What's supported
- Node hierarchy with TRS or matrix transforms (transform accumulated recursively)
- PBR materials: `baseColorFactor`, `metallicFactor`, `roughnessFactor`, `baseColorTexture`, `normalTexture`
- Tangents from GLTF data or computed via Gram-Schmidt if absent
- All index types: `UNSIGNED_BYTE`, `UNSIGNED_SHORT`, `UNSIGNED_INT`
- External URI textures and embedded (GLB) textures

### Known limitation
Combined metallic+roughness texture (`metallicRoughnessTexture`) is not yet mapped. Only factor values are used.

---

## Serialization

- Format: JSON via nlohmann/json (header-only, `vendor/`)
- Save: `SceneSerializer::Save(scene, filepath)` — F5 or editor Save
- Load: `SceneSerializer::Load(filepath)` — returns `shared_ptr<Scene>`
- Path: `assets/scenes/MainScene.json`
- Asset paths stored **relative to `ASSET_DIR`** for portability

**Components serialized:**

| Component | Key fields |
|---|---|
| Transform | position, rotation (quat), scale |
| MeshRenderer | mesh path, material (albedo, roughness, metallic, texture, normalMap, worldUV) |
| SkinnedMeshRenderer | mesh path, clips, animRules, extraClipFiles, material, physicsYOffset |
| Rigidbody | mass, useGravity, + full PhysicsMaterial (friction, restitution, density, hardness, ior, transparency, absorbance, phase) |
| Collider | size, offset, isStatic |
| Light | lightType, color, intensity, direction, radius |
| LuaScript | path |
| AudioSource | clip, volume, pitch, loop, playOnStart, spatial, minDistance, maxDistance |
| AudioListener | (no fields) |
| UILabel | text, color, fontSize, centered |
| UIImage | texture, size, tint |

---

## Editor (ImGui + ImGuizmo)

`EditorLayer` is split across 6 source files in `src/editor/`:

| File | Responsibility |
|---|---|
| `EditorLayer.cpp` | Init, Shutdown, BeginFrame/EndFrame, Render dispatch |
| `EditorMenuBar.cpp` | DrawMenuBar, ToggleFullscreen |
| `EditorHierarchy.cpp` | DrawHierarchy |
| `EditorInspector.cpp` | DrawInspector (all components) |
| `EditorGizmo.cpp` | DrawGizmo (ImGuizmo toolbar + manipulation) |
| `EditorDialogs.cpp` | DrawImportDialog, DrawTexturePickerDialog, DrawScriptPickerDialog, DrawAddClipDialog, DrawAudioPickerDialog |

Layout constants (`HIERARCHY_W=200`, `INSPECTOR_W=280`, `MENUBAR_H=20`) are `static constexpr` in `EditorLayer`.

### Panels
- **MenuBar** — Scene > Save / Import Model (`.obj`, `.gltf`, `.glb`, `.fbx`), View > Fullscreen, Play/Stop, FPS
- **Hierarchy** — click to select; right-click / Delete key to delete (disabled in play mode)
- **Inspector** — Transform, MeshRenderer (albedo + normal map pickers, PBR sliders), SkinnedMeshRenderer (clips, AnimRules, Add Clip), Rigidbody (mass, material preset dropdown, optical section), Collider, Light, AudioSource (clip picker, volume/pitch/loop/spatial, Play/Pause/Stop in play mode), AudioListener, LuaScript, UILabel, UIImage
- **Add Component** — MeshRenderer, Rigidbody, Collider, Light, AudioSource, AudioListener, LuaScript
- **Import Dialog** — file browser for `.obj`, `.gltf`, `.glb`, `.fbx`
- **Texture Picker** — file browser for `.png`, `.jpg`
- **Audio Picker** — file browser for `.wav`, `.mp3`, `.ogg`, `.flac`

### Transform Gizmos (ImGuizmo)
- `ImGuizmo::BeginFrame()` inside `EditorLayer::BeginFrame()`
- Shortcuts: **T** = Translate, **R** = Rotate, **G** = Scale
- Scale gizmo forced to LOCAL space (ImGuizmo limitation)
- Decompose: position from `mat[3]`, scale from column lengths, rotation via `glm::quat_cast` on normalised columns

---

## Normal Mapping

| Layer | Status |
|---|---|
| `OBJLoader` | Tangent + bitangent per triangle (Gram-Schmidt) |
| `GLTFLoader` | Uses GLTF TANGENT attribute or computes if absent |
| `Vertex` struct | `position, normal, texCoord, tangent, bitangent` |
| `Material` | `normalMap` (`shared_ptr<Texture>`) |
| `Renderer::DrawMesh` | Binds normalMap to slot 1, sets `u_UseNormalMap` |
| OpenGL shader | `u_NormalMap` sampler unit 1, TBN in fragment |
| DX11 shader | `t_NormalMap` register t1, TBN in pixel shader |

---

## DX11 Backend — Critical Notes

### Winding order
`FrontCounterClockwise = TRUE` (matches OpenGL). plane.obj faces are CCW from above.

### GLM matrices
GLM is column-major. HLSL is row-major. DX11Shader uploads matrices **transposed**.

### Depth range
`GLM_FORCE_DEPTH_ZERO_TO_ONE` for DX11 only — `glm::perspective` outputs [0,1].

### Textures
- DX11: `stbi_set_flip_vertically_on_load(false)` — V=0 is top
- OpenGL: `stbi_set_flip_vertically_on_load(true)` — V=0 is bottom

### Shadow sampler
`FILTER_MIN_MAG_MIP_POINT` + `COMPARISON_NEVER` (regular sampler). HLSL uses `.Sample()` not `.SampleCmp()`. Do not change to `FILTER_COMPARISON_*`.

### State objects
| State | Where created | Lifetime |
|---|---|---|
| Main rasterizer | `DX11Context::Init` | DX11Context |
| Shadow rasterizer (`s_ShadowRS`) | `Renderer::Init` | `Renderer::Shutdown` |
| Skybox DSS | `DX11Skybox` ctor | destructor |
| Main DSS | `DX11Context::Init` | DX11Context |

**Never create state objects per-frame.**

---

## Shader Conventions

### OpenGL (`shaders/standard/`)
- Attribute locations: 0=position, 1=normal, 2=texCoord, 3=tangent, 4=bitangent
- Normal matrix = `transpose(inverse(u_Model))`

### DX11 (`shaders/dx11/standard.hlsl`)
- Semantics: `POSITION, NORMAL, TEXCOORD, TANGENT, BITANGENT`
- Normal matrix = `(float3x3)u_Model` (fine for uniform scale)
- Shadow UV fixup: `proj.y = 1.0 - proj.y`
- Poisson disk PCF — 16 samples, spread = 2.5 texels

---

## Common Gotchas

1. **PATH must have MSYS2 before Git** — Git's `mingw64` on PATH before MSYS2 causes `STATUS_DLL_NOT_FOUND`
2. **New .cpp files need cmake re-run** — `file(GLOB_RECURSE)` caches at configure time; run `cmake ..` before `mingw32-make` when adding source files
3. **VSCode IntelliSense errors** — `cannot open source file "glm/glm.hpp"` etc. are always false positives; MSYS2 paths unknown to the C++ extension. Real build errors only come from `mingw32-make`
4. **plane.obj winding** — faces are CCW from above; replacing plane.obj must keep this or normal points down in DX11
5. **Floor collider offset** — `size.y=0.2` → AABB ±0.1; `offset.y=-0.1` keeps top face at Y=0; otherwise objects float
6. **stbi flip** — DX11 no flip; OpenGL flip. Mixing them mirrors textures
7. **GLM depth range** — `GLM_FORCE_DEPTH_ZERO_TO_ONE` for DX11 only
8. **ImGui must init after Input** — GLFW callbacks chain; wrong order breaks mouse/keyboard
9. **Delta time clamped at 100ms** — prevents physics tunneling on window move or lag
10. **DX11 vs OpenGL cull** — main RS: CCW=front, cull back; shadow RS: cull FRONT
11. **Jolt character position = capsule center** — `GetCharacterPosition()` returns center, not foot; standing on Y=0 → charPos.y = 1.1; spawn = `meshPos.y - yOff` where yOff is negative (e.g. −1.1)
12. **StopPlay order** — stop AudioSources → clear destroy callback → `ClearBodies` → restore scene; any other order risks iterating freed pointers
13. **OptimizeBroadPhase must be called after SyncBodiesFromScene** — without it, `CastRay` and `ExtendedUpdate` cannot detect newly added static bodies; called at end of `SyncBodiesFromScene`
14. **m_CharMoveDir reset must be unconditional** — resetting inside `WantsCaptureKeyboard()` block means animation stays on walk when ImGui has focus; reset happens before the keyboard check
15. **EvaluateRules fallback** — if no Idle rule exists and nothing matches, play clip 0; without this fallback, animation sticks on last active clip when movement stops
16. **ufbx compiled as C not C++** — ufbx.c must be compiled as C (not C++); C++ gives `const` globals internal linkage which breaks symbol export; ufbx.c is added directly via `target_sources` with `${ufbx_SOURCE_DIR}/ufbx.c`
17. **miniaudio implementation TU** — `#define MINIAUDIO_IMPLEMENTATION` must appear in exactly one `.cpp` file (`src/audio/audio_impl.cpp`); including it in a header or multiple TUs causes ODR violations
18. **AudioSource loads lazily** — sound file is not loaded until first `Play()` or `OnPlayStart()`; if `clipPath` is set after play starts, call `Play()` explicitly to reload
19. **Rigidbody friction/restitution are now in material** — access via `rb->material.friction` and `rb->material.restitution`, not flat fields; JSON keys unchanged for backward compat
20. **Physics calls go through PhysicsWorld** — never call `JoltPhysicsSystem::` directly from Application code; use `m_PhysicsWorld.*` instead so future backends (FluidSystem) are routed correctly
