#include "core/SceneSerializer.h"
#include "core/Scene.h"
#include "core/GameObject.h"
#include "core/Transform.h"
#include "renderer/MeshRenderer.h"
#include "renderer/SkinnedMeshRenderer.h"
#include "physics/Rigidbody.h"
#include "physics/Collider.h"
#include "renderer/Light.h"
#include "renderer/OBJLoader.h"
#include "renderer/GLTFLoader.h"
#include "renderer/Texture.h"
#include "renderer/Material.h"
#include "audio/AudioSource.h"
#include "audio/AudioListener.h"
#include "scripting/LuaScript.h"
#include "ui/UILabel.h"
#include "ui/UIImage.h"
#include "ui/UILayer.h"
#include "ui/UIElement.h"
#include <json.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <filesystem>

using json = nlohmann::json;

// ---- path helpers ----------------------------------------------------------

static std::string s_AssetDir = ASSET_DIR;

static std::string ToRelative(const std::string& absPath) {
    if (absPath.size() > s_AssetDir.size() &&
        absPath.compare(0, s_AssetDir.size(), s_AssetDir) == 0)
        return absPath.substr(s_AssetDir.size() + 1);
    return absPath;
}

static std::string ToAbsolute(const std::string& rel) {
    if (rel.empty()) return "";
    return s_AssetDir + "/" + rel;
}

// ---- glm <-> json ----------------------------------------------------------

static json V3(const glm::vec3& v)   { return {v.x, v.y, v.z}; }
static json V4(const glm::quat& q)   { return {q.w, q.x, q.y, q.z}; }
static glm::vec3 ToV3(const json& j) { return {j[0].get<float>(), j[1].get<float>(), j[2].get<float>()}; }
static glm::quat ToQ (const json& j) { return glm::quat(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>()); }

// ---- Shared JSON builder ---------------------------------------------------

