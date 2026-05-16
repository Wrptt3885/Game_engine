#include "RendererState.h"
#include "core/Scene.h"
#include "core/GameObject.h"
#include "renderer/MeshRenderer.h"
#include "renderer/SkinnedMeshRenderer.h"
#include "renderer/Light.h"
#include <glm/gtc/matrix_transform.hpp>

void Renderer::ShadowPass(Scene& scene, const glm::vec3& lightDir) {
    if (!s_DepthShader) return;

    glm::mat4 lightProj = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, 1.0f, 150.0f);
    glm::mat4 lightView = glm::lookAt(-glm::normalize(lightDir) * 80.0f,
                                       glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    s_LightSpaceMatrix  = lightProj * lightView;

#ifdef USE_DX11_BACKEND
    if (!s_DX11Shadow) return;

    auto* ctx = DX11Context::GetContext();
    GPU_MARKER(ctx, "Shadow Pass");

    s_DX11Shadow->Bind();
    s_DepthShader->use();
    s_DepthShader->setMat4("u_LightSpaceMatrix", s_LightSpaceMatrix);
    ctx->RSSetState(s_ShadowRS);

    for (size_t i = 0; i < scene.GetGameObjectCount(); i++) {
        auto obj = scene.GetGameObject(i);
        if (!obj || !obj->IsActive()) continue;

        if (auto mr = obj->GetComponent<MeshRenderer>()) {
            if (!mr->GetMesh()) continue;
            s_DepthShader->setMat4("u_Model", obj->GetTransform().GetMatrix());
            s_DepthShader->setBool("u_UseSkinning", false);
            s_DepthShader->use();
            mr->GetMesh()->Draw();
        } else if (auto smr = obj->GetComponent<SkinnedMeshRenderer>()) {
            if (!smr->GetMesh()) continue;
            s_DepthShader->setMat4("u_Model", obj->GetTransform().GetMatrix());
            s_DepthShader->setBool("u_UseSkinning", true);
            const auto& bones = smr->GetBoneMatrices();
            if (!bones.empty())
                s_DepthShader->setMat4Array("u_BoneMatrices[0]", bones.data(), (int)bones.size());
            s_DepthShader->use();
            smr->GetMesh()->Draw();
        }
    }

    ctx->RSSetState(DX11Context::GetRasterizerState());
    s_DX11Shadow->Unbind();
#else
    if (!s_ShadowMap) return;
    s_ShadowMap->Bind();
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    s_DepthShader->use();
    s_DepthShader->setMat4("u_LightSpaceMatrix", s_LightSpaceMatrix);

    for (size_t i = 0; i < scene.GetGameObjectCount(); i++) {
        auto obj = scene.GetGameObject(i);
        if (!obj || !obj->IsActive()) continue;

        if (auto mr = obj->GetComponent<MeshRenderer>()) {
            if (!mr->GetMesh()) continue;
            s_DepthShader->setBool("u_UseSkinning", false);
            s_DepthShader->setMat4("u_Model", obj->GetTransform().GetMatrix());
            mr->GetMesh()->Draw();
        } else if (auto smr = obj->GetComponent<SkinnedMeshRenderer>()) {
            if (!smr->GetMesh()) continue;
            s_DepthShader->setMat4("u_Model", obj->GetTransform().GetMatrix());
            s_DepthShader->setBool("u_UseSkinning", true);
            const auto& bones = smr->GetBoneMatrices();
            if (!bones.empty())
                s_DepthShader->setMat4Array("u_BoneMatrices[0]", bones.data(), (int)bones.size());
            smr->GetMesh()->Draw();
        }
    }

    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);
    s_ShadowMap->Unbind();
#endif
}

void Renderer::PointShadowPass(Scene& scene) {
#ifdef USE_DX11_BACKEND
    if (!s_DX11PointShadow || !s_PointDepthShader) return;

    s_HasPointShadow = false;

    // Find the first point light in the scene
    glm::vec3 lightPos;
    float farPlane = 10.0f;
    bool found = false;
    for (size_t i = 0; i < scene.GetGameObjectCount() && !found; i++) {
        auto obj = scene.GetGameObject(i);
        if (!obj || !obj->IsActive()) continue;
        auto light = obj->GetComponent<Light>();
        if (!light || light->type != LightType::Point) continue;
        lightPos = obj->GetTransform().position;
        farPlane = light->radius;
        found    = true;
    }
    if (!found) return;

    s_PointShadowLightPos = lightPos;
    s_PointShadowFarPlane = farPlane;
    s_HasPointShadow      = true;

    auto* ctx = DX11Context::GetContext();
    GPU_MARKER(ctx, "Point Shadow Pass");
    ctx->RSSetState(s_ShadowRS);

    glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, farPlane);

    static const glm::vec3 lookDirs[6] = {
        { 1, 0, 0}, {-1, 0, 0},
        { 0, 1, 0}, { 0,-1, 0},
        { 0, 0, 1}, { 0, 0,-1}
    };
    static const glm::vec3 upDirs[6] = {
        {0, 1, 0}, {0, 1, 0},
        {0, 0,-1}, {0, 0, 1},
        {0, 1, 0}, {0, 1, 0}
    };

    s_PointDepthShader->setFloat("u_FarPlane", farPlane);
    s_PointDepthShader->setVec3 ("u_LightPos", lightPos);

    for (int face = 0; face < 6; face++) {
        glm::mat4 view     = glm::lookAt(lightPos, lightPos + lookDirs[face], upDirs[face]);
        glm::mat4 viewProj = proj * view;
        s_DX11PointShadow->BindFace(face);
        s_PointDepthShader->setMat4("u_ViewProj", viewProj);

        for (size_t i = 0; i < scene.GetGameObjectCount(); i++) {
            auto obj = scene.GetGameObject(i);
            if (!obj || !obj->IsActive()) continue;
            auto mr = obj->GetComponent<MeshRenderer>();
            if (!mr || !mr->GetMesh()) continue;
            s_PointDepthShader->setMat4("u_Model", obj->GetTransform().GetMatrix());
            s_PointDepthShader->use();
            mr->GetMesh()->Draw();
        }
    }

    s_DX11PointShadow->Unbind();
    ctx->RSSetState(DX11Context::GetRasterizerState());
    const auto& vp = DX11Context::GetViewport();
    ctx->RSSetViewports(1, &vp);
#endif
}
