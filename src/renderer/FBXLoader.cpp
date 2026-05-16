#include "renderer/FBXLoader.h"
#include "renderer/Mesh.h"
#include "renderer/Texture.h"
#include "renderer/MeshRenderer.h"
#include "renderer/SkinnedMeshRenderer.h"
#include "renderer/Material.h"
#include "core/Scene.h"
#include "core/GameObject.h"
#include "core/Transform.h"

#include "ufbx.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <filesystem>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cstring>

namespace fs = std::filesystem;

// --------------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------------

static void ComputeTangents(std::vector<Vertex>& verts, const std::vector<uint32_t>& idx) {
    for (size_t i = 0; i + 2 < idx.size(); i += 3) {
        auto& v0 = verts[idx[i]], &v1 = verts[idx[i+1]], &v2 = verts[idx[i+2]];
        glm::vec3 e1 = v1.position - v0.position, e2 = v2.position - v0.position;
        glm::vec2 d1 = v1.texCoord - v0.texCoord, d2 = v2.texCoord - v0.texCoord;
        float f = d1.x*d2.y - d2.x*d1.y;
        if (glm::abs(f) < 1e-8f) continue;
        f = 1.0f / f;
        glm::vec3 tan = f*(d2.y*e1 - d1.y*e2);
        glm::vec3 bit = f*(-d2.x*e1 + d1.x*e2);
        for (auto k : {idx[i], idx[i+1], idx[i+2]}) {
            verts[k].tangent   += tan;
            verts[k].bitangent += bit;
        }
    }
    for (auto& v : verts) {
        if (glm::length(v.tangent) > 0.001f)
            v.tangent = glm::normalize(v.tangent - glm::dot(v.tangent, v.normal) * v.normal);
        else
            v.tangent = {1.0f, 0.0f, 0.0f};
        v.bitangent = glm::length(v.bitangent) > 0.001f
            ? glm::normalize(v.bitangent)
            : glm::normalize(glm::cross(v.normal, v.tangent));
    }
}

// ufbx_matrix → glm::mat4 via TRS (avoids assumptions about ufbx's internal layout)
static glm::mat4 FbxMat(const ufbx_matrix& m) {
    ufbx_transform t = ufbx_matrix_to_transform(&m);
    glm::vec3 tr{(float)t.translation.x, (float)t.translation.y, (float)t.translation.z};
    glm::quat r {(float)t.rotation.w,    (float)t.rotation.x,    (float)t.rotation.y,    (float)t.rotation.z};
    glm::vec3 s {(float)t.scale.x,       (float)t.scale.y,       (float)t.scale.z};
    return glm::translate(glm::mat4(1.0f), tr) * glm::mat4_cast(r) * glm::scale(glm::mat4(1.0f), s);
}

static void ApplyFbxTransform(Transform& out, const ufbx_node* node) {
    const ufbx_transform& t = node->local_transform;
    out.position = {(float)t.translation.x, (float)t.translation.y, (float)t.translation.z};
    out.rotation  = glm::quat{(float)t.rotation.w, (float)t.rotation.x, (float)t.rotation.y, (float)t.rotation.z};
    out.scale     = {(float)t.scale.x, (float)t.scale.y, (float)t.scale.z};
}

// --------------------------------------------------------------------------
// Mesh building
// --------------------------------------------------------------------------

