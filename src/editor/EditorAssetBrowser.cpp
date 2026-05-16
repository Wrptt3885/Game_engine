#include "editor/EditorLayer.h"
#include "editor/UndoManager.h"
#include "core/Scene.h"
#include "core/GameObject.h"
#include "renderer/OBJLoader.h"
#include "renderer/GLTFLoader.h"
#include "renderer/FBXLoader.h"
#include "renderer/MeshRenderer.h"
#include "renderer/Texture.h"
#include "audio/AudioSource.h"
#include "scripting/LuaScript.h"

#include <imgui.h>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <vector>
#include <string>

namespace fs = std::filesystem;

namespace {

enum AssetKind { K_Dir, K_Model, K_Texture, K_Script, K_Audio, K_Scene, K_Other };

AssetKind ClassifyExt(const std::string& extLower) {
    if (extLower == ".obj"  || extLower == ".gltf" || extLower == ".glb" || extLower == ".fbx")
        return K_Model;
    if (extLower == ".png"  || extLower == ".jpg"  || extLower == ".jpeg" || extLower == ".bmp" ||
        extLower == ".tga"  || extLower == ".hdr")
        return K_Texture;
    if (extLower == ".lua") return K_Script;
    if (extLower == ".wav" || extLower == ".mp3" || extLower == ".ogg" || extLower == ".flac")
        return K_Audio;
    if (extLower == ".json") return K_Scene;
    return K_Other;
}

bool MatchesFilter(AssetKind k, int filter) {
    if (filter == 0) return k != K_Other;
    if (filter == 1) return k == K_Model;
    if (filter == 2) return k == K_Texture;
    if (filter == 3) return k == K_Script;
    if (filter == 4) return k == K_Audio;
    if (filter == 5) return k == K_Scene;
    return true;
}

const char* IconFor(AssetKind k) {
    switch (k) {
        case K_Dir:     return "[+] ";
        case K_Model:   return "[M] ";
        case K_Texture: return "[T] ";
        case K_Script:  return "[L] ";
        case K_Audio:   return "[A] ";
        case K_Scene:   return "[S] ";
        default:        return "    ";
    }
}

ImVec4 ColorFor(AssetKind k) {
    switch (k) {
        case K_Dir:     return {0.75f, 0.85f, 1.0f,  1.0f};
        case K_Model:   return {0.55f, 0.95f, 0.55f, 1.0f};
        case K_Texture: return {1.0f,  0.80f, 0.45f, 1.0f};
        case K_Script:  return {0.90f, 0.60f, 1.0f,  1.0f};
        case K_Audio:   return {1.0f,  0.50f, 0.50f, 1.0f};
        case K_Scene:   return {0.55f, 0.85f, 1.0f,  1.0f};
        default:        return {0.7f,  0.7f,  0.7f,  1.0f};
    }
}

} // namespace

