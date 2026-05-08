#include "core/EditorLayer.h"
#include "core/Camera.h"
#include "core/Transform.h"
#include "core/GameObject.h"

#include <imgui.h>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>

void EditorLayer::DrawGizmo(Camera& camera) {
    if (!m_Selected) return;

    ImGuiIO& io    = ImGui::GetIO();
    ImVec2 display = io.DisplaySize;

    float viewX = HIERARCHY_W;
    float viewY = MENUBAR_H;
    float viewW = display.x - HIERARCHY_W - INSPECTOR_W;
    float viewH = display.y - MENUBAR_H;

    // Keyboard shortcuts (T/R/G) when not typing into a text field
    if (!io.WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_T)) m_GizmoOp = GizmoOp::Translate;
        if (ImGui::IsKeyPressed(ImGuiKey_R)) m_GizmoOp = GizmoOp::Rotate;
        if (ImGui::IsKeyPressed(ImGuiKey_G)) m_GizmoOp = GizmoOp::Scale;
    }

    // Floating toolbar in top-left of viewport
    ImGui::SetNextWindowPos ({viewX + 8.0f, viewY + 8.0f}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.75f);
    ImGui::Begin("##GizmoToolbar", nullptr,
        ImGuiWindowFlags_NoTitleBar   | ImGuiWindowFlags_NoResize       |
        ImGuiWindowFlags_NoScrollbar  | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_AlwaysAutoResize);

    auto ModeBtn = [&](const char* label, GizmoOp op) {
        if (m_GizmoOp == op)
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (ImGui::Button(label)) m_GizmoOp = op;
        if (m_GizmoOp == op) ImGui::PopStyleColor();
        ImGui::SameLine();
    };
    ModeBtn("T##t", GizmoOp::Translate);
    ModeBtn("R##r", GizmoOp::Rotate);
    ModeBtn("S##s", GizmoOp::Scale);
    ImGui::Checkbox("Local", &m_GizmoLocal);
    ImGui::End();

    // Build model matrix
    Transform& t    = m_Selected->GetTransform();
    glm::mat4 model = t.GetMatrix();
    glm::mat4 view  = camera.GetViewMatrix();
    glm::mat4 proj  = camera.GetProjectionMatrix();

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
    ImGuizmo::SetRect(viewX, viewY, viewW, viewH);

    ImGuizmo::OPERATION op;
    switch (m_GizmoOp) {
        case GizmoOp::Rotate: op = ImGuizmo::ROTATE; break;
        case GizmoOp::Scale:  op = ImGuizmo::SCALE;  break;
        default:              op = ImGuizmo::TRANSLATE; break;
    }
    ImGuizmo::MODE mode = (m_GizmoOp == GizmoOp::Scale || m_GizmoLocal)
                          ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

    if (ImGuizmo::Manipulate(
            glm::value_ptr(view), glm::value_ptr(proj),
            op, mode, glm::value_ptr(model))) {
        // Decompose modified matrix — avoids glm::decompose conjugate bug
        t.position = glm::vec3(model[3]);
        float sx = glm::length(glm::vec3(model[0]));
        float sy = glm::length(glm::vec3(model[1]));
        float sz = glm::length(glm::vec3(model[2]));
        t.scale = glm::vec3(sx, sy, sz);
        if (sx > 0.0001f && sy > 0.0001f && sz > 0.0001f) {
            glm::mat3 rotMat(
                glm::vec3(model[0]) / sx,
                glm::vec3(model[1]) / sy,
                glm::vec3(model[2]) / sz);
            t.rotation = glm::quat_cast(rotMat);
        }
    }
}