static json BuildSceneJson(Scene& scene) {
    json root;
    root["name"] = scene.GetName();

    // GameObjects
    root["gameObjects"] = json::array();
    for (size_t i = 0; i < scene.GetGameObjectCount(); i++) {
        auto obj = scene.GetGameObject(i);
        if (!obj) continue;

        json jo;
        jo["name"]   = obj->GetName();
        jo["active"] = obj->IsActive();

        const Transform& t = obj->GetTransform();
        jo["transform"] = {
            {"position", V3(t.position)},
            {"rotation", V4(t.rotation)},
            {"scale",    V3(t.scale)}
        };

        jo["components"] = json::array();

        if (auto mr = obj->GetComponent<MeshRenderer>()) {
            json jc;
            jc["type"] = "MeshRenderer";
            jc["mesh"] = mr->GetMesh() ? ToRelative(mr->GetMesh()->GetPath()) : "";
            const Material& mat = mr->GetMaterial();
            jc["material"] = {
                {"albedo",       V3(mat.albedo)},
                {"roughness",    mat.roughness},
                {"metallic",     mat.metallic},
                {"texture",      mat.texture   ? ToRelative(mat.texture->GetPath())   : ""},
                {"normalMap",    mat.normalMap  ? ToRelative(mat.normalMap->GetPath()) : ""},
                {"worldSpaceUV", mat.worldSpaceUV},
                {"worldUVTile",  mat.worldUVTile}
            };
            jo["components"].push_back(jc);
        }
        if (auto smr = obj->GetComponent<SkinnedMeshRenderer>()) {
            json jc;
            jc["type"] = "SkinnedMeshRenderer";
            jc["mesh"] = smr->GetMesh() ? ToRelative(smr->GetMesh()->GetPath()) : "";
            jc["clip"]         = smr->GetCurrentClip();
            jc["loop"]         = smr->GetLoop();
            jc["speed"]        = smr->GetSpeed();
            jc["physicsYOffset"] = smr->GetPhysicsYOffset();
            json jrules = json::array();
            for (const auto& r : smr->GetAnimRules())
                jrules.push_back({{"clip", r.clipIndex}, {"trigger", (int)r.trigger}});
            jc["animRules"] = jrules;
            json jextra = json::array();
            for (const auto& p : smr->GetExtraClipFiles())
                jextra.push_back(ToRelative(p));
            jc["extraClipFiles"] = jextra;
            const Material& mat = smr->GetMaterial();
            jc["material"] = {
                {"albedo",       V3(mat.albedo)},
                {"roughness",    mat.roughness},
                {"metallic",     mat.metallic},
                {"texture",      mat.texture   ? ToRelative(mat.texture->GetPath())   : ""},
                {"normalMap",    mat.normalMap  ? ToRelative(mat.normalMap->GetPath()) : ""},
                {"worldSpaceUV", mat.worldSpaceUV},
                {"worldUVTile",  mat.worldUVTile}
            };
            jo["components"].push_back(jc);
        }
        if (auto rb = obj->GetComponent<Rigidbody>()) {
            jo["components"].push_back({
                {"type",        "Rigidbody"},
                {"mass",        rb->mass},
                {"useGravity",  rb->useGravity},
                {"friction",    rb->material.friction},
                {"restitution", rb->material.restitution},
                {"density",     rb->material.density},
                {"hardness",    rb->material.hardness},
                {"ior",         rb->material.ior},
                {"transparency",rb->material.transparency},
                {"absorbance",  rb->material.absorbance},
                {"phase",       static_cast<int>(rb->material.phase)}
            });
        }
        if (auto col = obj->GetComponent<Collider>()) {
            jo["components"].push_back({
                {"type",     "Collider"},
                {"size",     V3(col->size)},
                {"offset",   V3(col->offset)},
                {"isStatic", col->isStatic}
            });
        }
        if (auto light = obj->GetComponent<Light>()) {
            jo["components"].push_back({
                {"type",      "Light"},
                {"lightType", light->type == LightType::Directional ? "Directional" : "Point"},
                {"color",     V3(light->color)},
                {"intensity", light->intensity},
                {"direction", V3(light->direction)},
                {"radius",    light->radius}
            });
        }
        if (auto ls = obj->GetComponent<LuaScript>()) {
            jo["components"].push_back({
                {"type", "LuaScript"},
                {"path", ls->GetPath().empty() ? "" : ToRelative(ls->GetPath())}
            });
        }
        if (auto as = obj->GetComponent<AudioSource>()) {
            jo["components"].push_back({
                {"type",        "AudioSource"},
                {"clip",        as->clipPath.empty() ? "" : ToRelative(as->clipPath)},
                {"volume",      as->volume},
                {"pitch",       as->pitch},
                {"loop",        as->loop},
                {"playOnStart", as->playOnStart},
                {"spatial",     as->spatial},
                {"minDistance", as->minDistance},
                {"maxDistance", as->maxDistance}
            });
        }
        if (obj->GetComponent<AudioListener>()) {
            jo["components"].push_back({{"type", "AudioListener"}});
        }

        root["gameObjects"].push_back(jo);
    }

    // UILayer
    root["uiLayer"] = json::array();
    for (const auto& el : scene.GetUILayer().GetElements()) {
        if (!el) continue;
        json je;
        je["type"]    = el->GetTypeName();
        je["name"]    = el->name;
        je["pos"]     = {el->pos.x, el->pos.y};
        je["visible"] = el->visible;

        if (auto lbl = std::dynamic_pointer_cast<UILabel>(el)) {
            je["text"]     = lbl->text;
            je["color"]    = {lbl->color.r, lbl->color.g, lbl->color.b, lbl->color.a};
            je["fontSize"] = lbl->fontSize;
            je["centered"] = lbl->centered;
        }
        else if (auto img = std::dynamic_pointer_cast<UIImage>(el)) {
            je["texture"] = img->texture ? ToRelative(img->texture->GetPath()) : "";
            je["size"]    = {img->size.x, img->size.y};
            je["tint"]    = {img->tint.r, img->tint.g, img->tint.b, img->tint.a};
        }
        root["uiLayer"].push_back(je);
    }

    return root;
}

// ---- Shared scene loader ---------------------------------------------------

