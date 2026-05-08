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
| `EditorInspector.cpp` | DrawInspector |
| `EditorGizmo.cpp` | DrawGizmo (ImGuizmo) |
| `EditorDialogs.cpp` | DrawImportDialog, DrawTexturePickerDialog |

---

## Menu Bar

- **Scene > Save** — บันทึก MainScene.json (หรือกด F5)
- **Scene > Import Model** — เปิด import dialog (.obj / .gltf / .glb)
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

### MeshRenderer
- Mesh path (readonly)
- Albedo color picker
- Texture + Normal Map picker (เปิด texture picker dialog)
- Clear texture / clear normal map buttons
- Roughness, Metallic sliders
- World UV toggle + tile slider

### Rigidbody
- Body Type (Static / Dynamic)
- Mass, Friction, Restitution sliders
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

File browser กรอง `.obj`, `.gltf`, `.glb`

เมื่อ import:
- OBJ → `OBJLoader::Load` → สร้าง 1 GameObject
- GLTF/GLB → `GLTFLoader::Import` → สร้างหลาย GameObject ตาม node hierarchy

---

## Texture Picker Dialog

File browser กรอง `.png`, `.jpg`

`m_TexSlot` enum เลือกว่าจะ set diffuse หรือ normal map

---

## Play Mode Restrictions

- Hierarchy: delete disabled
- Inspector: Transform edit disabled
- ปุ่ม Play เปลี่ยนเป็น Stop

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
