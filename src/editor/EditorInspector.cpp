#include "core/EditorLayer.h"
#include "core/GameObject.h"
#include "core/Transform.h"
#include "renderer/MeshRenderer.h"
#include "renderer/Texture.h"
#include "physics/Rigidbody.h"
#include "physics/Collider.h"
#include "graphics/Light.h"
#include "scripting/LuaScript.h"
#include "ui/UIElement.h"
#include "ui/UILabel.h"
#include "ui/UIImage.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

namespace fs = std::filesystem;

// Row label + DragFloat3
static void Field3(const char* label, glm::vec3& v, float speed = 0.1f,
                   float vmin = 0.0f, float vmax = 0.0f) {
    ImGui::PushID(label);
    ImGui::Text("%s", label);
    ImGui::SameLine(80.0f);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::DragFloat3("##v", glm::value_ptr(v), speed, vmin, vmax);
    ImGui::PopID();
}

// ---- UIElement inspector ---------------------------------------------------

static void DrawUIElementHeader(UIElement& el) {
    ImGui::Separator();
    char nameBuf[128];
    strncpy(nameBuf, el.name.c_str(), sizeof(nameBuf) - 1);
    nameBuf[sizeof(nameBuf) - 1] = '\0';
    ImGui::Text("Name");
    ImGui::SameLine(80.0f);
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::InputText("##uiname", nameBuf, sizeof(nameBuf)))
        el.name = nameBuf;

    ImGui::Checkbox("Visible", &el.visible);
    ImGui::Spacing();
    ImGui::DragFloat2("Position", &el.pos.x, 1.0f);
}

// ---- DrawInspector ---------------------------------------------------------

