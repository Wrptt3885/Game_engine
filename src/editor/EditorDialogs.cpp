#include "core/EditorLayer.h"
#include "core/Scene.h"
#include "renderer/OBJLoader.h"
#include "renderer/GLTFLoader.h"
#include "renderer/MeshRenderer.h"
#include "renderer/Texture.h"
#include "scripting/LuaScript.h"
#include "ui/UIImage.h"

#include <imgui.h>
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

// ---- Import Dialog ----------------------------------------------------------

void EditorLayer::DrawImportDialog() {
    if (!m_ShowImportDlg) return;

    ImGui::SetNextWindowSize({560, 420}, ImGuiCond_FirstUseEver);
    ImVec2 ds = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos({ds.x * 0.5f, ds.y * 0.5f}, ImGuiCond_FirstUseEver, {0.5f, 0.5f});

    if (!ImGui::Begin("Import Model", &m_ShowImportDlg)) { ImGui::End(); return; }

    ImGui::TextDisabled("Path:");
    ImGui::SameLine();
    ImGui::TextUnformatted(m_BrowsePath.c_str());
    ImGui::Separator();

    ImGui::BeginChild("##files", {0, -60}, true);

    fs::path current(m_BrowsePath);
    if (current.has_parent_path()) {
        if (ImGui::Selectable("[..] Go up", false)) {
            m_BrowsePath  = current.parent_path().string();
            m_SelectedOBJ = "";
        }
    }

    std::vector<fs::directory_entry> dirs, models;
    try {
        for (auto& e : fs::directory_iterator(m_BrowsePath)) {
            if (e.is_directory()) {
                dirs.push_back(e);
            } else if (e.is_regular_file()) {
                std::string ext = e.path().extension().string();
                for (auto& c : ext) c = (char)tolower((unsigned char)c);
                if (ext == ".obj" || ext == ".gltf" || ext == ".glb")
                    models.push_back(e);
            }
        }
    } catch (...) {}

    auto byName = [](const fs::directory_entry& a, const fs::directory_entry& b) {
        return a.path().filename() < b.path().filename();
    };
    std::sort(dirs.begin(),   dirs.end(),   byName);
    std::sort(models.begin(), models.end(), byName);

    for (auto& d : dirs) {
        std::string label = "[+] " + d.path().filename().string();
        if (ImGui::Selectable(label.c_str(), false)) {
            m_BrowsePath  = d.path().string();
            m_SelectedOBJ = "";
        }
    }
    for (auto& f : models) {
        std::string path = f.path().string();
        bool sel = (m_SelectedOBJ == path);
        std::string label = "    " + f.path().filename().string();
        if (ImGui::Selectable(label.c_str(), sel))
            m_SelectedOBJ = path;
    }

    ImGui::EndChild();
    ImGui::Separator();

    if (m_SelectedOBJ.empty())
        ImGui::TextDisabled("No file selected");
    else
        ImGui::TextUnformatted(fs::path(m_SelectedOBJ).filename().string().c_str());

    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 130.0f);

    bool canImport = !m_SelectedOBJ.empty() && m_Scene;
    if (!canImport) ImGui::BeginDisabled();
    if (ImGui::Button("Import", {60, 0})) {
        std::string ext = fs::path(m_SelectedOBJ).extension().string();
        for (auto& c : ext) c = (char)tolower((unsigned char)c);

        if (ext == ".gltf" || ext == ".glb") {
            auto created = GLTFLoader::Import(m_SelectedOBJ, *m_Scene);
            if (!created.empty()) m_Selected = created.front();
        } else {
            auto mesh = OBJLoader::Load(m_SelectedOBJ);
            if (mesh) {
                std::string name = fs::path(m_SelectedOBJ).stem().string();
                auto go = m_Scene->CreateGameObject(name);
                auto mr = go->AddComponent<MeshRenderer>();
                mr->SetMesh(mesh);
                m_Selected = go;
            }
        }
        m_ShowImportDlg = false;
        m_SelectedOBJ   = "";
    }
    if (!canImport) ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel", {60, 0})) {
        m_ShowImportDlg = false;
        m_SelectedOBJ   = "";
    }

    ImGui::End();
}

// ---- Texture Picker Dialog --------------------------------------------------

