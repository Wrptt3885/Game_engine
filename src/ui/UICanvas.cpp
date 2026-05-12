#include "ui/UICanvas.h"
#include "renderer/Texture.h"
#include <imgui.h>

float UICanvas::s_W = 0.0f;
float UICanvas::s_H = 0.0f;

void UICanvas::BeginFrame(float w, float h) {
    s_W = w;
    s_H = h;
}

void UICanvas::DrawText(const std::string& text, glm::vec2 pos,
                        glm::vec4 color, float fontSize, bool centered) {
    if (text.empty()) return;
    auto* dl   = ImGui::GetBackgroundDrawList();
    ImFont* font = ImGui::GetFont();
    ImU32 col  = ImGui::ColorConvertFloat4ToU32({color.r, color.g, color.b, color.a});
    if (centered) {
        ImVec2 sz = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text.c_str());
        pos.x -= sz.x * 0.5f;
        pos.y -= sz.y * 0.5f;
    }
    dl->AddText(font, fontSize, {pos.x, pos.y}, col, text.c_str());
}

void UICanvas::DrawRect(glm::vec2 pos, glm::vec2 size, glm::vec4 color) {
    auto* dl  = ImGui::GetBackgroundDrawList();
    ImU32 col = ImGui::ColorConvertFloat4ToU32({color.r, color.g, color.b, color.a});
    dl->AddRectFilled({pos.x, pos.y}, {pos.x + size.x, pos.y + size.y}, col);
}

void UICanvas::DrawImage(std::shared_ptr<Texture> texture, glm::vec2 pos,
                         glm::vec2 size, glm::vec4 tint) {
    if (!texture || !texture->IsLoaded()) return;
    void* id = texture->GetImTextureID();
    if (!id) return;
    auto* dl  = ImGui::GetBackgroundDrawList();
    ImU32 col = ImGui::ColorConvertFloat4ToU32({tint.r, tint.g, tint.b, tint.a});
    dl->AddImage((ImTextureID)id,
                 {pos.x, pos.y}, {pos.x + size.x, pos.y + size.y},
                 {0, 0}, {1, 1}, col);
}