static void BuildFbxGeometry(
    const ufbx_mesh* fbxMesh,
    std::vector<Vertex>&   verts,
    std::vector<uint32_t>& indices,
    const ufbx_skin_deformer* skin = nullptr)
{
    // Pre-aggregate skin weights indexed by position index for fast lookup
    struct SkinInfo { uint32_t ids[4]{}; float wts[4]{}; };
    std::vector<SkinInfo> skinData;
    if (skin) {
        size_t numPos = fbxMesh->vertex_position.values.count;
        skinData.resize(numPos);
        size_t numV = std::min(skin->vertices.count, numPos);
        for (size_t vi = 0; vi < numV; vi++) {
            const ufbx_skin_vertex& sv = skin->vertices.data[vi];
            SkinInfo& sd = skinData[vi];
            for (uint32_t wi = 0; wi < sv.num_weights && wi < 4; wi++) {
                const ufbx_skin_weight& sw = skin->weights.data[sv.weight_begin + wi];
                sd.ids[wi] = (uint32_t)sw.cluster_index;
                sd.wts[wi] = (float)sw.weight;
            }
        }
    }

    bool hasNrm = fbxMesh->vertex_normal.exists;
    bool hasUV  = fbxMesh->vertex_uv.exists;
    bool hasTan = fbxMesh->vertex_tangent.exists;
    bool hasBit = fbxMesh->vertex_bitangent.exists;

    std::vector<uint32_t> triBuffer(fbxMesh->max_face_triangles * 3);

    for (size_t fi = 0; fi < fbxMesh->faces.count; fi++) {
        ufbx_face face = fbxMesh->faces.data[fi];
        uint32_t numTris = ufbx_triangulate_face(triBuffer.data(), triBuffer.size(), fbxMesh, face);

        for (uint32_t ti = 0; ti < numTris; ti++) {
            for (uint32_t vi = 0; vi < 3; vi++) {
                uint32_t fvi = triBuffer[ti * 3 + vi];
                Vertex v;

                uint32_t posIdx = fbxMesh->vertex_position.indices.data[fvi];
                const ufbx_vec3& p = fbxMesh->vertex_position.values.data[posIdx];
                v.position = {(float)p.x, (float)p.y, (float)p.z};

                if (hasNrm) {
                    uint32_t ni = fbxMesh->vertex_normal.indices.data[fvi];
                    const ufbx_vec3& n = fbxMesh->vertex_normal.values.data[ni];
                    v.normal = {(float)n.x, (float)n.y, (float)n.z};
                }
                if (hasUV) {
                    uint32_t ui = fbxMesh->vertex_uv.indices.data[fvi];
                    const ufbx_vec2& uv = fbxMesh->vertex_uv.values.data[ui];
                    v.texCoord = {(float)uv.x, (float)uv.y};
                }
                if (hasTan) {
                    uint32_t ti2 = fbxMesh->vertex_tangent.indices.data[fvi];
                    const ufbx_vec3& t = fbxMesh->vertex_tangent.values.data[ti2];
                    v.tangent = {(float)t.x, (float)t.y, (float)t.z};
                }
                if (hasBit) {
                    uint32_t bi = fbxMesh->vertex_bitangent.indices.data[fvi];
                    const ufbx_vec3& b = fbxMesh->vertex_bitangent.values.data[bi];
                    v.bitangent = {(float)b.x, (float)b.y, (float)b.z};
                }
                if (!skinData.empty() && posIdx < skinData.size()) {
                    const SkinInfo& sd = skinData[posIdx];
                    for (int k = 0; k < 4; k++) {
                        v.boneIds[k]     = sd.ids[k];
                        v.boneWeights[k] = sd.wts[k];
                    }
                }

                indices.push_back((uint32_t)verts.size());
                verts.push_back(v);
            }
        }
    }

    if (!hasTan || !hasBit) ComputeTangents(verts, indices);
}

// --------------------------------------------------------------------------
// Texture loading
// --------------------------------------------------------------------------

static std::shared_ptr<Texture> LoadFbxTexture(
    const ufbx_texture* tex, const std::string& fbxDir)
{
    if (!tex) return nullptr;

    auto tryPath = [](const std::string& p) -> std::shared_ptr<Texture> {
        if (!p.empty() && fs::exists(p)) return Texture::Create(p);
        return nullptr;
    };

    // Relative path (portable) first, then absolute path stored in file
    if (tex->relative_filename.length > 0) {
        std::string rel = fbxDir + "/" +
            std::string(tex->relative_filename.data, tex->relative_filename.length);
        if (auto t = tryPath(rel)) return t;
    }
    if (tex->filename.length > 0) {
        if (auto t = tryPath(std::string(tex->filename.data, tex->filename.length))) return t;
    }
    return nullptr;
}

