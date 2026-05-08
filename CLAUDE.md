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
│   ├── core/               Camera, Application, Scene, GameObject, Transform, Component,
│   │                       EditorLayer, SceneSerializer
│   ├── graphics/           Light, Material, LightData, ShadowMap (GL), Skybox (GL)
│   ├── physics/            Rigidbody, Collider, JoltPhysicsSystem
│   ├── platform/           Input
│   ├── renderer/           Renderer, Mesh, MeshRenderer, OBJLoader, GLTFLoader, Texture, Shader
│   └── rhi/
│       ├── RHI.h           GraphicsAPI enum + static Init/GetAPI
│       ├── dx11/           DX11Context, DX11Mesh, DX11Shader, DX11ShadowMap,
│       │                   DX11Skybox, DX11Texture
│       └── opengl/         GLMesh, GLShader, GLTexture
├── src/
│   ├── main.cpp            entry point
│   ├── core/
│   │   ├── Application.cpp         init, run loop, render, shutdown
│   │   ├── ApplicationPlayMode.cpp StartPlay / StopPlay
│   │   ├── Scene.cpp, Camera.cpp, GameObject.cpp, SceneSerializer.cpp, ...
│   ├── editor/
│   │   ├── EditorLayer.cpp         Init, Shutdown, BeginFrame/EndFrame, Render dispatch
│   │   ├── EditorMenuBar.cpp       DrawMenuBar, ToggleFullscreen
│   │   ├── EditorHierarchy.cpp     DrawHierarchy
│   │   ├── EditorInspector.cpp     DrawInspector
│   │   ├── EditorGizmo.cpp         DrawGizmo
│   │   └── EditorDialogs.cpp       DrawImportDialog, DrawTexturePickerDialog
│   ├── physics/            JoltPhysicsSystem.cpp
│   ├── renderer/           Renderer.cpp, OBJLoader.cpp, GLTFLoader.cpp, MeshRenderer.cpp, ...
│   ├── rhi/
│   │   ├── dx11/
│   │   └── opengl/
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
    ├── stb_image.h
    └── stb_image_write.h   used by GLTFLoader to export embedded GLB textures
```

---

## Architecture

### Startup sequence

```
main.cpp
  └─ Application()
       ├─ InitWindow()      GLFW window; DX11: also DX11Context::Init(hwnd)
       ├─ InitRenderer()    Renderer::Init() — shaders, shadow map, skybox, RS states
       ├─ InitInput()       Input::Init(window) — installs GLFW callbacks
       ├─ m_Editor.Init()   ImGui init (must come after Input so callbacks chain)
       ├─ JoltPhysicsSystem::Init()
       └─ InitScene()       create Camera, Scene, default GameObjects
```

### Frame loop

```
Run()
  ProcessInput(dt)   → Input::Update, WASD/mouse-look, scroll speed, F5/F11
  Update(dt)         → floor follows camera XZ (edit only), play/stop requests,
                        character movement → JoltPhysicsSystem::Update
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

Powered by **JoltPhysics** (`vendor/JoltPhysics/`). The legacy `PhysicsSystem.cpp` (AABB-based) still exists but is superseded in play mode.

### Key API
```cpp
JoltPhysicsSystem::Init();                         // once at startup
JoltPhysicsSystem::SyncBodiesFromScene(scene);     // build body map from Collider/Rigidbody
JoltPhysicsSystem::Update(scene, dt);              // step + write transforms back
JoltPhysicsSystem::ClearBodies();                  // remove all bodies
JoltPhysicsSystem::RemoveBody(GameObject* go);     // remove a single body (called by Scene destroy callback)
JoltPhysicsSystem::CreateCharacter(spawnPos);      // capsule CharacterVirtual
JoltPhysicsSystem::MoveCharacter(dt, wishDir, jump);
JoltPhysicsSystem::GetCharacterPosition();
```

### Rigidbody fields
| Field | Default | Notes |
|---|---|---|
| `mass` | 1.0f | kg |
| `useGravity` | true | |
| `friction` | 0.2f | 0–1 |
| `restitution` | 0.0f | 0–1 (bounciness) |
| `velocity` | vec3(0) | read/write |

### Character capsule geometry
Jolt `CharacterVirtual`: `halfHeight=0.8`, `radius=0.3` → total height 2.2 units.  
`GetCharacterPosition()` returns the **capsule center** (1.1 above ground when standing), not the foot.  
Visual body cube (scale 1.6 tall) must offset by `−0.3` (= −radius) to sit on the ground. Head cube offset `+0.75`.

