# Rendering Pipeline

## Frame Order (DX11)

```
1. ShadowPass          — directional light, 2048×2048 depth
2. PointShadowPass     — first point light, cubemap 512×512 (6 faces)
3. GBufferPass         — view-space position + normal (MRT)
4. SSAOPass            — 32-sample hemisphere, 4×4 noise
5. SSAOBlurPass        — 5×5 box blur → R8 AO texture
6. BeginHDRPass        — bind RGBA16F render target
7.   Clear
8.   DrawSkybox        — procedural gradient, depth LESS_EQUAL
9.   Scene::Render     — BeginScene → DrawMesh per object
10. EndHDRPass
11. BloomPass          — threshold (soft-knee) → H-blur → V-blur (half-res)
12. ApplyTonemap       — ACES, composite bloom, → backbuffer
13. Editor (ImGui)
14. Present
```

## Frame Order (OpenGL)

```
1. ShadowPass
2. BeginHDRPass (HDR FBO)
3.   Clear
4.   DrawSkybox
5.   Scene::Render
6. EndHDRPass
7. ApplyTonemap
8. Editor (ImGui)
9. glfwSwapBuffers
```

---

## Shaders (DX11)

| File | ใช้ที่ไหน |
|---|---|
| `standard.hlsl` | main scene pass — PBR, lighting, shadows, SSAO |
| `depth.hlsl` | directional shadow pass |
| `point_shadow.hlsl` | point light cubemap shadow pass |
| `gpass.hlsl` | G-buffer: outputs position + normal |
| `ssao.hlsl` | SSAO compute |
| `ssao_blur.hlsl` | 5×5 box blur |
| `tonemap.hlsl` | ACES tonemap + bloom composite |
| `bloom_threshold.hlsl` | soft-knee bright-pass filter |
| `bloom_blur.hlsl` | separable Gaussian blur (9-tap) |
| `skybox.hlsl` | procedural sky |

## Shaders (OpenGL)

| File | ใช้ที่ไหน |
|---|---|
| `standard/default.vert/.frag` | main scene pass |
| `depth/depth.vert/.frag` | shadow pass |
| `skybox/skybox.vert/.frag` | skybox |
| `postprocess/tonemap.vert/.frag` | tonemap |

---

## PBR Model

Cook-Torrance BRDF:
- **D** — GGX normal distribution
- **G** — Smith geometry (Schlick-GGX)
- **F** — Fresnel-Schlick

Ambient: hemisphere lerp (sky/ground color) + Fresnel specular ambient

Lights supported per frame:
- Directional: max 4
- Point: max 8

---

## Shadows

### Directional Shadow
- 2048×2048 depth map
- Ortho projection 100×100, range 1–150
- Shadow RS: `CULL_FRONT` (peter-panning prevention)
- Sampling: 16-sample Poisson disk PCF, spread 2.5 texels
- **Skinning**: `RendererShadow` ตรวจ `SkinnedMeshRenderer` และอัปโหลด bone matrices (`u_BoneMatrices[100]`, `u_UseSkinning`) — shadow pass รองรับ skinned mesh ทั้ง OpenGL และ DX11

### Point Light Shadow (DX11)
- R16_FLOAT cubemap, 512×512 per face
- 6 passes, 90° FOV perspective
- Linear distance: writes `dist/farPlane` as color
- Sampling: single sample + bias 0.005
- Shadows only the **first** point light in scene

---

## SSAO (DX11)

- G-buffer: view-space position (RGBA32F) + normal (RGBA16F)
- 32 hemisphere samples (random kernel, seed=42)
- 4×4 rotation noise texture
- Range check: discards samples outside 0.5 unit range
- Blur: 5×5 box → R8_UNORM output
- Bound to `t3` in standard.hlsl, sampled via screen UV from SV_POSITION

---

## Bloom (DX11)

- **Threshold pass**: soft-knee filter — smooth ramp around threshold
  - `knee = threshold * kneeRatio`
  - quadratic falloff in knee region
- **Blur**: 9-tap separable Gaussian at **half resolution**
  - weights: `{0.227027, 0.194595, 0.121622, 0.054054, 0.016216}`
  - step scale ×2 → effective ~32px radius at full-res
  - H-pass: BloomA → BloomB; V-pass: BloomB → BloomA
- **Composite**: tonemap.hlsl adds `bloom * u_BloomIntensity` before ACES

Editor sliders: **BT** (Bloom Threshold 0.1–3.0), **BI** (Bloom Intensity 0.0–2.0)

---

## Texture Slots (DX11 standard.hlsl)

| Slot | ชื่อ | ใช้กับ |
|---|---|---|
| t0 | t_Albedo | diffuse texture |
| t1 | t_NormalMap | normal map |
| t2 | t_ShadowMap | directional shadow |
| t3 | t_SSAO | SSAO blur result |
| t4 | t_PointShadow | point shadow cubemap |

---

## Material Fields

```cpp
struct Material {
    glm::vec3              albedo        = {1,1,1};
    float                  roughness     = 0.5f;
    float                  metallic      = 0.0f;
    shared_ptr<Texture>    texture;       // diffuse
    shared_ptr<Texture>    normalMap;
    bool                   worldSpaceUV  = false;
    float                  worldUVTile   = 1.0f;
};
```

---

## RHI Abstraction

Abstract factory pattern — code ใช้ base class, runtime เลือก impl:

```cpp
auto mesh    = Mesh::Create(vertices, indices);
auto shader  = Shader::Create(vsPath, fsPath);
auto texture = Texture::Create(imagePath);
auto tex     = Texture::CreateFromMemory(w, h, rgbaPtr, "name");
```

DX11 implementations: `rhi/dx11/`
OpenGL implementations: `rhi/opengl/`
