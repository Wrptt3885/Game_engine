#include "renderer/GLTFLoader.h"
#include "renderer/Mesh.h"
#include "renderer/Texture.h"
#include "renderer/MeshRenderer.h"
#include "graphics/Material.h"
#include "core/Scene.h"
#include "core/GameObject.h"
#include "core/Transform.h"

#include <stb_image.h>
#include <stb_image_write.h>

// tinygltf: supply our own stb_image (already in stb_image.cpp)
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tinygltf/tiny_gltf.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <filesystem>
#include <iostream>
#include <cstring>

namespace fs = std::filesystem;

static const std::string s_AssetDir = ASSET_DIR;

// ---- stb_image bridge for tinygltf ----------------------------------------
// tinygltf calls this for every image (both external URI and embedded binary).

static bool GltfImageLoader(tinygltf::Image* img, const int,
    std::string* err, std::string*,
    int, int,
    const unsigned char* bytes, int size, void*)
{
    int w, h, ch;
    stbi_set_flip_vertically_on_load(false); // GLTF UV: V=0 is top (DX11 convention)
    unsigned char* data = stbi_load_from_memory(bytes, size, &w, &h, &ch, 4);
    if (!data) {
        if (err) *err = stbi_failure_reason();
        return false;
    }
    img->width      = w;
    img->height     = h;
    img->component  = 4;
    img->bits       = 8;
    img->pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
    img->image.resize((size_t)w * h * 4);
    std::memcpy(img->image.data(), data, img->image.size());
    stbi_image_free(data);
    return true;
}

// ---- Accessor helpers -------------------------------------------------------

template<typename T>
static std::vector<T> ReadAccessor(const tinygltf::Model& m, int accIdx) {
    const auto& acc = m.accessors[accIdx];
    const auto& bv  = m.bufferViews[acc.bufferView];
    const auto& buf = m.buffers[bv.buffer];
    const uint8_t* base = buf.data.data() + bv.byteOffset + acc.byteOffset;
    int stride = (bv.byteStride > 0) ? (int)bv.byteStride : (int)sizeof(T);
    std::vector<T> out(acc.count);
    for (size_t i = 0; i < acc.count; i++)
        std::memcpy(&out[i], base + i * stride, sizeof(T));
    return out;
}

static std::vector<uint32_t> ReadIndices(const tinygltf::Model& m, int accIdx) {
    const auto& acc = m.accessors[accIdx];
    const auto& bv  = m.bufferViews[acc.bufferView];
    const auto& buf = m.buffers[bv.buffer];
    const uint8_t* base = buf.data.data() + bv.byteOffset + acc.byteOffset;
    std::vector<uint32_t> out(acc.count);
    for (size_t i = 0; i < acc.count; i++) {
        if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
            std::memcpy(&out[i], base + i * 4, 4);
        } else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            uint16_t v; std::memcpy(&v, base + i * 2, 2); out[i] = v;
        } else {
            out[i] = base[i];
        }
    }
    return out;
}

// ---- Tangent computation (when GLTF has none) --------------------------------

static void ComputeTangents(std::vector<Vertex>& verts, const std::vector<uint32_t>& idx) {
    for (size_t i = 0; i + 2 < idx.size(); i += 3) {
        auto& v0 = verts[idx[i]]; auto& v1 = verts[idx[i+1]]; auto& v2 = verts[idx[i+2]];
        glm::vec3 e1 = v1.position - v0.position;
        glm::vec3 e2 = v2.position - v0.position;
        glm::vec2 d1 = v1.texCoord - v0.texCoord;
        glm::vec2 d2 = v2.texCoord - v0.texCoord;
        float f = d1.x * d2.y - d2.x * d1.y;
        if (glm::abs(f) < 1e-8f) continue;
        f = 1.0f / f;
        glm::vec3 t = f * (d2.y * e1 - d1.y * e2);
        glm::vec3 b = f * (-d2.x * e1 + d1.x * e2);
        for (auto k : {idx[i], idx[i+1], idx[i+2]}) {
            verts[k].tangent   += t;
            verts[k].bitangent += b;
        }
    }
    for (auto& v : verts) {
        if (glm::length(v.tangent) > 0.001f)
            v.tangent = glm::normalize(v.tangent - glm::dot(v.tangent, v.normal) * v.normal);
        else
            v.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
        v.bitangent = glm::length(v.bitangent) > 0.001f
            ? glm::normalize(v.bitangent)
            : glm::normalize(glm::cross(v.normal, v.tangent));
    }
}

// ---- Node transform ---------------------------------------------------------