void EditorLayer::DrawAssetBrowser(bool isPlaying) {
    if (!m_ShowAssetBrowser) return;
    if (m_AssetBrowsePath.empty()) m_AssetBrowsePath = ASSET_DIR;

    ImVec2 ds = ImGui::GetIO().DisplaySize;
    float x = HIERARCHY_W;
    float w = ds.x - HIERARCHY_W - INSPECTOR_W;
    if (w < 100.0f) return;
    float y = ds.y - ASSETBROWSER_H;
    ImGui::SetNextWindowPos ({x, y},                ImGuiCond_Always);
    ImGui::SetNextWindowSize({w, ASSETBROWSER_H},   ImGuiCond_Always);

    if (!ImGui::Begin("Assets", &m_ShowAssetBrowser,
                      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                      ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }

    // ---- breadcrumb / nav row ------------------------------------------------
    fs::path root(ASSET_DIR);
    fs::path cur(m_AssetBrowsePath);

    if (ImGui::SmallButton("Home")) m_AssetBrowsePath = ASSET_DIR;
    ImGui::SameLine();

    bool atRoot = (fs::weakly_canonical(cur) == fs::weakly_canonical(root));
    if (atRoot) ImGui::BeginDisabled();
    if (ImGui::SmallButton("Up")) {
        if (cur.has_parent_path()) {
            m_AssetBrowsePath = cur.parent_path().string();
            m_AssetSelected.clear();
        }
    }
    if (atRoot) ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    ImGui::TextUnformatted(m_AssetBrowsePath.c_str());

    // Filter dropdown, right-aligned
    const float kFilterW = 120.0f;
    float rightX = ImGui::GetWindowContentRegionMax().x - kFilterW;
    if (rightX > ImGui::GetCursorPosX()) {
        ImGui::SameLine();
        ImGui::SetCursorPosX(rightX);
    } else {
        ImGui::SameLine();
    }
    ImGui::SetNextItemWidth(kFilterW);
    const char* kFilters[] = { "All", "Models", "Textures", "Scripts", "Audio", "Scenes" };
    ImGui::Combo("##assetfilter", &m_AssetFilter, kFilters, IM_ARRAYSIZE(kFilters));

    ImGui::Separator();

    // ---- file grid -----------------------------------------------------------
    struct Entry { fs::path path; AssetKind kind; };
    std::vector<Entry> entries;
    try {
        for (auto& e : fs::directory_iterator(m_AssetBrowsePath)) {
            if (e.is_directory()) {
                entries.push_back({e.path(), K_Dir});
            } else if (e.is_regular_file()) {
                std::string ext = e.path().extension().string();
                for (auto& c : ext) c = (char)tolower((unsigned char)c);
                AssetKind k = ClassifyExt(ext);
                if (k == K_Other) continue;
                if (!MatchesFilter(k, m_AssetFilter)) continue;
                entries.push_back({e.path(), k});
            }
        }
    } catch (...) {}

    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
        if (a.kind == K_Dir && b.kind != K_Dir) return true;
        if (a.kind != K_Dir && b.kind == K_Dir) return false;
        return a.path.filename() < b.path.filename();
    });

    ImGui::BeginChild("##assetfiles", {0, 0}, false);

    // Multi-column layout
    float avail = ImGui::GetContentRegionAvail().x;
    int cols = std::max(1, (int)(avail / 150.0f));
    if (ImGui::BeginTable("##assetgrid", cols,
                          ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_SizingStretchSame)) {
        for (auto& e : entries) {
            ImGui::TableNextColumn();
            std::string label = std::string(IconFor(e.kind)) + e.path.filename().string();
            bool selected = (m_AssetSelected == e.path.string());

            ImGui::PushStyleColor(ImGuiCol_Text, ColorFor(e.kind));
            if (ImGui::Selectable(label.c_str(), selected,
                                  ImGuiSelectableFlags_AllowDoubleClick)) {
                m_AssetSelected = e.path.string();
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    // Double-click action
                    if (e.kind == K_Dir) {
                        m_AssetBrowsePath = e.path.string();
                        m_AssetSelected.clear();
                    } else if (!isPlaying) {
                        std::string path = e.path.string();
                        std::string ext  = e.path.extension().string();
                        for (auto& c : ext) c = (char)tolower((unsigned char)c);

                        if (e.kind == K_Model && m_Scene) {
                            std::vector<std::shared_ptr<GameObject>> created;
                            if (ext == ".gltf" || ext == ".glb") {
                                created = GLTFLoader::Import(path, *m_Scene);
                            } else if (ext == ".fbx") {
                                created = FBXLoader::Import(path, *m_Scene);
                            } else {
                                auto mesh = OBJLoader::Load(path);
                                if (mesh) {
                                    std::string name = e.path.stem().string();
                                    auto go = m_Scene->CreateGameObject(name);
                                    auto mr = go->AddComponent<MeshRenderer>();
                                    mr->SetMesh(mesh);
                                    created.push_back(go);
                                }
                            }
                            if (!created.empty()) {
                                m_Selected = created.front();
                                Scene* sc = m_Scene;
                                UndoManager::Push("Import " + e.path.filename().string(),
                                    [sc, created]{ for (auto& g : created) sc->AddGameObject(g); },
                                    [sc, created]{ for (auto& g : created) sc->DestroyGameObject(g); });
                            }
                        } else if (e.kind == K_Texture && m_Selected) {
                            auto mr = m_Selected->GetComponent<MeshRenderer>();
                            if (mr) mr->SetTexture(Texture::Create(path));
                        } else if (e.kind == K_Script && m_Selected) {
                            auto sc = m_Selected->GetComponent<LuaScript>();
                            if (!sc) sc = m_Selected->AddComponent<LuaScript>();
                            sc->SetPath(path);
                        } else if (e.kind == K_Audio && m_Selected) {
                            auto as = m_Selected->GetComponent<AudioSource>();
                            if (!as) as = m_Selected->AddComponent<AudioSource>();
                            as->clipPath = path;
                        }
                    }
                }
            }
            ImGui::PopStyleColor();

            // Tooltip with full path
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(e.path.filename().string().c_str());
                ImGui::Separator();
                ImGui::TextDisabled("%s", e.path.string().c_str());
                if (e.kind != K_Dir && !isPlaying) {
                    ImGui::Separator();
                    if      (e.kind == K_Model)   ImGui::TextDisabled("Double-click: import to scene");
                    else if (e.kind == K_Texture) ImGui::TextDisabled("Double-click: apply texture to selected");
                    else if (e.kind == K_Script)  ImGui::TextDisabled("Double-click: attach script to selected");
                    else if (e.kind == K_Audio)   ImGui::TextDisabled("Double-click: assign audio to selected");
                }
                ImGui::EndTooltip();
            }
        }
        ImGui::EndTable();
    }

    ImGui::EndChild();
    ImGui::End();
}