void EditorLayer::DrawTexturePickerDialog() {
    if (!m_ShowTexDlg) return;

    ImGui::SetNextWindowSize({520, 400}, ImGuiCond_FirstUseEver);
    ImVec2 ds = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos({ds.x * 0.5f, ds.y * 0.5f}, ImGuiCond_FirstUseEver, {0.5f, 0.5f});

    if (!ImGui::Begin("Pick Texture", &m_ShowTexDlg)) { ImGui::End(); return; }

    ImGui::TextDisabled("Path:");
    ImGui::SameLine();
    ImGui::TextUnformatted(m_TexBrowsePath.c_str());
    ImGui::Separator();

    ImGui::BeginChild("##texfiles", {0, -60}, true);

    fs::path cur(m_TexBrowsePath);
    if (cur.has_parent_path()) {
        if (ImGui::Selectable("[..] Go up")) {
            m_TexBrowsePath = cur.parent_path().string();
            m_SelectedTex   = "";
        }
    }

    std::vector<fs::directory_entry> dirs, imgs;
    try {
        for (auto& e : fs::directory_iterator(m_TexBrowsePath)) {
            if (e.is_directory()) {
                dirs.push_back(e);
            } else if (e.is_regular_file()) {
                std::string ext = e.path().extension().string();
                for (auto& c : ext) c = (char)tolower((unsigned char)c);
                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp")
                    imgs.push_back(e);
            }
        }
    } catch (...) {}

    auto byName = [](const fs::directory_entry& a, const fs::directory_entry& b) {
        return a.path().filename() < b.path().filename();
    };
    std::sort(dirs.begin(), dirs.end(), byName);
    std::sort(imgs.begin(), imgs.end(), byName);

    for (auto& d : dirs) {
        std::string label = "[+] " + d.path().filename().string();
        if (ImGui::Selectable(label.c_str(), false)) {
            m_TexBrowsePath = d.path().string();
            m_SelectedTex   = "";
        }
    }
    for (auto& f : imgs) {
        std::string path = f.path().string();
        bool sel = (m_SelectedTex == path);
        std::string label = "    " + f.path().filename().string();
        if (ImGui::Selectable(label.c_str(), sel))
            m_SelectedTex = path;
    }

    ImGui::EndChild();
    ImGui::Separator();

    if (m_SelectedTex.empty())
        ImGui::TextDisabled("No file selected");
    else
        ImGui::TextUnformatted(fs::path(m_SelectedTex).filename().string().c_str());

    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 130.0f);

    bool canApply = !m_SelectedTex.empty() && (m_TexTarget || m_UIImageTexTarget);
    if (!canApply) ImGui::BeginDisabled();
    if (ImGui::Button("Apply", {60, 0})) {
        auto tex = Texture::Create(m_SelectedTex);
        if (m_TexSlot == TexSlot::UIImageTex) {
            if (m_UIImageTexTarget) m_UIImageTexTarget->texture = tex;
            m_UIImageTexTarget = nullptr;
        } else {
            if (m_TexSlot == TexSlot::NormalMap)
                m_TexTarget->SetNormalMap(tex);
            else
                m_TexTarget->SetTexture(tex);
            m_TexTarget = nullptr;
        }
        m_ShowTexDlg  = false;
        m_SelectedTex = "";
    }
    if (!canApply) ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel", {60, 0})) {
        m_TexTarget        = nullptr;
        m_UIImageTexTarget = nullptr;
        m_ShowTexDlg       = false;
        m_SelectedTex      = "";
    }

    ImGui::End();
}

// ---- Script Picker Dialog ---------------------------------------------------

void EditorLayer::DrawScriptPickerDialog() {
    if (!m_ShowScriptDlg) return;

    ImGui::SetNextWindowSize({480, 380}, ImGuiCond_FirstUseEver);
    ImVec2 ds = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos({ds.x * 0.5f, ds.y * 0.5f}, ImGuiCond_FirstUseEver, {0.5f, 0.5f});

    if (!ImGui::Begin("Pick Script", &m_ShowScriptDlg)) { ImGui::End(); return; }

    ImGui::TextDisabled("Path:");
    ImGui::SameLine();
    ImGui::TextUnformatted(m_ScriptBrowsePath.c_str());
    ImGui::Separator();

    ImGui::BeginChild("##scriptfiles", {0, -60}, true);

    fs::path cur(m_ScriptBrowsePath);
    if (cur.has_parent_path()) {
        if (ImGui::Selectable("[..] Go up")) {
            m_ScriptBrowsePath = cur.parent_path().string();
            m_SelectedScript   = "";
        }
    }

    std::vector<fs::directory_entry> dirs, scripts;
    try {
        for (auto& e : fs::directory_iterator(m_ScriptBrowsePath)) {
            if (e.is_directory()) {
                dirs.push_back(e);
            } else if (e.is_regular_file()) {
                std::string ext = e.path().extension().string();
                for (auto& c : ext) c = (char)tolower((unsigned char)c);
                if (ext == ".lua") scripts.push_back(e);
            }
        }
    } catch (...) {}

    auto byName = [](const fs::directory_entry& a, const fs::directory_entry& b) {
        return a.path().filename() < b.path().filename();
    };
    std::sort(dirs.begin(),     dirs.end(),     byName);
    std::sort(scripts.begin(),  scripts.end(),  byName);

    for (auto& d : dirs) {
        std::string label = "[+] " + d.path().filename().string();
        if (ImGui::Selectable(label.c_str(), false)) {
            m_ScriptBrowsePath = d.path().string();
            m_SelectedScript   = "";
        }
    }
    for (auto& f : scripts) {
        std::string path = f.path().string();
        bool sel = (m_SelectedScript == path);
        std::string label = "    " + f.path().filename().string();
        if (ImGui::Selectable(label.c_str(), sel))
            m_SelectedScript = path;
    }

    ImGui::EndChild();
    ImGui::Separator();

    if (m_SelectedScript.empty())
        ImGui::TextDisabled("No file selected");
    else
        ImGui::TextUnformatted(fs::path(m_SelectedScript).filename().string().c_str());

    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 130.0f);

    bool canApply = !m_SelectedScript.empty() && m_ScriptTarget;
    if (!canApply) ImGui::BeginDisabled();
    if (ImGui::Button("Apply", {60, 0})) {
        if (m_ScriptTarget) m_ScriptTarget->SetPath(m_SelectedScript);
        m_ScriptTarget    = nullptr;
        m_ShowScriptDlg   = false;
        m_SelectedScript  = "";
    }
    if (!canApply) ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel", {60, 0})) {
        m_ScriptTarget   = nullptr;
        m_ShowScriptDlg  = false;
        m_SelectedScript = "";
    }

    ImGui::End();
}