static std::shared_ptr<Scene> SceneFromJson(const json& root) {
    auto scene = std::make_shared<Scene>(root.value("name", "Scene"));

    // GameObjects
    for (auto& jo : root["gameObjects"]) {
        auto obj = scene->CreateGameObject(jo.value("name", "GameObject"));
        obj->SetActive(jo.value("active", true));

        if (jo.contains("transform")) {
            const auto& jt = jo["transform"];
            Transform& t = obj->GetTransform();
            if (jt.contains("position")) t.position = ToV3(jt["position"]);
            if (jt.contains("rotation")) t.rotation = ToQ (jt["rotation"]);
            if (jt.contains("scale"))    t.scale    = ToV3(jt["scale"]);
        }

        if (!jo.contains("components") || !jo["components"].is_array()) continue;
        for (auto& jc : jo["components"]) {
            std::string type = jc.value("type", "");

            if (type == "MeshRenderer") {
                auto mr = obj->AddComponent<MeshRenderer>();
                std::string meshRel = jc.value("mesh", "");
                if (!meshRel.empty()) {
                    std::string basePath = meshRel;
                    size_t sep = basePath.rfind("::");
                    if (sep != std::string::npos) basePath = basePath.substr(0, sep);
                    std::string ext = std::filesystem::path(basePath).extension().string();
                    for (auto& c : ext) c = (char)tolower((unsigned char)c);
                    if (ext == ".gltf" || ext == ".glb")
                        mr->SetMesh(GLTFLoader::LoadMesh(ToAbsolute(meshRel)));
                    else
                        mr->SetMesh(OBJLoader::Load(ToAbsolute(meshRel)));
                }
                if (jc.contains("material")) {
                    const auto& jm = jc["material"];
                    Material mat;
                    if (jm.contains("albedo"))    mat.albedo    = ToV3(jm["albedo"]);
                    if (jm.contains("roughness")) mat.roughness = jm["roughness"].get<float>();
                    if (jm.contains("metallic"))  mat.metallic  = jm["metallic"].get<float>();
                    std::string texRel  = jm.value("texture",   "");
                    std::string normRel = jm.value("normalMap", "");
                    if (!texRel.empty())  mat.texture   = Texture::Create(ToAbsolute(texRel));
                    if (!normRel.empty()) mat.normalMap = Texture::Create(ToAbsolute(normRel));
                    mat.worldSpaceUV = jm.value("worldSpaceUV", false);
                    mat.worldUVTile  = jm.value("worldUVTile",  0.1f);
                    mr->SetMaterial(mat);
                }
            }
            else if (type == "SkinnedMeshRenderer") {
                auto smr = obj->AddComponent<SkinnedMeshRenderer>();
                std::string meshRel = jc.value("mesh", "");
                if (!meshRel.empty()) {
                    auto data = GLTFLoader::LoadSkinnedMesh(ToAbsolute(meshRel));
                    smr->SetMesh(data.mesh);
                    smr->SetSkeleton(data.skeleton);
                    smr->SetClips(std::move(data.clips));
                    smr->PlayClip(jc.value("clip", 0));
                    smr->SetLoop(jc.value("loop", true));
                    smr->SetSpeed(jc.value("speed", 1.0f));
                    smr->SetPhysicsYOffset(jc.value("physicsYOffset", -1.1f));
                    if (jc.contains("extraClipFiles")) {
                        for (const auto& jp : jc["extraClipFiles"]) {
                            std::string absPath = ToAbsolute(jp.get<std::string>());
                            auto bundle = GLTFLoader::LoadClipsFromFile(absPath);
                            if (!bundle.clips.empty())
                                smr->AddClipsFromFile(absPath,
                                                      bundle.skeleton,
                                                      std::move(bundle.clips));
                        }
                    }
                    if (jc.contains("animRules")) {
                        for (const auto& jr : jc["animRules"]) {
                            SkinnedMeshRenderer::AnimRule r;
                            r.clipIndex = jr.value("clip",    -1);
                            r.trigger   = (SkinnedMeshRenderer::AnimRule::Trigger)jr.value("trigger", 0);
                            smr->GetAnimRules().push_back(r);
                        }
                    }
                    if (jc.contains("material")) {
                        const auto& jm = jc["material"];
                        Material mat;
                        if (jm.contains("albedo"))    mat.albedo    = ToV3(jm["albedo"]);
                        if (jm.contains("roughness")) mat.roughness = jm["roughness"].get<float>();
                        if (jm.contains("metallic"))  mat.metallic  = jm["metallic"].get<float>();
                        std::string texRel  = jm.value("texture",   "");
                        std::string normRel = jm.value("normalMap", "");
                        if (!texRel.empty())  mat.texture   = Texture::Create(ToAbsolute(texRel));
                        if (!normRel.empty()) mat.normalMap = Texture::Create(ToAbsolute(normRel));
                        mat.worldSpaceUV = jm.value("worldSpaceUV", false);
                        mat.worldUVTile  = jm.value("worldUVTile",  0.1f);
                        smr->SetMaterial(mat);
                    }
                }
            }
            else if (type == "Rigidbody") {
                auto rb = obj->AddComponent<Rigidbody>();
                rb->mass            = jc.value("mass",        1.0f);
                rb->useGravity      = jc.value("useGravity",  true);
                rb->material.friction    = jc.value("friction",    0.2f);
                rb->material.restitution = jc.value("restitution", 0.0f);
                rb->material.density     = jc.value("density",     1000.0f);
                rb->material.hardness    = jc.value("hardness",    1.0f);
                rb->material.ior         = jc.value("ior",         1.0f);
                rb->material.transparency= jc.value("transparency",0.0f);
                rb->material.absorbance  = jc.value("absorbance",  0.0f);
                rb->material.phase       = static_cast<PhysicsMaterial::Phase>(jc.value("phase", 0));
            }
            else if (type == "Collider") {
                auto col = obj->AddComponent<Collider>();
                if (jc.contains("size"))   col->size   = ToV3(jc["size"]);
                if (jc.contains("offset")) col->offset = ToV3(jc["offset"]);
                col->isStatic = jc.value("isStatic", false);
            }
            else if (type == "Light") {
                auto light = obj->AddComponent<Light>();
                std::string lt = jc.value("lightType", "Directional");
                light->type      = (lt == "Point") ? LightType::Point : LightType::Directional;
                light->intensity = jc.value("intensity", 1.0f);
                light->radius    = jc.value("radius",    10.0f);
                if (jc.contains("color"))     light->color     = ToV3(jc["color"]);
                if (jc.contains("direction")) light->direction = ToV3(jc["direction"]);
            }
            else if (type == "LuaScript") {
                auto ls = obj->AddComponent<LuaScript>();
                std::string pathRel = jc.value("path", "");
                if (!pathRel.empty()) ls->SetPath(ToAbsolute(pathRel));
            }
            else if (type == "AudioSource") {
                auto as = obj->AddComponent<AudioSource>();
                std::string clipRel = jc.value("clip", "");
                as->clipPath    = clipRel.empty() ? "" : ToAbsolute(clipRel);
                as->volume      = jc.value("volume",      1.0f);
                as->pitch       = jc.value("pitch",       1.0f);
                as->loop        = jc.value("loop",        false);
                as->playOnStart = jc.value("playOnStart", false);
                as->spatial     = jc.value("spatial",     true);
                as->minDistance = jc.value("minDistance", 1.0f);
                as->maxDistance = jc.value("maxDistance", 50.0f);
            }
            else if (type == "AudioListener") {
                obj->AddComponent<AudioListener>();
            }
        }
    }

    // UILayer
    if (root.contains("uiLayer") && root["uiLayer"].is_array()) {
        for (auto& je : root["uiLayer"]) {
            std::string type = je.value("type", "");
            if (type == "UILabel") {
                auto lbl      = scene->GetUILayer().CreateLabel(je.value("name", "UILabel"));
                lbl->visible  = je.value("visible",  true);
                lbl->text     = je.value("text",     "Label");
                lbl->fontSize = je.value("fontSize", 16.0f);
                lbl->centered = je.value("centered", false);
                if (je.contains("pos"))   { lbl->pos.x   = je["pos"][0];   lbl->pos.y   = je["pos"][1]; }
                if (je.contains("color")) { lbl->color.r = je["color"][0]; lbl->color.g = je["color"][1];
                                            lbl->color.b = je["color"][2]; lbl->color.a = je["color"][3]; }
            }
            else if (type == "UIImage") {
                auto img     = scene->GetUILayer().CreateImage(je.value("name", "UIImage"));
                img->visible = je.value("visible", true);
                std::string texRel = je.value("texture", "");
                if (!texRel.empty()) img->texture = Texture::Create(ToAbsolute(texRel));
                if (je.contains("pos"))  { img->pos.x  = je["pos"][0];  img->pos.y  = je["pos"][1]; }
                if (je.contains("size")) { img->size.x = je["size"][0]; img->size.y = je["size"][1]; }
                if (je.contains("tint")) { img->tint.r = je["tint"][0]; img->tint.g = je["tint"][1];
                                           img->tint.b = je["tint"][2]; img->tint.a = je["tint"][3]; }
            }
        }
    }

    return scene;
}