static glm::mat4 NodeMatrix(const tinygltf::Node& node) {
    if (!node.matrix.empty()) {
        glm::mat4 mat;
        for (int i = 0; i < 16; i++) mat[i/4][i%4] = (float)node.matrix[i];
        return mat;
    }
    glm::mat4 T(1.0f), R(1.0f), S(1.0f);
    if (!node.translation.empty())
        T = glm::translate(glm::mat4(1.0f), glm::vec3((float)node.translation[0], (float)node.translation[1], (float)node.translation[2]));
    if (!node.rotation.empty())
        R = glm::mat4_cast(glm::quat((float)node.rotation[3], (float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2]));
    if (!node.scale.empty())
        S = glm::scale(glm::mat4(1.0f), glm::vec3((float)node.scale[0], (float)node.scale[1], (float)node.scale[2]));
    return T * R * S;
}

// ---- Texture loading --------------------------------------------------------

static std::shared_ptr<Texture> LoadGltfTexture(
    const tinygltf::Model& model, int texIndex, const std::string& gltfDir)
{
    if (texIndex < 0) return nullptr;
    const auto& tex = model.textures[texIndex];
    if (tex.source < 0 || tex.source >= (int)model.images.size()) return nullptr;
    const auto& img = model.images[tex.source];

    // External URI: load from file
    if (!img.uri.empty() && img.uri.rfind("data:", 0) != 0) {
        std::string fullPath = gltfDir + "/" + img.uri;
        return Texture::Create(fullPath);
    }

    // Embedded (GLB binary or data URI): save to disk so paths survive serialization
    if (!img.image.empty()) {
        std::string cacheDir = s_AssetDir + "/textures/gltf_cache";
        fs::create_directories(cacheDir);
        std::string name = img.name.empty()
            ? ("tex" + std::to_string(tex.source))
            : img.name;
        // Sanitize name: replace path separators
        for (char& c : name) if (c == '/' || c == '\\') c = '_';
        std::string outPath = cacheDir + "/" + name + ".png";
        if (!fs::exists(outPath))
            stbi_write_png(outPath.c_str(), img.width, img.height, 4,
                           img.image.data(), img.width * 4);
        return Texture::Create(outPath);
    }

    return nullptr;
}

// ---- Build a single Mesh from a GLTF primitive --------------------------------

static std::shared_ptr<Mesh> BuildPrimitiveMesh(
    const tinygltf::Model& model, const tinygltf::Primitive& prim)
{
    auto it = prim.attributes.find("POSITION");
    if (it == prim.attributes.end() || prim.indices < 0) return nullptr;

    auto positions = ReadAccessor<glm::vec3>(model, it->second);
    size_t vcount  = positions.size();

    std::vector<glm::vec3> normals(vcount, glm::vec3(0.0f, 1.0f, 0.0f));
    std::vector<glm::vec2> uvs    (vcount, glm::vec2(0.0f));
    std::vector<glm::vec4> tangents4(vcount, glm::vec4(0.0f));
    bool hasTangents = false;

    if (auto nit = prim.attributes.find("NORMAL"); nit != prim.attributes.end())
        normals = ReadAccessor<glm::vec3>(model, nit->second);

    if (auto uvit = prim.attributes.find("TEXCOORD_0"); uvit != prim.attributes.end())
        uvs = ReadAccessor<glm::vec2>(model, uvit->second);

    if (auto tit = prim.attributes.find("TANGENT"); tit != prim.attributes.end()) {
        tangents4 = ReadAccessor<glm::vec4>(model, tit->second);
        hasTangents = true;
    }

    std::vector<Vertex> verts(vcount);
    for (size_t i = 0; i < vcount; i++) {
        verts[i].position = positions[i];
        verts[i].normal   = normals[i];
        verts[i].texCoord = uvs[i];
        if (hasTangents) {
            glm::vec3 t = glm::vec3(tangents4[i]);
            float     w = tangents4[i].w;
            verts[i].tangent   = t;
            verts[i].bitangent = glm::cross(normals[i], t) * w;
        }
    }

    auto indices = ReadIndices(model, prim.indices);

    if (!hasTangents)
        ComputeTangents(verts, indices);

    return Mesh::Create(verts, indices);
}

// ---- Material extraction ---------------------------------------------------

static Material BuildMaterial(
    const tinygltf::Model& model, int matIndex, const std::string& gltfDir)
{
    Material mat;
    if (matIndex < 0 || matIndex >= (int)model.materials.size()) return mat;

    const auto& m = model.materials[matIndex];
    const auto& pbr = m.pbrMetallicRoughness;

    if (pbr.baseColorFactor.size() == 4)
        mat.albedo = glm::vec3((float)pbr.baseColorFactor[0],
                               (float)pbr.baseColorFactor[1],
                               (float)pbr.baseColorFactor[2]);

    mat.metallic  = (float)pbr.metallicFactor;
    mat.roughness = (float)pbr.roughnessFactor;

    mat.texture   = LoadGltfTexture(model, pbr.baseColorTexture.index,   gltfDir);
    mat.normalMap = LoadGltfTexture(model, m.normalTexture.index,        gltfDir);

    return mat;
}

