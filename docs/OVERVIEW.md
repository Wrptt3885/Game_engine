# MyEngine — Overview

Custom 3D game engine written in C++17. Dual graphics backend (DX11 / OpenGL).
Built with MinGW-w64 on Windows.

---

## Quick Build

```powershell
$env:PATH = "C:\msys64\mingw64\bin;C:\msys64\usr\bin;" + $env:PATH
cd build

# DX11 (primary — most features)
cmake -G "MinGW Makefiles" -DUSE_DX11=ON -DCMAKE_BUILD_TYPE=Release ..
mingw32-make -j8

# OpenGL
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
mingw32-make -j8
```

> CMake uses `file(GLOB_RECURSE)` — re-run `cmake ..` whenever you add a new `.cpp` file.

---

## Feature Status

### Rendering
| Feature | DX11 | OpenGL |
|---|---|---|
| PBR (Cook-Torrance BRDF) | ✅ | ✅ |
| Normal Mapping | ✅ | ✅ |
| Directional Shadow (PCF 16-sample) | ✅ | ✅ |
| Point Light Shadow (cubemap) | ✅ | ❌ |
| SSAO (32-sample) | ✅ | ❌ |
| Bloom (soft-knee + Gaussian) | ✅ | ❌ |
| HDR + ACES Tonemap | ✅ | ✅ |
| Procedural Skybox | ✅ | ✅ |
| Skeletal Animation (GPU skinning) | ✅ | ✅ |

### Engine Systems
| System | Status |
|---|---|
| Scene / GameObject / Component (ECS-lite) | ✅ |
| Transform (TRS, quaternion rotation) | ✅ |
| Scene Serialization (JSON, F5 save/load) | ✅ |
| Input (GLFW keyboard + mouse + scroll) | ✅ |
| Camera (free spectator + third-person follow) | ✅ |
| Play / Stop mode (scene snapshot/restore) | ✅ |
| Jolt Physics — rigidbody, collider, character controller | ✅ |
| Physics Architecture — PhysicsWorld, PhysicsMaterial, IPhysicsSystem | ✅ |
| Collision Callbacks (Enter/Stay/Exit → Component) | ✅ |
| Asset Import — OBJ, GLTF/GLB, FBX | ✅ |
| Skeletal Animation — clips, state machine, joint remapping | ✅ |
| In-game UI — UILabel, UIImage, anchor layout | ✅ |
| Lua Scripting — sol2 + Lua 5.4, hot-reload, bindings | ✅ |
| Audio — miniaudio, AudioSource (3D spatial), AudioListener | ✅ |
| Editor — Hierarchy, Inspector, Gizmos, dialogs | ✅ |
| Debug — D3D11 Debug Layer, DX_CHECK, PIX/GPU markers | ✅ |

### Planned (Future)
| System | Priority |
|---|---|
| Scene Transitions (LoadScene, scene stack) | High |
| IBL (Image-Based Lighting) | Medium |
| Frustum Culling | Medium |
| Undo/Redo (command pattern) | Low |
| Hardware Ray Tracing (DX12 + DXR) | Long-term |

---

## Directory Structure

```
Game_engine/
├── docs/           ← documentation (this folder)
├── include/
│   ├── core/       Application, Scene, GameObject, Component, Transform, Camera, SceneSerializer
│   ├── editor/     EditorLayer
│   ├── audio/      AudioManager, AudioSource, AudioListener
│   ├── physics/    PhysicsWorld, PhysicsMaterial, IPhysicsSystem,
│   │               JoltPhysicsSystem, Rigidbody, Collider
│   ├── renderer/   Renderer, Mesh, MeshRenderer, SkinnedMeshRenderer, Skeleton,
│   │               AnimationClip, Shader, Texture, Material, Light, LightData,
│   │               ShadowMap, Skybox, OBJLoader, GLTFLoader, FBXLoader
│   ├── scripting/  LuaManager, LuaScript
│   ├── ui/         UICanvas, UILayer, UIElement, UILabel, UIImage, UITypes
│   ├── platform/   Input
│   └── rhi/        RHI.h, dx11/, opengl/
├── src/            mirrors include/ module layout
├── shaders/
│   ├── dx11/       standard, depth, gpass, ssao, ssao_blur, tonemap, bloom_*, point_shadow, skybox
│   ├── standard/   OpenGL default vert/frag
│   ├── depth/      OpenGL shadow vert/frag
│   └── skybox/     OpenGL skybox vert/frag
├── assets/
│   ├── models/
│   ├── textures/gltf_cache/
│   └── scenes/     MainScene.json
└── vendor/
    ├── imgui/      Dear ImGui
    ├── imguizmo/   ImGuizmo
    ├── tinygltf/   tiny_gltf.h (GLTF/GLB parser)
    ├── miniaudio.h single-header audio library
    ├── stb_image.h / stb_image_write.h
    ├── JoltPhysics/ physics library
    └── (ufbx via FetchContent → build/_deps/ufbx-src/)
```
