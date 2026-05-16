#pragma once

#include "renderer/Material.h"
#include "renderer/LightData.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

class Shader;
class Mesh;
class Camera;
class Scene;
class ShadowMap;
class Skybox;

class Renderer {
public:
    static void Init();
    static void Shutdown();
    static void Clear();
    static void ReloadShaders();  // dev-only: re-create all shaders from disk (F6)

    // Picks the first active directional light in the scene and fits the
    // ortho box to the camera frustum. No-op if there is no directional light.
    static void ShadowPass(Scene& scene, const Camera& camera);

    // SSAO pre-pass (DX11 only — no-op on OpenGL path)
    static void GBufferPass(Scene& scene, Camera& cam, int w, int h);
    static void SSAOPass(Camera& cam, int w, int h);
    static void SSAOBlurPass();

    // Point light shadow (DX11 only — shadows the first point light in the scene)
    static void PointShadowPass(Scene& scene);

    static void BeginScene(const Camera& camera,
                           const std::vector<DirLightData>& dirLights,
                           const std::vector<PointLightData>& pointLights);

    static void DrawMesh(const Mesh& mesh, const glm::mat4& modelMatrix,
                         const Material& material = Material::Default(),
                         const glm::mat4* boneMatrices = nullptr, int boneCount = 0);

    static void DrawSkybox(const Camera& camera, const glm::vec3& sunDirection);

    // HDR + Tone Mapping
    static void  BeginHDRPass(int w, int h);
    static void  EndHDRPass();
    static void  ApplyTonemap();
    static void  SetExposure(float e) { s_Exposure = e; }
    static float GetExposure()        { return s_Exposure; }

    // Render-to-texture viewport (scene-in-ImGui-window mode).
    // ResizeViewport (re)allocates the offscreen color target.
    // ApplyTonemapToViewport runs the tonemap pass writing to that target
    // instead of the backbuffer. GetViewportTextureID returns an
    // ImGui-compatible handle (ID3D11ShaderResourceView* for DX11,
    // (void*)(uintptr_t)glTex for OpenGL).
    static void  ResizeViewport(int w, int h);
    static void  ApplyTonemapToViewport();
    static void* GetViewportTextureID();
    static int   GetViewportWidth();
    static int   GetViewportHeight();
    static void  ClearBackbuffer(int w, int h);

    // Bloom (DX11 only — no-op on OpenGL path)
    static void  BloomPass(int w, int h);
    static void  SetBloomThreshold(float t) { s_BloomThreshold = t; }
    static float GetBloomThreshold()        { return s_BloomThreshold; }
    static void  SetBloomIntensity(float i) { s_BloomIntensity = i; }
    static float GetBloomIntensity()        { return s_BloomIntensity; }

private:
    // Only public-configurable knobs live in the class. Pipeline state and
    // backend resources are file-scope in RendererState.h.
    static float                   s_Exposure;
    static float                   s_BloomThreshold;
    static float                   s_BloomIntensity;
};
