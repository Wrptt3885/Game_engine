#include "RendererState.h"
#include "core/Scene.h"
#include "core/GameObject.h"
#include "core/Camera.h"
#include "renderer/MeshRenderer.h"
#include "renderer/SkinnedMeshRenderer.h"
#include "renderer/Light.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <algorithm>

namespace {
// Pull every active GameObject with a mesh (static or skinned) from the scene,
// in a single place so passes (shadow / point-shadow / future culling) share
// one iteration shape instead of each open-coding the dispatch.
struct RenderItem {
    glm::mat4                              model;
    Mesh*                                  mesh;
    const std::vector<glm::mat4>*          bones;  // null = static mesh
};

template <typename Fn>
void ForEachRenderable(Scene& scene, Fn&& fn) {
    for (size_t i = 0; i < scene.GetGameObjectCount(); i++) {
        auto obj = scene.GetGameObject(i);
        if (!obj || !obj->IsActive()) continue;

        if (auto mr = obj->GetComponent<MeshRenderer>()) {
            if (!mr->GetMesh()) continue;
            RenderItem it{obj->GetTransform().GetMatrix(), mr->GetMesh().get(), nullptr};
            fn(it);
        } else if (auto smr = obj->GetComponent<SkinnedMeshRenderer>()) {
            if (!smr->GetMesh()) continue;
            RenderItem it{obj->GetTransform().GetMatrix(), smr->GetMesh().get(), &smr->GetBoneMatrices()};
            fn(it);
        }
    }
}
} // namespace

namespace {
// First active directional light in the scene, or nullptr if none.
Light* FindShadowCaster(Scene& scene) {
    for (size_t i = 0; i < scene.GetGameObjectCount(); i++) {
        auto obj = scene.GetGameObject(i);
        if (!obj || !obj->IsActive()) continue;
        auto light = obj->GetComponent<Light>();
        if (light && light->type == LightType::Directional)
            return light.get();
    }
    return nullptr;
}

// Build a light-space view+ortho centered on the camera. Uses a sphere of
// fixed world-space radius (rotation-invariant in light space) so the shadow
// texel density stays constant regardless of camera angle / scene size.
// A full camera-frustum fit would let the far plane (default 500) blow the
// ortho box up so badly that the 4096^2 shadow map gives ~8 texels/unit,
// making the 16-sample PCF blur the shadow away entirely.
glm::mat4 BuildLightSpaceMatrix(const Camera& cam, const glm::vec3& lightDir) {
    const float kShadowRadius = 30.0f;   // half-extent of shadow coverage (world units)
    const float kForwardBias  = 0.4f;    // push shadow region ahead of camera

    glm::vec3 center = cam.GetPosition() + cam.GetLookForward() * (kShadowRadius * kForwardBias);

    glm::vec3 L  = glm::normalize(lightDir);
    glm::vec3 up = (std::abs(L.y) > 0.99f) ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
    glm::mat4 lightView = glm::lookAt(center - L * 100.0f, center, up);

    // Tight near/far around the caster sphere (centered at depth 100 from
    // light cam, radius kShadowRadius) keeps the depth-bias-in-world small —
    // a wide near/far span turns bias×range into very visible peter-panning.
    float r = kShadowRadius;
    float zNear = 100.0f - kShadowRadius - 20.0f;  // some padding for tall casters
    float zFar  = 100.0f + kShadowRadius + 20.0f;
    return glm::ortho(-r, r, -r, r, zNear, zFar) * lightView;
}
} // namespace

void Renderer::ShadowPass(Scene& scene, const Camera& camera) {
    if (!s_DepthShader) return;

    Light* caster = FindShadowCaster(scene);
    if (!caster) return;  // no directional light → no shadow this frame

    s_LightSpaceMatrix = BuildLightSpaceMatrix(camera, caster->direction);

#ifdef USE_DX11_BACKEND
    if (!s_DX11Shadow) return;

    auto* ctx = DX11Context::GetContext();
    GPU_MARKER(ctx, "Shadow Pass");

    s_DX11Shadow->Bind();
    s_DepthShader->use();
    s_DepthShader->setMat4("u_LightSpaceMatrix", s_LightSpaceMatrix);
    ctx->RSSetState(s_ShadowRS);

    ForEachRenderable(scene, [](const RenderItem& it) {
        s_DepthShader->setMat4("u_Model", it.model);
        if (it.bones && !it.bones->empty()) {
            s_DepthShader->setBool("u_UseSkinning", true);
            s_DepthShader->setMat4Array("u_BoneMatrices[0]", it.bones->data(), (int)it.bones->size());
        } else {
            s_DepthShader->setBool("u_UseSkinning", false);
        }
        s_DepthShader->use();
        it.mesh->Draw();
    });

    ctx->RSSetState(DX11Context::GetRasterizerState());
    s_DX11Shadow->Unbind();
#else
    if (!s_ShadowMap) return;
    s_ShadowMap->Bind();
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    s_DepthShader->use();
    s_DepthShader->setMat4("u_LightSpaceMatrix", s_LightSpaceMatrix);

    ForEachRenderable(scene, [](const RenderItem& it) {
        s_DepthShader->setMat4("u_Model", it.model);
        if (it.bones && !it.bones->empty()) {
            s_DepthShader->setBool("u_UseSkinning", true);
            s_DepthShader->setMat4Array("u_BoneMatrices[0]", it.bones->data(), (int)it.bones->size());
        } else {
            s_DepthShader->setBool("u_UseSkinning", false);
        }
        it.mesh->Draw();
    });

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

        ForEachRenderable(scene, [](const RenderItem& it) {
            s_PointDepthShader->setMat4("u_Model", it.model);
            if (it.bones && !it.bones->empty()) {
                s_PointDepthShader->setBool("u_UseSkinning", true);
                s_PointDepthShader->setMat4Array("u_BoneMatrices[0]", it.bones->data(), (int)it.bones->size());
            } else {
                s_PointDepthShader->setBool("u_UseSkinning", false);
            }
            s_PointDepthShader->use();
            it.mesh->Draw();
        });
    }

    s_DX11PointShadow->Unbind();
    ctx->RSSetState(DX11Context::GetRasterizerState());
    const auto& vp = DX11Context::GetViewport();
    ctx->RSSetViewports(1, &vp);
#endif
}
