# Editor

ImGui-based editor ฝังอยู่ใน engine — ไม่มี separate editor process

---

## Layout

```
┌─────────────────────────────────────────────────────┐
│ MenuBar (File, View, Play/Stop, FPS, sliders)        │
├────────────┬────────────────────────┬────────────────┤
│            │                        │                │
│ Hierarchy  │   3D Viewport          │  Inspector     │
│  (200px)   │   (full screen 3D)     │   (280px)      │
│            │                        │                │
└────────────┴────────────────────────┴────────────────┘
```

Constants: `HIERARCHY_W=200`, `INSPECTOR_W=280`, `MENUBAR_H=20`

---

## Source Files

| File | ทำอะไร |
|---|---|
| `EditorLayer.cpp` | Init, Shutdown, BeginFrame/EndFrame, Render dispatch |
| `EditorMenuBar.cpp` | DrawMenuBar, ToggleFullscreen |
| `EditorHierarchy.cpp` | DrawHierarchy |
| `EditorInspector.cpp` | DrawInspector (all component panels) |
| `EditorGizmo.cpp` | DrawGizmo (ImGuizmo) |
| `EditorDialogs.cpp` | DrawImportDialog, DrawTexturePickerDialog, DrawScriptPickerDialog, DrawAddClipDialog, DrawAudioPickerDialog |

---

## Menu Bar

- **Scene > Save** — บันทึก MainScene.json (หรือกด F5)
- **Scene > Import Model** — เปิด import dialog (`.obj` / `.gltf` / `.glb` / `.fbx`)
- **View > Fullscreen** — toggle fullscreen (หรือกด F11)
- **Play / Stop** — toggle play mode
- **Exposure** slider (ทุก backend)
- **BT** (Bloom Threshold) slider (DX11)
- **BI** (Bloom Intensity) slider (DX11)
- FPS counter

---

## Hierarchy Panel

- Click → select GameObject
- Right-click → Delete
- Delete key → Delete selected
- **disabled ใน play mode**

---

## Inspector Panel

แสดง component ของ selected GameObject:

### Transform
- Position, Rotation (Euler), Scale sliders
- Disabled ใน play mode

### MeshRenderer
- Mesh path (readonly)
- Albedo color picker
- Texture + Normal Map picker (เปิด texture picker dialog)
- Clear texture / clear normal map buttons
- Roughness, Metallic sliders
- World UV toggle + tile slider

### SkinnedMeshRenderer
- Mesh path (readonly)
- Animation clips list (ชื่อ + ปุ่ม Play ต่อ clip)
- **Add Clips From File** button → เปิด Add Clip dialog (รองรับ `.fbx`, `.gltf`, `.glb`)
- Current clip index / total clips

### Rigidbody
- **Preset** dropdown — Default / Metal / Wood / Rubber / Ice / Glass / Water / Stone (โหลด PhysicsMaterial presets ทันที)
- Body Type (Static / Dynamic)
- Mass slider
- **Material** section:
  - Friction, Restitution sliders
  - Density slider
  - Hardness slider
  - **Optical** section (Glass / Water presets):
    - IOR (Index of Refraction) slider
    - Transparency slider
    - Absorbance slider
- Use Gravity checkbox

### Collider
- Size (AABB half-extents) XYZ
- Offset XYZ
- Is Static checkbox

### Light
- Type (Directional / Point)
- Color picker
- Intensity slider
- Direction XYZ (Directional)
- Radius slider (Point)

### LuaScript
- Script path (readonly)
- **Browse Script** button → เปิด script picker dialog
- Hot-reload indicator

### AudioSource
- Clip path (readonly)
- **Browse Audio** button → เปิด audio picker dialog
- Volume, Pitch sliders
- Loop, Play On Start checkboxes
- Spatial (3D) checkbox
- Min Distance, Max Distance sliders (แสดงเมื่อ Spatial = true)
- ปุ่ม **Play / Pause / Stop** (ใช้ได้ขณะ play mode ทำงาน)

### AudioListener
- แสดงเป็น header เท่านั้น (ไม่มี field ปรับ)
- ทำหน้าที่ sync ตำแหน่งหูจาก Transform ของ GameObject นี้

### UILabel
- Text input
- Font Size slider
- Color picker
- Centered checkbox
- Anchor dropdown (9 anchor points)
- Offset XY

### UIImage
- Texture path + picker button
- Size XY
- Tint color picker
- Anchor dropdown
- Offset XY

---

## Add Component

ปุ่ม **Add Component** ด้านล่าง Inspector รายการที่รองรับ:
- MeshRenderer
- Rigidbody
- Collider
- Light
- LuaScript
- AudioSource
- AudioListener
- UILabel
- UIImage

---

## Transform Gizmos (ImGuizmo)

keyboard shortcuts:
- **T** — Translate
- **R** — Rotate
- **G** — Scale (forced LOCAL space)

Space toggle: Local / World (Scale ล็อค Local เสมอ เป็น ImGuizmo limitation)

Matrix decompose: position from `mat[3]`, scale from column lengths, rotation via `glm::quat_cast`

---

## Import Dialog

File browser กรอง `.obj`, `.gltf`, `.glb`, `.fbx`

เมื่อ import:
- **OBJ** → `OBJLoader::Load` → สร้าง 1 GameObject
- **GLTF/GLB** → `GLTFLoader::Import` → สร้างหลาย GameObject ตาม node hierarchy
- **FBX** → `FBXLoader::Import` → สร้าง GameObject (static หรือ skinned)

---

## Texture Picker Dialog

File browser กรอง `.png`, `.jpg`

`m_TexSlot` enum เลือกว่าจะ set diffuse หรือ normal map

---

## Script Picker Dialog

File browser กรอง `.lua`

ผล: set `LuaScript::scriptPath` + hot-reload script ทันที

---

## Add Clip Dialog

File browser กรอง `.fbx`, `.gltf`, `.glb`

ผล: เรียก `SkinnedMeshRenderer::AddClipsFromFile(path, skeleton, clips)` — append animation clips ด้วย joint remapping by name

---

## Audio Picker Dialog

File browser กรอง `.wav`, `.mp3`, `.ogg`, `.flac`

ผล: set `AudioSource::clipPath` — sound จะโหลดใหม่เมื่อเล่นครั้งถัดไป

---

## Play Mode Restrictions

- Hierarchy: delete disabled
- Inspector: Transform edit disabled
- ปุ่ม Play เปลี่ยนเป็น Stop
- AudioSource: ปุ่ม Play/Pause/Stop ใช้ได้ใน play mode

---

## Keyboard Shortcuts (Global)

| Key | Action |
|---|---|
| F5 | Save scene |
| F11 | Toggle fullscreen |
| Escape | Stop play / quit |
| T | Gizmo: Translate |
| R | Gizmo: Rotate |
| G | Gizmo: Scale |
| RMB + drag | Camera look |
| WASD | Camera move (editor) / Character move (play) |
| Space/E | Move up (editor) / Jump (play) |
| Q/Ctrl | Move down (editor) |
| Shift | Camera speed ×3 |
| Scroll | Adjust fly speed |
