# MyEngine — Roadmap

## Done

### Core Engine
- [x] Scene / GameObject / Component (ECS-lite)
- [x] Transform (TRS, GetMatrix)
- [x] Scene Serialization (JSON, F5 save, load)
- [x] Input (GLFW keyboard + mouse + scroll)
- [x] Camera (free spectator + third-person follow)
- [x] Play / Stop mode (scene snapshot/restore)

### Editor
- [x] Hierarchy panel (select, delete)
- [x] Inspector panel (Transform, MeshRenderer, Rigidbody, Collider, Light)
- [x] Transform Gizmos (ImGuizmo) — T/R/G shortcuts
- [x] Import dialog (.obj, .gltf, .glb)
- [x] Texture picker dialog
- [x] Fullscreen toggle (F11)
- [x] Exposure / Bloom sliders in menu bar

### Rendering
- [x] DX11 + OpenGL dual backend (RHI abstraction)
- [x] PBR shading — Cook-Torrance BRDF (GGX, Smith, Fresnel)
- [x] Normal mapping (OBJ + GLTF tangents)
- [x] Directional shadow (PCF 16-sample, 2048×2048)
- [x] Point light shadow — omnidirectional cubemap 512×512 (DX11)
- [x] SSAO — 32-sample hemisphere, blur (DX11)
- [x] HDR render target (RGBA16F) + ACES tonemap
- [x] Bloom — soft-knee threshold + Gaussian blur half-res (DX11)
- [x] Procedural skybox (gradient + sun direction)
- [x] Up to 4 directional + 8 point lights per frame

### Physics
- [x] Jolt Physics — rigidbody, static collider, character controller
- [x] Collision callbacks → OnCollisionEnter/Stay/Exit
- [x] Character capsule — WASD move, Space jump, third-person camera

### Assets
- [x] OBJ import (tangent generation)
- [x] GLTF/GLB import — node hierarchy, PBR materials, embedded textures

### Debug
- [x] D3D11 Debug Layer (cmake -DDX11_DEBUG=ON)
- [x] DX_CHECK macro (HRESULT + file/line)
- [x] GPU markers (PIX/RenderDoc) รอบทุก pass

---

## Up Next — Gameplay Systems (ลำดับนี้)

### 1. In-game UI
- [ ] Text rendering (font atlas หรือ ImGui runtime draw)
- [ ] UILabel, UIImage components
- [ ] Screen-space anchor/layout (top-left, center, etc.)
- [ ] HUD ที่ render แยกจาก editor ImGui

### 2. Lua Scripting
- [ ] รวม sol2 + Lua 5.4 ใน vendor/
- [ ] LuaScript component — path + hot-reload
- [ ] Bind Transform (read/write position/rotation/scale)
- [ ] Bind Input (IsKeyPressed, IsMouseButtonPressed)
- [ ] Bind GameObject (Find, Destroy, SetActive)
- [ ] Bind Time (deltaTime, totalTime)
- [ ] Update() + OnCollisionEnter() hooks จาก Lua

### 3. Scene Transitions
- [ ] `Application::LoadScene(path)` — โหลด scene ใหม่แทนที่ปัจจุบัน
- [ ] Scene stack / history (กลับ scene ก่อนหน้าได้)
- [ ] Transition ใน play mode (ไม่ใช่แค่ editor)

### 4. Audio
- [ ] รวม miniaudio (single-header) ใน vendor/
- [ ] AudioSource component — clip path, volume, loop, play/stop
- [ ] AudioListener component (ผูกกับ camera position)
- [ ] 3D positional audio (distance attenuation)

---

## Future — เมื่อ gameplay systems ครบแล้ว

### Editor Polish
- [ ] Undo/Redo (command pattern)
- [ ] Asset browser panel
- [ ] Scene view เป็น render target ใน ImGui window

### Graphics
- [ ] SSAO — OpenGL parity
- [ ] IBL (Image Based Lighting) — equirectangular HDR → cubemap, irradiance, prefilter
- [ ] Auto shadow from any directional light (ไม่ hardcode m_Sun)

### Performance
- [ ] Frustum culling (6-plane test per object)
- [ ] Instanced rendering (DrawMeshInstanced)

### Scripting / Modding
- [ ] Hot-reload Lua ขณะ play mode ทำงาน
- [ ] Expose scene API ให้ครบ (CreateGameObject, AddComponent)
