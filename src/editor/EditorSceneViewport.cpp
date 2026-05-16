#include "editor/EditorLayer.h"
#include "renderer/Renderer.h"

#include <imgui.h>

bool EditorLayer::WantsCaptureMouseForCamera() const {
    // Allow camera input when the scene viewport itself is hovered, even
    // though ImGui's WantCaptureMouse returns true for that window.
    if (m_ShowSceneViewport && m_ViewportHovered) return false;
    return WantsCaptureMouse();
}

void EditorLayer::DrawSceneViewport() {
    if (!m_ShowSceneViewport) {
        m_ViewportHovered = false;
        return;
    }

    ImVec2 ds = ImGui::GetIO().DisplaySize;
    float x = HIERARCHY_W;
    float y = MENUBAR_H;
    float w = ds.x - HIERARCHY_W - INSPECTOR_W;
    float h = ds.y - MENUBAR_H - (m_ShowAssetBrowser ? ASSETBROWSER_H : 0.0f);
    if (w < 100.0f || h < 100.0f) {
        m_ViewportHovered = false;
        return;
    }

    ImGui::SetNextWindowPos ({x, y}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({w, h}, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});

    bool open = ImGui::Begin("Scene##viewport", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse);
    if (!open) {
        ImGui::End();
        ImGui::PopStyleVar();
        m_ViewportHovered = false;
        return;
    }

    ImVec2 avail = ImGui::GetContentRegionAvail();
    int vw = std::max(1, (int)avail.x);
    int vh = std::max(1, (int)avail.y);
    m_ViewportW    = vw;
    m_ViewportH    = vh;
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    m_ViewportPosX = cursor.x;
    m_ViewportPosY = cursor.y;

    void* texID = Renderer::GetViewportTextureID();
    if (texID && Renderer::GetViewportWidth() > 0 && Renderer::GetViewportHeight() > 0) {
#ifdef USE_DX11_BACKEND
        // DX11 textures: V=0 is top, normal UV
        ImGui::Image((ImTextureID)texID, {(float)vw, (float)vh},
                     {0, 0}, {1, 1});
#else
        // OpenGL textures: V=0 is bottom, flip vertically
        ImGui::Image((ImTextureID)(uintptr_t)texID, {(float)vw, (float)vh},
                     {0, 1}, {1, 0});
#endif
    }

    m_ViewportHovered = ImGui::IsWindowHovered();
    ImGui::End();
    ImGui::PopStyleVar();
}