// --------------------------------------------------------------------------
// Material building
// --------------------------------------------------------------------------

static Material BuildFbxMaterial(const ufbx_mesh* fbxMesh, const std::string& fbxDir) {
    Material mat;
    if (!fbxMesh || fbxMesh->materials.count == 0 || !fbxMesh->materials.data[0])
        return mat;

    const ufbx_material* m = fbxMesh->materials.data[0];

    if (m->pbr.base_color.has_value) {
        const ufbx_vec4& c = m->pbr.base_color.value_vec4;
        mat.albedo = {(float)c.x, (float)c.y, (float)c.z};
    }
    mat.texture = LoadFbxTexture(m->pbr.base_color.texture, fbxDir);
    if (m->pbr.roughness.has_value) mat.roughness = (float)m->pbr.roughness.value_real;
    if (m->pbr.metalness.has_value) mat.metallic  = (float)m->pbr.metalness.value_real;
    return mat;
}

// --------------------------------------------------------------------------
// Skeleton building
// --------------------------------------------------------------------------

static std::shared_ptr<Skeleton> BuildFbxSkeleton(
    const ufbx_skin_deformer* skin, std::vector<ufbx_node*>& boneNodes)
{
    if (!skin || skin->clusters.count == 0) return nullptr;
    auto skel = std::make_shared<Skeleton>();

    std::unordered_map<const ufbx_node*, int> nodeToIdx;
    for (size_t ci = 0; ci < skin->clusters.count; ci++) {
        ufbx_node* bn = skin->clusters.data[ci]->bone_node;
        boneNodes.push_back(bn);
        if (bn) nodeToIdx[bn] = (int)ci;
    }

    skel->joints.resize(skin->clusters.count);
    for (size_t ci = 0; ci < skin->clusters.count; ci++) {
        const ufbx_skin_cluster* c = skin->clusters.data[ci];
        Joint& joint = skel->joints[ci];
        if (c->bone_node) {
            joint.name = std::string(c->bone_node->name.data, c->bone_node->name.length);
            auto pit = nodeToIdx.find(c->bone_node->parent);
            joint.parentIndex = (pit != nodeToIdx.end()) ? pit->second : -1;
        }
        joint.inverseBindMatrix = FbxMat(c->geometry_to_bone);
    }
    return skel;
}

// --------------------------------------------------------------------------
// Animation building (sample at 30 fps)
// --------------------------------------------------------------------------

static std::vector<AnimationClip> BuildFbxClips(
    const ufbx_scene* scene, const std::vector<ufbx_node*>& boneNodes)
{
    std::vector<AnimationClip> clips;
    if (boneNodes.empty()) return clips;

    const float fps = 30.0f;

    for (size_t si = 0; si < scene->anim_stacks.count; si++) {
        const ufbx_anim_stack* stack = scene->anim_stacks.data[si];
        float duration = (float)(stack->time_end - stack->time_begin);
        if (duration <= 0.0f) continue;

        AnimationClip clip;
        clip.name     = std::string(stack->name.data, stack->name.length);
        clip.duration = duration;

        int numFrames = (int)(duration * fps);

        for (size_t bi = 0; bi < boneNodes.size(); bi++) {
            ufbx_node* bn = boneNodes[bi];
            if (!bn) continue;

            JointTrack track;
            track.jointIndex = (int)bi;

            for (int f = 0; f <= numFrames; f++) {
                double t = stack->time_begin + f * (1.0 / fps);
                if (t > stack->time_end) t = stack->time_end;
                float ft = (float)(t - stack->time_begin);

                ufbx_transform tfm = ufbx_evaluate_transform(stack->anim, bn, t);
                track.translationTimes.push_back(ft);
                track.translations.push_back({
                    (float)tfm.translation.x, (float)tfm.translation.y, (float)tfm.translation.z});
                track.rotationTimes.push_back(ft);
                track.rotations.push_back({
                    (float)tfm.rotation.w,    (float)tfm.rotation.x,
                    (float)tfm.rotation.y,    (float)tfm.rotation.z});
                track.scaleTimes.push_back(ft);
                track.scales.push_back({
                    (float)tfm.scale.x, (float)tfm.scale.y, (float)tfm.scale.z});
            }

            if (!track.translationTimes.empty())
                clip.tracks.push_back(std::move(track));
        }

        clips.push_back(std::move(clip));
    }
    return clips;
}

