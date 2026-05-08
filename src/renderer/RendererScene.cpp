#include "RendererState.h"
#include "renderer/Mesh.h"
#include "renderer/MeshRenderer.h"
#include "core/Camera.h"
#include "graphics/Material.h"
#include "graphics/LightData.h"
#include <algorithm>

void Renderer::BeginScene(const Camera& camera,
                           const std::vector<DirLightData>& dirLights,
                           const std::vector<PointLightData>& pointLights) {
    if (!s_Shader) return;

    s_Shader->setMat4("u_View",             camera.GetViewMatrix());
    s_Shader->setMat4("u_Projection",       camera.GetProjectionMatrix());
    s_Shader->setVec3("u_ViewPos",          camera.GetPosition());
    s_Shader->setMat4("u_LightSpaceMatrix", s_LightSpaceMatrix);

#ifdef USE_DX11_BACKEND
    {
        auto* ctx = DX11Context::GetContext();
        if (s_DX11Shadow) {
            auto* srv     = s_DX11Shadow->GetSRV();
            auto* sampler = s_DX11Shadow->GetSampler();
            ctx->PSSetShaderResources(2, 1, &srv);
            ctx->PSSetSamplers(1, 1, &sampler);
        }
        // Point shadow cubemap — slot t4; null if no point shadow this frame
        ID3D11ShaderResourceView* ptSRV = s_HasPointShadow && s_DX11PointShadow
                                          ? s_DX11PointShadow->GetSRV() : nullptr;
        ctx->PSSetShaderResources(4, 1, &ptSRV);
    }
    s_Shader->setVec3 ("u_PointShadowPos", s_PointShadowLightPos);
    s_Shader->setFloat("u_PointShadowFar", s_PointShadowFarPlane);
    s_Shader->setBool ("u_HasPointShadow", s_HasPointShadow);
#else
    s_Shader->setInt("u_ShadowMap", 2);
    if (s_ShadowMap) {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, s_ShadowMap->GetDepthTexture());
    }
#endif

    int dc = std::min((int)dirLights.size(), 4);
    s_Shader->setInt("u_DirLightCount", dc);
    for (int i = 0; i < dc; i++) {
        std::string b = "u_DirLights[" + std::to_string(i) + "].";
        s_Shader->setVec3 (b + "direction", dirLights[i].direction);
        s_Shader->setVec3 (b + "color",     dirLights[i].color);
        s_Shader->setFloat(b + "intensity", dirLights[i].intensity);
    }

    int pc = std::min((int)pointLights.size(), 8);
    s_Shader->setInt("u_PointLightCount", pc);
    for (int i = 0; i < pc; i++) {
        std::string b = "u_PointLights[" + std::to_string(i) + "].";
        s_Shader->setVec3 (b + "position",  pointLights[i].position);
        s_Shader->setVec3 (b + "color",     pointLights[i].color);
        s_Shader->setFloat(b + "intensity", pointLights[i].intensity);
        s_Shader->setFloat(b + "radius",    pointLights[i].radius);
    }
}

void Renderer::DrawMesh(const Mesh& mesh, const glm::mat4& modelMatrix,
                         const Material& material) {
    if (!s_Shader) return;

    s_Shader->setMat4("u_Model",      modelMatrix);
    s_Shader->setVec3("u_Color",      material.albedo);

    bool useTexture   = material.texture   && material.texture->IsLoaded();
    bool useNormalMap = material.normalMap && material.normalMap->IsLoaded();

    s_Shader->setBool ("u_UseTexture",   useTexture);
    s_Shader->setBool ("u_UseNormalMap", useNormalMap);
    s_Shader->setBool ("u_UseWorldUV",   material.worldSpaceUV);
    s_Shader->setFloat("u_WorldUVTile",  material.worldUVTile);
    s_Shader->setFloat("u_Roughness",    material.roughness);
    s_Shader->setFloat("u_Metallic",     material.metallic);

#ifndef USE_DX11_BACKEND
    s_Shader->setInt("u_Texture",   0);
    s_Shader->setInt("u_NormalMap", 1);
#endif

    if (useTexture)   material.texture->Bind(0);
    if (useNormalMap) material.normalMap->Bind(1);

#ifdef USE_DX11_BACKEND
    DX11Context::GetContext()->PSSetShaderResources(3, 1, &s_SSAOBlur_SRV);
#endif

    s_Shader->use();
    mesh.Draw();
}

void Renderer::DrawSkybox(const Camera& camera, const glm::vec3& sunDirection) {
#ifdef USE_DX11_BACKEND
    if (s_DX11Sky)
        s_DX11Sky->Draw(camera.GetViewMatrix(), camera.GetProjectionMatrix(), sunDirection);
#else
    if (s_Skybox)
        s_Skybox->Draw(camera.GetViewMatrix(), camera.GetProjectionMatrix(), sunDirection);
#endif
}
