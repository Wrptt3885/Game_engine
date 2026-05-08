#include "core/EditorLayer.h"
#include "core/Scene.h"
#include "core/GameObject.h"

#include <imgui.h>

void EditorLayer::DrawHierarchy(Scene& scene) {
    ImGui::TextDisabled("%s  (%zu)", scene.GetName().c_str(), scene.GetGameObjectCount());
    ImGui::Separator();

    for (size_t i = 0; i < scene.GetGameObjectCount(); i++) {
        auto obj = scene.GetGameObject(i);
        if (!obj) continue;

        bool selected = (m_Selected == obj);
        ImGui::PushID((int)i);
        if (ImGui::Selectable(obj->GetName().c_str(), selected))
            m_Selected = obj;

        // right-click context menu — disabled during play to avoid dangling physics pointers
        if (!m_IsPlaying && ImGui::BeginPopupContextItem("##ctx")) {
            if (ImGui::MenuItem("Delete")) {
                if (m_Selected == obj) m_Selected = nullptr;
                if (m_Scene) m_Scene->DestroyGameObject(obj);
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }

    // Delete key shortcut — disabled during play
    if (!m_IsPlaying && m_Selected && m_Scene && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        m_Scene->DestroyGameObject(m_Selected);
        m_Selected = nullptr;
    }
}