// --------------------------------------------------------------------------
// Node traversal
// --------------------------------------------------------------------------

static void CollectMeshNodes(ufbx_node* node, std::vector<ufbx_node*>& out) {
    if (!node) return;
    if (node->mesh) out.push_back(node);
    for (size_t i = 0; i < node->children.count; i++)
        CollectMeshNodes(node->children.data[i], out);
}

// --------------------------------------------------------------------------
// Scene load
// --------------------------------------------------------------------------

static ufbx_scene* OpenFbx(const std::string& path) {
    ufbx_load_opts opts = {};
    opts.target_axes              = ufbx_axes_right_handed_y_up;
    opts.target_unit_meters       = 1.0f;
    opts.generate_missing_normals = true;

    ufbx_error err;
    ufbx_scene* scene = ufbx_load_file(path.c_str(), &opts, &err);
    if (!scene)
        std::cerr << "[FBXLoader] " << path << ": " << err.description.data << "\n";
    return scene;
}

// --------------------------------------------------------------------------
// Public API
// --------------------------------------------------------------------------

std::vector<std::shared_ptr<GameObject>> FBXLoader::Import(
    const std::string& filepath, Scene& scene)
{
    ufbx_scene* fbx = OpenFbx(filepath);
    if (!fbx) return {};
    std::string dir = fs::path(filepath).parent_path().string();

    std::vector<ufbx_node*> meshNodes;
    if (fbx->root_node) CollectMeshNodes(fbx->root_node, meshNodes);

    std::vector<std::shared_ptr<GameObject>> created;

    for (size_t ni = 0; ni < meshNodes.size(); ni++) {
        ufbx_node* node    = meshNodes[ni];
        ufbx_mesh* fbxMesh = node->mesh;
        std::string meshPath = filepath + "::" + std::to_string(ni);
        std::string name = (node->name.length > 0)
            ? std::string(node->name.data, node->name.length)
            : fs::path(filepath).stem().string();

        const ufbx_skin_deformer* skin =
            (fbxMesh->skin_deformers.count > 0) ? fbxMesh->skin_deformers.data[0] : nullptr;

        std::vector<Vertex>   verts;
        std::vector<uint32_t> indices;
        BuildFbxGeometry(fbxMesh, verts, indices, skin);
        if (verts.empty()) continue;

        auto mesh = Mesh::Create(verts, indices);
        mesh->SetPath(meshPath);
        Material mat = BuildFbxMaterial(fbxMesh, dir);

        auto go = scene.CreateGameObject(name);
        ApplyFbxTransform(go->GetTransform(), node);

        if (skin) {
            std::vector<ufbx_node*> boneNodes;
            auto skel  = BuildFbxSkeleton(skin, boneNodes);
            auto clips = BuildFbxClips(fbx, boneNodes);
            auto smr   = go->AddComponent<SkinnedMeshRenderer>();
            smr->SetMesh(mesh);
            smr->SetMaterial(mat);
            smr->SetSkeleton(skel);
            smr->SetClips(std::move(clips));
        } else {
            auto mr = go->AddComponent<MeshRenderer>();
            mr->SetMesh(mesh);
            mr->SetMaterial(mat);
        }

        created.push_back(go);
    }

    ufbx_free_scene(fbx);
    std::cout << "[FBXLoader] Loaded " << created.size()
              << " mesh(es) from '" << filepath << "'\n";
    return created;
}

