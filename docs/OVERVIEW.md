# MyEngine — Overview

Custom 3D game engine written in C++17. Dual graphics backend (DX11 / OpenGL).
Built with MinGW-w64 on Windows.

---

## Quick Build

```powershell
$env:PATH = "C:\msys64\mingw64\bin;C:\msys64\usr\bin;" + $env:PATH
cd build_dx11

# DX11 (primary)
cmake -G "MinGW Makefiles" -DUSE_DX11=ON -DCMAKE_BUILD_TYPE=Release ..
mingw32-make -j1

# OpenGL
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
mingw32-make -j1
```

> ใช้ `-j1` เพราะ MinGW parallel build มี race condition

---

## Current Status

### ทำเสร็จแล้ว — พร้อมใช้

| ระบบ | หมายเหตุ |
|---|---|
| DX11 + OpenGL RHI | abstract Mesh/Shader/Texture factory |
| PBR Shading | Cook-Torrance BRDF, metallic/roughness |
| Directional Shadow | PCF 16-sample, 2048×2048 |
| Point Light Shadow | cubemap 512×512, linear distance (DX11) |
| SSAO | G-buffer pre-pass, 32-sample kernel (DX11) |
| Bloom | soft-knee threshold + Gaussian blur (DX11) |
| HDR + ACES Tonemap | RGBA16F render target |
| Normal Mapping | TBN, OBJ + GLTF tangent |
| Procedural Skybox | gradient + sun direction |
| Jolt Physics | rigidbody, static collider, character controller |
| Collision Callbacks | OnCollisionEnter/Stay/Exit → Component |
| GLTF/GLB Import | node hierarchy, PBR materials, embedded textures |
| OBJ Import | triangle fan, tangent generation |
| Scene Serialization | JSON (nlohmann), F5 = save |
| Editor | Hierarchy, Inspector, Gizmos (T/R/G), Import dialog |
| Play Mode | physics snapshot/restore, character capsule |
| Input | GLFW keyboard + mouse, scroll |
| Camera | free spectator + third-person follow |
| Debug Tools | D3D11 Debug Layer, DX_CHECK, PIX/GPU markers |

### ยังไม่มี — ต้องทำก่อนทำเกมได้

| ระบบ | ความสำคัญ |
|---|---|
| In-game UI (HUD, text, menu) | สูงมาก |
| Lua Scripting | สูงมาก |
| Scene Transitions | สูง |
| Audio | สูง |

---

## DX11 vs OpenGL Feature Matrix

| Feature | DX11 | OpenGL |
|---|---|---|
| PBR | ✅ | ✅ |
| Normal Mapping | ✅ | ✅ |
| Directional Shadow | ✅ | ✅ |
| HDR + Tonemap | ✅ | ✅ |
| SSAO | ✅ | ❌ |
| Bloom | ✅ | ❌ |
| Point Light Shadow | ✅ | ❌ |

DX11 คือ primary path ที่ feature ครบที่สุด

---

## Directory Structure

```
Game_engine/
├── docs/           ← documentation (this folder)
├── include/        ← headers
│   ├── core/       Camera, Application, Scene, GameObject, Transform, Component
│   ├── graphics/   Light, Material, LightData, ShadowMap, Skybox
│   ├── physics/    Rigidbody, Collider, JoltPhysicsSystem
│   ├── platform/   Input
│   ├── renderer/   Renderer, Mesh, MeshRenderer, OBJLoader, GLTFLoader, Texture, Shader
│   └── rhi/        RHI.h, dx11/, opengl/
├── src/
│   ├── core/
│   ├── editor/     6 files: EditorLayer, MenuBar, Hierarchy, Inspector, Gizmo, Dialogs
│   ├── physics/
│   ├── renderer/   Renderer.cpp แยก: Scene, Shadow, HDR, Bloom, SSAO
│   └── rhi/
├── shaders/
│   ├── dx11/       standard, depth, gpass, ssao, ssao_blur, tonemap, bloom_*, point_shadow
│   └── standard/   OpenGL default vert/frag
├── assets/
│   ├── models/
│   ├── textures/
│   └── scenes/     MainScene.json
└── vendor/         imgui, imguizmo, tinygltf, stb_image, JoltPhysics, nlohmann
```
