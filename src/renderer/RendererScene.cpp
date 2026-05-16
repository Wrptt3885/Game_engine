#include "RendererState.h"
#include "renderer/Mesh.h"
#include "renderer/MeshRenderer.h"
#include "core/Camera.h"
#include "renderer/Material.h"
#include "renderer/LightData.h"
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

    // Uniform names precomputed once — avoids std::string allocs per frame
    static const char* kDirFields[4][3] = {
        {"u_DirLights[0].direction","u_DirLights[0].color","u_DirLights[0].intensity"},
        {"u_DirLights[1].direction","u_DirLights[1].color","u_DirLights[1].intensity"},
        {"u_DirLights[2].direction","u_DirLights[2].color","u_DirLights[2].intensity"},
        {"u_DirLights[3].direction","u_DirLights[3].color","u_DirLights[3].intensity"},
    };
    static const char* kPointFields[8][4] = {
        {"u_PointLights[0].position","u_PointLights[0].color","u_PointLights[0].intensity","u_PointLights[0].radius"},
        {"u_PointLights[1].position","u_PointLights[1].color","u_PointLights[1].intensity","u_PointLights[1].radius"},
        {"u_PointLights[2].position","u_PointLights[2].color","u_PointLights[2].intensity","u_PointLights[2].radius"},
        {"u_PointLights[3].position","u_PointLights[3].color","u_PointLights[3].intensity","u_PointLights[3].radius"},
        {"u_PointLights[4].position","u_PointLights[4].color","u_PointLights[4].intensity","u_PointLights[4].radius"},
        {"u_PointLights[5].position","u_PointLights[5].color","u_PointLights[5].intensity","u_PointLights[5].radius"},
        {"u_PointLights[6].position","u_PointLights[6].color","u_PointLights[6].intensity","u_PointLights[6].radius"},
        {"u_PointLights[7].position","u_PointLights[7].color","u_PointLights[7].intensity","u_PointLights[7].radius"},
    };

    int dc = std::min((int)dirLights.size(), 4);
    s_Shader->setInt("u_DirLightCount", dc);
    for (int i = 0; i < dc; i++) {
        s_Shader->setVec3 (kDirFields[i][0], dirLights[i].direction);
        s_Shader->setVec3 (kDirFields[i][1], dirLights[i].color);
        s_Shader->setFloat(kDirFields[i][2], dirLights[i].intensity);
    }

    int pc = std::min((int)pointLights.size(), 8);
    s_Shader->setInt("u_PointLightCount", pc);
    for (int i = 0; i < pc; i++) {
        s_Shader->setVec3 (kPointFields[i][0], pointLights[i].position);
        s_Shader->setVec3 (kPointFields[i][1], pointLights[i].color);
        s_Shader->setFloat(kPointFields[i][2], pointLights[i].intensity);
        s_Shader->setFloat(kPointFields[i][3], pointLights[i].radius);
    }

#ifdef USE_DX11_BACKEND
    // SSAO ambient occlusion — bind once per scene, not per draw call
    auto* ctx = DX11Context::GetContext();
    ctx->PSSetShaderResources(3, 1, &s_SSAOBlur_SRV);
    // IBL — irradiance (t5), prefilter (t6), BRDF LUT (t7); null when not ready
    ctx->PSSetShaderResources(5, 1, &s_Irradiance_SRV);
    ctx->PSSetShaderResources(6, 1, &s_Prefilter_SRV);
    ctx->PSSetShaderResources(7, 1, &s_BrdfLUT_SRV);
#endif
}

void Renderer::DrawMesh(const Mesh& mesh, const glm::mat4& modelMatrix,
                         const Material& material,
                         const glm::mat4* boneMatrices, int boneCount) {
    if (!s_Shader) return;

    s_Shader->setMat4("u_Model",      modelMatrix);
    s_Shader->setVec3("u_Color",      material.albedo);

    if (boneMatrices && boneCount > 0) {
        s_Shader->setBool    ("u_UseSkinning",     true);
        s_Shader->setMat4Array("u_BoneMatrices[0]", boneMatrices, std::min(boneCount, 100));
    } else {
        s_Shader->setBool("u_UseSkinning", false);
    }

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