std::shared_ptr<Mesh> FBXLoader::LoadMesh(const std::string& filepath) {
    size_t sep = filepath.rfind("::");
    std::string path = (sep != std::string::npos) ? filepath.substr(0, sep) : filepath;
    int idx          = (sep != std::string::npos) ? std::stoi(filepath.substr(sep + 2)) : 0;

    ufbx_scene* fbx = OpenFbx(path);
    if (!fbx) return nullptr;

    std::vector<ufbx_node*> nodes;
    if (fbx->root_node) CollectMeshNodes(fbx->root_node, nodes);

    std::shared_ptr<Mesh> result;
    if (idx < (int)nodes.size()) {
        std::vector<Vertex> verts;
        std::vector<uint32_t> indices;
        BuildFbxGeometry(nodes[idx]->mesh, verts, indices);
        if (!verts.empty()) {
            result = Mesh::Create(verts, indices);
            result->SetPath(filepath);
        }
    }
    ufbx_free_scene(fbx);
    return result;
}

FBXLoader::SkinnedMeshData FBXLoader::LoadSkinnedMesh(const std::string& filepath) {
    SkinnedMeshData data;
    size_t sep = filepath.rfind("::");
    std::string path = (sep != std::string::npos) ? filepath.substr(0, sep) : filepath;
    int idx          = (sep != std::string::npos) ? std::stoi(filepath.substr(sep + 2)) : 0;

    ufbx_scene* fbx = OpenFbx(path);
    if (!fbx) return data;
    std::string dir = fs::path(path).parent_path().string();

    std::vector<ufbx_node*> nodes;
    if (fbx->root_node) CollectMeshNodes(fbx->root_node, nodes);

    if (idx < (int)nodes.size()) {
        ufbx_mesh* fbxMesh = nodes[idx]->mesh;
        const ufbx_skin_deformer* skin =
            (fbxMesh->skin_deformers.count > 0) ? fbxMesh->skin_deformers.data[0] : nullptr;

        std::vector<Vertex> verts;
        std::vector<uint32_t> indices;
        BuildFbxGeometry(fbxMesh, verts, indices, skin);
        if (!verts.empty()) {
            data.mesh = Mesh::Create(verts, indices);
            data.mesh->SetPath(filepath);
        }
        data.material = BuildFbxMaterial(fbxMesh, dir);
        if (skin) {
            std::vector<ufbx_node*> boneNodes;
            data.skeleton = BuildFbxSkeleton(skin, boneNodes);
            data.clips    = BuildFbxClips(fbx, boneNodes);
        }
    }
    ufbx_free_scene(fbx);
    return data;
}

FBXLoader::ClipBundle FBXLoader::LoadClipsFromFile(const std::string& filepath) {
    ClipBundle bundle;
    ufbx_scene* fbx = OpenFbx(filepath);
    if (!fbx) return bundle;

    // Find first skinned mesh to extract skeleton + clips
    for (size_t ni = 0; ni < fbx->nodes.count; ni++) {
        ufbx_node* node = fbx->nodes.data[ni];
        if (!node->mesh || node->mesh->skin_deformers.count == 0) continue;
        const ufbx_skin_deformer* skin = node->mesh->skin_deformers.data[0];
        std::vector<ufbx_node*> boneNodes;
        bundle.skeleton = BuildFbxSkeleton(skin, boneNodes);
        bundle.clips    = BuildFbxClips(fbx, boneNodes);
        break;
    }

    ufbx_free_scene(fbx);
    return bundle;
}