### Body lifecycle
- `SyncBodiesFromScene` rebuilds the entire body map from scratch (called at play start).
- `Scene::DestroyGameObject` calls `JoltPhysicsSystem::RemoveBody` automatically in play mode (via destroy callback).
- `StopPlay` clears the callback **before** `ClearBodies` to avoid iterating stale pointers.

---

## Play Mode

Toggled with the Play/Stop button in the editor menu bar, or **Escape** key.

**StartPlay** (`ApplicationPlayMode.cpp`):
1. Snapshots the scene to JSON string
2. Calls `JoltPhysicsSystem::SyncBodiesFromScene`
3. Creates `CharacterVirtual` capsule near camera position
4. Spawns two `GameObject`s: `__CharBody` (blue box) and `__CharHead` (skin box)
5. Registers a destroy callback so `DestroyGameObject` → `JoltPhysicsSystem::RemoveBody`

**StopPlay**:
1. Destroys character, clears callback, clears all bodies
2. Restores scene from JSON snapshot (removes `__CharBody` / `__CharHead`)
3. Re-syncs physics for the restored scene

**Play controls**: WASD = move character (relative to camera heading); Space = jump; RMB + drag = look around. Hierarchy delete / Inspector editing are disabled during play.

---

## GLTF / GLB Import

`GLTFLoader` in `include/renderer/GLTFLoader.h` + `src/renderer/GLTFLoader.cpp` using `vendor/tinygltf/tiny_gltf.h`.

```cpp
// Import file → creates GameObjects in scene, returns them
auto objects = GLTFLoader::Import("path/to/model.glb", scene);

// Reload a specific primitive (used by SceneSerializer)
auto mesh = GLTFLoader::LoadMesh("path/to/model.gltf::2");
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
Combined metallic+roughness texture (`metallicRoughnessTexture`, G=roughness, B=metallic) is not yet mapped to the material. Only factor values are used.

---

## Serialization

- Format: JSON via nlohmann/json (header-only, `vendor/`)
- Save: `SceneSerializer::Save(scene, filepath)` — F5 or editor Save
- Load: `SceneSerializer::Load(filepath)` — returns `shared_ptr<Scene>`
- Path: `assets/scenes/MainScene.json`
- Asset paths stored **relative to `ASSET_DIR`** for portability
- GLTF meshes stored as relative `filepath::N` (primitive index suffix)
- Components serialized: Transform, MeshRenderer (mesh + material + textures), Rigidbody (**including friction + restitution**), Collider, Light

---

## Editor (ImGui + ImGuizmo)

`EditorLayer` is split across 6 source files in `src/editor/`:

| File | Responsibility |
|---|---|
| `EditorLayer.cpp` | Init, Shutdown, BeginFrame/EndFrame, Render dispatch |
| `EditorMenuBar.cpp` | DrawMenuBar, ToggleFullscreen |
| `EditorHierarchy.cpp` | DrawHierarchy |
| `EditorInspector.cpp` | DrawInspector (Transform, MeshRenderer, Rigidbody, Collider, Light) |
| `EditorGizmo.cpp` | DrawGizmo (ImGuizmo toolbar + manipulation) |
| `EditorDialogs.cpp` | DrawImportDialog, DrawTexturePickerDialog |

Layout constants (`HIERARCHY_W=200`, `INSPECTOR_W=280`, `MENUBAR_H=20`) are `static constexpr` in `EditorLayer`.

### Panels
- **MenuBar** — Scene > Save / Import Model (`.obj`, `.gltf`, `.glb`), View > Fullscreen, Play/Stop button, FPS
- **Hierarchy** — click to select; right-click / Delete key to delete (disabled in play mode)
- **Inspector** — Transform, MeshRenderer (albedo tex + normal map pickers, PBR sliders), Rigidbody (mass, friction, restitution, gravity), Collider, Light (type, color, intensity, direction/radius)
- **Import Dialog** — file browser for `.obj`, `.gltf`, `.glb`
- **Texture Picker** — file browser for `.png`, `.jpg`; `m_TexSlot` enum selects diffuse vs normal map

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
11. **Jolt character position = capsule center** — `GetCharacterPosition()` returns center, not foot; standing on Y=0 → charPos.y = 1.1; offset visuals accordingly
12. **StopPlay order** — clear destroy callback → `ClearBodies` → `m_CharacterBody/Head = nullptr` → restore scene; any other order risks iterating freed pointers
