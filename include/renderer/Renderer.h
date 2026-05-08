#pragma once

#include "graphics/Material.h"
#include "graphics/LightData.h"
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

    static void ShadowPass(Scene& scene, const glm::vec3& lightDir);

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
                         const Material& material = Material::Default());

    static void DrawSkybox(const Camera& camera, const glm::vec3& sunDirection);

    // HDR + Tone Mapping
    static void  BeginHDRPass(int w, int h);
    static void  EndHDRPass();
    static void  ApplyTonemap();
    static void  SetExposure(float e) { s_Exposure = e; }
    static float GetExposure()        { return s_Exposure; }

    // Bloom (DX11 only — no-op on OpenGL path)
    static void  BloomPass(int w, int h);
    static void  SetBloomThreshold(float t) { s_BloomThreshold = t; }
    static float GetBloomThreshold()        { return s_BloomThreshold; }
    static void  SetBloomIntensity(float i) { s_BloomIntensity = i; }
    static float GetBloomIntensity()        { return s_BloomIntensity; }

private:
    static std::shared_ptr<Shader> s_Shader;
    static std::shared_ptr<Shader> s_DepthShader;
    static std::shared_ptr<Shader> s_TonemapShader;
    static std::shared_ptr<Shader> s_BloomThresholdShader;
    static std::shared_ptr<Shader> s_BloomBlurShader;
    static std::shared_ptr<Shader> s_PointDepthShader;
    static glm::mat4               s_LightSpaceMatrix;
    static float                   s_Exposure;
    static float                   s_BloomThreshold;
    static float                   s_BloomIntensity;
    static glm::vec3               s_PointShadowLightPos;
    static float                   s_PointShadowFarPlane;
    static bool                    s_HasPointShadow;
    static int                     s_HDR_W, s_HDR_H;
};
