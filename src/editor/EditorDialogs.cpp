#include "editor/EditorLayer.h"
#include "editor/FilePicker.h"
#include "editor/UndoManager.h"
#include "core/Scene.h"
#include "core/GameObject.h"
#include "renderer/OBJLoader.h"
#include "renderer/GLTFLoader.h"
#include "renderer/FBXLoader.h"
#include "renderer/MeshRenderer.h"
#include "renderer/SkinnedMeshRenderer.h"
#include "renderer/Texture.h"
#include "audio/AudioSource.h"
#include "scripting/LuaScript.h"
#include "ui/UIImage.h"

#include <filesystem>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

static std::string LowerExt(const std::string& path) {
    std::string ext = fs::path(path).extension().string();
    for (auto& c : ext) c = (char)std::tolower((unsigned char)c);
    return ext;
}

// ---- Import Dialog ----------------------------------------------------------

void EditorLayer::DrawImportDialog() {
    auto r = FilePicker::Draw("Import Model", m_ShowImportDlg,
                              m_BrowsePath, m_SelectedOBJ,
                              {".obj", ".gltf", ".glb", ".fbx"}, "Import");
    if (!r.applied || !m_Scene) return;

    std::string ext = LowerExt(r.path);
    std::vector<std::shared_ptr<GameObject>> created;
    if (ext == ".gltf" || ext == ".glb") {
        created = GLTFLoader::Import(r.path, *m_Scene);
    } else if (ext == ".fbx") {
        created = FBXLoader::Import(r.path, *m_Scene);
    } else {
        auto mesh = OBJLoader::Load(r.path);
        if (mesh) {
            std::string name = fs::path(r.path).stem().string();
            auto go = m_Scene->CreateGameObject(name);
            auto mr = go->AddComponent<MeshRenderer>();
            mr->SetMesh(mesh);
            created.push_back(go);
        }
    }
    if (!created.empty()) {
        m_Selected = created.front();
        Scene* sc = m_Scene;
        UndoManager::Push("Import " + fs::path(r.path).filename().string(),
            [sc, created]{ for (auto& g : created) sc->AddGameObject(g); },
            [sc, created]{ for (auto& g : created) sc->DestroyGameObject(g); });
    }
}

// ---- Texture Picker Dialog --------------------------------------------------

void EditorLayer::DrawTexturePickerDialog() {
    auto r = FilePicker::Draw("Pick Texture", m_ShowTexDlg,
                              m_TexBrowsePath, m_SelectedTex,
                              {".png", ".jpg", ".jpeg", ".bmp"}, "Apply");
    if (!r.applied) return;

    auto tex = Texture::Create(r.path);
    if (m_TexSlot == TexSlot::UIImageTex) {
        if (m_UIImageTexTarget) m_UIImageTexTarget->texture = tex;
        m_UIImageTexTarget = nullptr;
    } else {
        if (m_TexTarget) {
            if (m_TexSlot == TexSlot::NormalMap) m_TexTarget->SetNormalMap(tex);
            else                                  m_TexTarget->SetTexture(tex);
        }
        m_TexTarget = nullptr;
    }
}

// ---- Script Picker Dialog ---------------------------------------------------

void EditorLayer::DrawScriptPickerDialog() {
    auto r = FilePicker::Draw("Pick Script", m_ShowScriptDlg,
                              m_ScriptBrowsePath, m_SelectedScript,
                              {".lua"}, "Apply");
    if (!r.applied) return;

    if (m_ScriptTarget) m_ScriptTarget->SetPath(r.path);
    m_ScriptTarget = nullptr;
}

// ---- Add Clip Dialog --------------------------------------------------------

void EditorLayer::DrawAddClipDialog() {
    auto r = FilePicker::Draw("Add Animation Clip", m_ShowAddClipDlg,
                              m_AddClipBrowsePath, m_SelectedClipFile,
                              {".glb", ".gltf", ".fbx"}, "Add");
    if (!r.applied || !m_ClipTarget) return;

    std::string ext = LowerExt(r.path);
    std::shared_ptr<Skeleton>  clipSkel;
    std::vector<AnimationClip> clipClips;
    if (ext == ".fbx") {
        auto bundle = FBXLoader::LoadClipsFromFile(r.path);
        clipSkel  = std::move(bundle.skeleton);
        clipClips = std::move(bundle.clips);
    } else {
        auto bundle = GLTFLoader::LoadClipsFromFile(r.path);
        clipSkel  = std::move(bundle.skeleton);
        clipClips = std::move(bundle.clips);
    }
    if (!clipClips.empty())
        m_ClipTarget->AddClipsFromFile(r.path, clipSkel, std::move(clipClips));
    m_ClipTarget = nullptr;
}

// ---- Audio Clip Picker Dialog -----------------------------------------------

void EditorLayer::DrawAudioPickerDialog() {
    auto r = FilePicker::Draw("Pick Audio Clip", m_ShowAudioDlg,
                              m_AudioBrowsePath, m_SelectedAudio,
                              {".wav", ".mp3", ".ogg", ".flac"}, "Apply");
    if (!r.applied) return;

    if (m_AudioTarget) m_AudioTarget->clipPath = r.path;
    m_AudioTarget = nullptr;
}