// ---- Public API ------------------------------------------------------------

void SceneSerializer::Save(Scene& scene, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[SceneSerializer] Cannot write: " << filepath << "\n";
        return;
    }
    file << BuildSceneJson(scene).dump(2);
    std::cout << "[SceneSerializer] Saved: " << filepath << "\n";
}

std::string SceneSerializer::SaveToString(Scene& scene) {
    return BuildSceneJson(scene).dump();
}

std::shared_ptr<Scene> SceneSerializer::Load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[SceneSerializer] Cannot open: " << filepath << "\n";
        return nullptr;
    }
    json root;
    try { root = json::parse(file); }
    catch (const json::exception& e) {
        std::cerr << "[SceneSerializer] Parse error: " << e.what() << "\n";
        return nullptr;
    }
    auto scene = SceneFromJson(root);
    std::cout << "[SceneSerializer] Loaded: " << filepath
              << " (" << scene->GetGameObjectCount() << " objects)\n";
    return scene;
}

std::shared_ptr<Scene> SceneSerializer::LoadFromString(const std::string& data) {
    json root;
    try { root = json::parse(data); }
    catch (const json::exception& e) {
        std::cerr << "[SceneSerializer] Snapshot parse error: " << e.what() << "\n";
        return nullptr;
    }
    return SceneFromJson(root);
}
