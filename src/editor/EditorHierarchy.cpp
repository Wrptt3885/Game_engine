#include "editor/EditorLayer.h"
#include "editor/UndoManager.h"
#include "core/Scene.h"
#include "core/GameObject.h"
#include "ui/UILabel.h"
#include "ui/UIImage.h"
#include "ui/UILayer.h"

#include <imgui.h>

void EditorLayer::DrawHierarchy(Scene& scene) {
    // ---- GameObjects section -----------------------------------------------
    ImGui::TextDisabled("GameObjects  (%zu)", scene.GetGameObjectCount());
    ImGui::SameLine();
    if (!m_IsPlaying && ImGui::SmallButton("+##newgo"))
        ImGui::OpenPopup("##creatego");

    if (ImGui::BeginPopup("##creatego")) {
        if (ImGui::MenuItem("Empty GameObject")) {
            auto go = scene.CreateGameObject("GameObject");
            m_Selected   = go;
            m_SelectedUI = nullptr;
            Scene* sc = &scene;
            UndoManager::Push("Create GameObject",
                [sc, go]{ sc->AddGameObject(go); },
                [sc, go]{ sc->DestroyGameObject(go); });
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::Separator();

    // Defer destroy to after iteration so we don't mutate the vector mid-loop
    std::shared_ptr<GameObject> goToDelete;

    for (size_t i = 0; i < scene.GetGameObjectCount(); i++) {
        auto obj = scene.GetGameObject(i);
        if (!obj) continue;

        bool selected = (m_Selected == obj);
        ImGui::PushID((int)i);
        if (ImGui::Selectable(obj->GetName().c_str(), selected)) {
            m_Selected   = obj;
            m_SelectedUI = nullptr;
        }

        if (!m_IsPlaying && ImGui::BeginPopupContextItem("##ctx")) {
            if (ImGui::MenuItem("Delete")) goToDelete = obj;
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }

    if (!m_IsPlaying && !goToDelete && m_Selected && m_Scene &&
        ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
        goToDelete = m_Selected;
    }

    if (goToDelete && m_Scene) {
        if (m_Selected == goToDelete) m_Selected = nullptr;
        Scene* sc = m_Scene;
        auto go = goToDelete;
        sc->DestroyGameObject(go);
        UndoManager::Push("Delete GameObject",
            [sc, go]{ sc->DestroyGameObject(go); },
            [sc, go]{ sc->AddGameObject(go); });
    }

    // ---- UI Layer section --------------------------------------------------
    ImGui::Spacing();
    ImGui::TextDisabled("UI Layer  (%zu)", scene.GetUILayer().GetCount());
    ImGui::SameLine();
    if (!m_IsPlaying && ImGui::SmallButton("+##newui"))
        ImGui::OpenPopup("##createui");

    if (ImGui::BeginPopup("##createui")) {
        if (ImGui::MenuItem("UILabel")) {
            int n = 0;
            for (auto& e : scene.GetUILayer().GetElements())
                if (e && std::string(e->GetTypeName()) == "UILabel") n++;
            std::string newName = n == 0 ? "UILabel" : "UILabel_" + std::to_string(n + 1);
            m_SelectedUI = scene.GetUILayer().CreateLabel(newName);
            m_Selected   = nullptr;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("UIImage")) {
            int n = 0;
            for (auto& e : scene.GetUILayer().GetElements())
                if (e && std::string(e->GetTypeName()) == "UIImage") n++;
            std::string newName = n == 0 ? "UIImage" : "UIImage_" + std::to_string(n + 1);
            m_SelectedUI = scene.GetUILayer().CreateImage(newName);
            m_Selected   = nullptr;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::Separator();

    std::shared_ptr<UIElement> toDelete;
    const auto& uiElems = scene.GetUILayer().GetElements();
    for (size_t i = 0; i < uiElems.size(); i++) {
        auto& el = uiElems[i];
        if (!el) continue;

        bool selected = (m_SelectedUI == el);
        ImGui::PushID((int)(0x8000 + i));
        std::string label = std::string("[") + el->GetTypeName() + "]  " + el->name;
        if (ImGui::Selectable(label.c_str(), selected)) {
            m_SelectedUI = el;
            m_Selected   = nullptr;
        }

        if (!m_IsPlaying && ImGui::BeginPopupContextItem("##uictx")) {
            if (ImGui::MenuItem("Delete")) {
                if (m_SelectedUI == el) m_SelectedUI = nullptr;
                toDelete = el;
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }
    if (toDelete) scene.GetUILayer().Remove(toDelete);

    // Delete key for selected UI element
    if (!m_IsPlaying && m_SelectedUI && ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
        scene.GetUILayer().Remove(m_SelectedUI);
        m_SelectedUI = nullptr;
    }
}