// ---- Core load logic --------------------------------------------------------

struct PrimResult {
    std::shared_ptr<Mesh>     mesh;
    Material                  material;
    glm::mat4                 transform;
    std::string               name;
};

static void CollectPrimitives(
    const tinygltf::Model& model,
    int nodeIdx,
    const glm::mat4& parentTransform,
    const std::string& gltfDir,
    std::vector<PrimResult>& out)
{
    if (nodeIdx < 0 || nodeIdx >= (int)model.nodes.size()) return;
    const auto& node = model.nodes[nodeIdx];
    glm::mat4 xform = parentTransform * NodeMatrix(node);

    if (node.mesh >= 0 && node.mesh < (int)model.meshes.size()) {
        const auto& mesh = model.meshes[node.mesh];
        for (const auto& prim : mesh.primitives) {
            auto m = BuildPrimitiveMesh(model, prim);
            if (!m) continue;
            out.push_back({
                m,
                BuildMaterial(model, prim.material, gltfDir),
                xform,
                node.name.empty() ? mesh.name : node.name
            });
        }
    }

    for (int child : node.children)
        CollectPrimitives(model, child, xform, gltfDir, out);
}

static bool LoadModel(const std::string& filepath, tinygltf::Model& model) {
    tinygltf::TinyGLTF loader;
    loader.SetImageLoader(GltfImageLoader, nullptr);

    std::string err, warn;
    bool ok;
    std::string ext = fs::path(filepath).extension().string();
    for (auto& c : ext) c = (char)tolower((unsigned char)c);

    if (ext == ".glb")
        ok = loader.LoadBinaryFromFile(&model, &err, &warn, filepath);
    else
        ok = loader.LoadASCIIFromFile (&model, &err, &warn, filepath);

    if (!warn.empty()) std::cout << "[GLTF] Warn: " << warn << "\n";
    if (!err.empty())  std::cerr << "[GLTF] Error: " << err  << "\n";
    return ok;
}

// ---- Public API -------------------------------------------------------------

std::vector<std::shared_ptr<GameObject>> GLTFLoader::Import(
    const std::string& filepath, Scene& scene)
{
    tinygltf::Model model;
    if (!LoadModel(filepath, model)) return {};

    std::string gltfDir = fs::path(filepath).parent_path().string();

    std::vector<PrimResult> prims;
    const auto& scn = model.scenes[std::max(0, model.defaultScene)];
    for (int root : scn.nodes)
        CollectPrimitives(model, root, glm::mat4(1.0f), gltfDir, prims);

    std::vector<std::shared_ptr<GameObject>> created;
    int primIdx = 0;
    for (auto& pr : prims) {
        std::string goName = pr.name.empty()
            ? fs::path(filepath).stem().string() + "_" + std::to_string(primIdx)
            : pr.name;

        auto go = scene.CreateGameObject(goName);

        // Decompose combined transform into position / rotation / scale
        glm::vec3 pos, scale, skew;
        glm::vec4 persp;
        glm::quat rot;
        glm::decompose(pr.transform, scale, rot, pos, skew, persp);
        go->GetTransform().position = pos;
        go->GetTransform().rotation = rot;
        go->GetTransform().scale    = scale;

        // Assign mesh; store serialization path "file.gltf::N"
        pr.mesh->SetPath(filepath + "::" + std::to_string(primIdx));

        auto mr = go->AddComponent<MeshRenderer>();
        mr->SetMesh(pr.mesh);
        mr->SetMaterial(pr.material);

        created.push_back(go);
        primIdx++;
    }

    std::cout << "[GLTFLoader] Imported " << primIdx << " primitive(s) from " << filepath << "\n";
    return created;
}

std::shared_ptr<Mesh> GLTFLoader::LoadMesh(const std::string& filepath) {
    // Split "path/file.gltf::N" into path + index
    size_t sep = filepath.rfind("::");
    int targetIdx = 0;
    std::string path = filepath;
    if (sep != std::string::npos) {
        targetIdx = std::stoi(filepath.substr(sep + 2));
        path      = filepath.substr(0, sep);
    }

    tinygltf::Model model;
    if (!LoadModel(path, model)) return nullptr;

    std::string gltfDir = fs::path(path).parent_path().string();

    // Enumerate primitives in order — same traversal as Import()
    std::vector<PrimResult> prims;
    const auto& scn = model.scenes[std::max(0, model.defaultScene)];
    for (int root : scn.nodes)
        CollectPrimitives(model, root, glm::mat4(1.0f), gltfDir, prims);

    if (targetIdx < 0 || targetIdx >= (int)prims.size()) return nullptr;

    auto mesh = prims[targetIdx].mesh;
    if (mesh) mesh->SetPath(filepath);
    return mesh;
}