void EditorLayer::DrawInspector(bool isPlaying) {
    // ---- UIElement selected ------------------------------------------------
    if (m_SelectedUI) {
        ImGui::TextUnformatted(m_SelectedUI->GetTypeName());
        DrawUIElementHeader(*m_SelectedUI);
        ImGui::Separator();

        if (auto lbl = std::dynamic_pointer_cast<UILabel>(m_SelectedUI)) {
            if (ImGui::CollapsingHeader("Text", ImGuiTreeNodeFlags_DefaultOpen)) {
                char buf[256];
                strncpy(buf, lbl->text.c_str(), sizeof(buf) - 1);
                buf[sizeof(buf) - 1] = '\0';
                if (ImGui::InputText("##lbltxt", buf, sizeof(buf)))
                    lbl->text = buf;
                ImGui::ColorEdit4("Color",     &lbl->color.r);
                ImGui::DragFloat ("Font Size", &lbl->fontSize, 0.5f, 6.0f, 128.0f);
                ImGui::Checkbox  ("Centered",  &lbl->centered);
            }
        }
        else if (auto img = std::dynamic_pointer_cast<UIImage>(m_SelectedUI)) {
            if (ImGui::CollapsingHeader("Image", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Texture");
                ImGui::SameLine(80.0f);
                if (img->texture)
                    ImGui::TextDisabled("%.20s", fs::path(img->texture->GetPath()).filename().string().c_str());
                else
                    ImGui::TextDisabled("None");
                ImGui::SameLine();
                if (ImGui::SmallButton("...##uitex")) {
                    m_UIImageTexTarget = img;
                    m_TexSlot          = TexSlot::UIImageTex;
                    m_ShowTexDlg       = true;
                    m_SelectedTex      = "";
                    m_TexBrowsePath    = ASSET_DIR;
                }
                if (img->texture) {
                    ImGui::SameLine();
                    if (ImGui::SmallButton("X##clruitex")) img->texture = nullptr;
                }
                ImGui::DragFloat2("Size", &img->size.x, 1.0f, 1.0f, 4096.0f);
                ImGui::ColorEdit4("Tint", &img->tint.r);
            }
        }
        return;
    }

    // ---- GameObject selected -----------------------------------------------
    if (isPlaying) {
        ImGui::TextDisabled("Editing disabled in Play Mode");
        ImGui::Spacing();
    }
    if (!m_Selected) {
        ImGui::TextDisabled("Select an object in Hierarchy");
        return;
    }
    if (isPlaying) ImGui::BeginDisabled();

    bool active = m_Selected->IsActive();
    if (ImGui::Checkbox("##active", &active)) m_Selected->SetActive(active);
    ImGui::SameLine();
    ImGui::TextUnformatted(m_Selected->GetName().c_str());
    ImGui::Separator();

    // Transform
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        Transform& t = m_Selected->GetTransform();
        Field3("Position", t.position, 0.1f);

        glm::vec3 euler = glm::degrees(glm::eulerAngles(t.rotation));
        ImGui::Text("Rotation");
        ImGui::SameLine(80.0f);
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::DragFloat3("##rot", glm::value_ptr(euler), 1.0f))
            t.rotation = glm::quat(glm::radians(euler));

        Field3("Scale", t.scale, 0.01f, 0.001f, 100.0f);
    }

    // Components
    for (auto& comp : m_Selected->GetComponents()) {

        if (auto mr = std::dynamic_pointer_cast<MeshRenderer>(comp)) {
            if (ImGui::CollapsingHeader("MeshRenderer", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (mr->GetMesh())
                    ImGui::TextDisabled("Mesh: %.35s", mr->GetMesh()->GetPath().c_str());

                ImGui::Separator();
                Material& mat = mr->GetMaterial();

                auto OpenTexPicker = [&](TexSlot slot) {
                    m_TexTarget   = mr;
                    m_TexSlot     = slot;
                    m_ShowTexDlg  = true;
                    m_SelectedTex = "";
                    if (mr->GetMesh() && !mr->GetMesh()->GetPath().empty())
                        m_TexBrowsePath = fs::path(mr->GetMesh()->GetPath()).parent_path().string();
                };

                ImGui::Text("Albedo Tex");
                ImGui::SameLine(80.0f);
                if (mat.texture)
                    ImGui::TextDisabled("%.20s", fs::path(mat.texture->GetPath()).filename().string().c_str());
                else
                    ImGui::TextDisabled("None");
                ImGui::SameLine();
                if (ImGui::SmallButton("...##tex"))  OpenTexPicker(TexSlot::Diffuse);
                if (mat.texture) {
                    ImGui::SameLine();
                    if (ImGui::SmallButton("X##cleartex")) mat.texture = nullptr;
                }

                ImGui::Text("Normal Map");
                ImGui::SameLine(80.0f);
                if (mat.normalMap)
                    ImGui::TextDisabled("%.20s", fs::path(mat.normalMap->GetPath()).filename().string().c_str());
                else
                    ImGui::TextDisabled("None");
                ImGui::SameLine();
                if (ImGui::SmallButton("...##nrm"))  OpenTexPicker(TexSlot::NormalMap);
                if (mat.normalMap) {
                    ImGui::SameLine();
                    if (ImGui::SmallButton("X##clearnrm")) mat.normalMap = nullptr;
                }

                ImGui::Separator();
                ImGui::ColorEdit3("Albedo",    glm::value_ptr(mat.albedo));
                ImGui::DragFloat ("Roughness", &mat.roughness, 0.01f, 0.0f, 1.0f);
                ImGui::DragFloat ("Metallic",  &mat.metallic,  0.01f, 0.0f, 1.0f);
                ImGui::Checkbox  ("World UV",  &mat.worldSpaceUV);
                if (mat.worldSpaceUV)
                    ImGui::DragFloat("UV Tile", &mat.worldUVTile, 0.001f, 0.001f, 1.0f);
            }
        }

        else if (auto rb = std::dynamic_pointer_cast<Rigidbody>(comp)) {
            if (ImGui::CollapsingHeader("Rigidbody", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto col = m_Selected->GetComponent<Collider>();
                bool staticBody = col && col->isStatic;
                ImGui::LabelText("Body Type", staticBody ? "Static" : "Dynamic");
                ImGui::Separator();
                ImGui::DragFloat("Mass",        &rb->mass,        0.1f,  0.01f, 1000.0f);
                ImGui::DragFloat("Friction",    &rb->friction,    0.01f, 0.0f,  1.0f);
                ImGui::DragFloat("Restitution", &rb->restitution, 0.01f, 0.0f,  1.0f);
                ImGui::Checkbox ("Use Gravity", &rb->useGravity);
            }
        }

        else if (auto col = std::dynamic_pointer_cast<Collider>(comp)) {
            if (ImGui::CollapsingHeader("Collider", ImGuiTreeNodeFlags_DefaultOpen)) {
                Field3("Size",   col->size,   0.1f);
                Field3("Offset", col->offset, 0.1f);
                ImGui::Checkbox("Static", &col->isStatic);
            }
        }

        else if (auto ls = std::dynamic_pointer_cast<LuaScript>(comp)) {
            if (ImGui::CollapsingHeader("LuaScript", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Script");
                ImGui::SameLine(80.0f);
                if (ls->GetPath().empty())
                    ImGui::TextDisabled("None");
                else
                    ImGui::TextDisabled("%.28s", fs::path(ls->GetPath()).filename().string().c_str());
                ImGui::SameLine();
                if (ImGui::SmallButton("...##lua")) {
                    m_ScriptTarget     = ls;
                    m_ShowScriptDlg    = true;
                    m_SelectedScript   = "";
                    m_ScriptBrowsePath = std::string(ASSET_DIR) + "/scripts";
                }
                if (!ls->GetPath().empty()) {
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Reload")) ls->Reload();
                }
                if (!ls->GetPath().empty()) {
                    if (ls->IsValid())
                        ImGui::TextColored({0.3f, 1.0f, 0.3f, 1.0f}, "OK");
                    else {
                        ImGui::TextColored({1.0f, 0.35f, 0.35f, 1.0f}, "Error");
                        if (!ls->GetError().empty())
                            ImGui::TextWrapped("%s", ls->GetError().c_str());
                    }
                }
            }
        }

        else if (auto light = std::dynamic_pointer_cast<Light>(comp)) {
            if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                const char* types[] = {"Directional", "Point"};
                int idx = (light->type == LightType::Point) ? 1 : 0;
                if (ImGui::Combo("Type", &idx, types, 2))
                    light->type = idx ? LightType::Point : LightType::Directional;
                ImGui::ColorEdit3("Color",    glm::value_ptr(light->color));
                ImGui::DragFloat ("Intensity", &light->intensity, 0.01f, 0.0f, 10.0f);
                if (light->type == LightType::Directional)
                    Field3("Direction", light->direction, 0.01f);
                else
                    ImGui::DragFloat("Radius", &light->radius, 0.5f, 0.1f, 500.0f);
            }
        }
    }

    // ---- Add Component button ----------------------------------------------
    if (!isPlaying) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float btnW = ImGui::GetContentRegionAvail().x;
        if (ImGui::Button("Add Component", {btnW, 0}))
            ImGui::OpenPopup("##addcomp");

        if (ImGui::BeginPopup("##addcomp")) {
            auto& sel = m_Selected;
            if (!sel->HasComponent<MeshRenderer>()) {
                if (ImGui::MenuItem("MeshRenderer")) {
                    sel->AddComponent<MeshRenderer>();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!sel->HasComponent<Rigidbody>()) {
                if (ImGui::MenuItem("Rigidbody")) {
                    sel->AddComponent<Rigidbody>();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!sel->HasComponent<Collider>()) {
                if (ImGui::MenuItem("Collider")) {
                    auto col = sel->AddComponent<Collider>();
                    col->size = {0.5f, 0.5f, 0.5f};
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!sel->HasComponent<Light>()) {
                if (ImGui::MenuItem("Light")) {
                    sel->AddComponent<Light>();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!sel->HasComponent<LuaScript>()) {
                if (ImGui::MenuItem("LuaScript")) {
                    sel->AddComponent<LuaScript>();
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }
    }

    if (isPlaying) ImGui::EndDisabled();
}
