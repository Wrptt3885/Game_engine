#include "editor/EditorLayer.h"
#include "editor/UndoManager.h"
#include "core/SceneSerializer.h"
#include "core/Scene.h"
#include "renderer/Renderer.h"

#include <imgui.h>
#include <GLFW/glfw3.h>

void EditorLayer::DrawMenuBar(Scene& scene, const std::string& scenePath, bool isPlaying) {
    if (!ImGui::BeginMainMenuBar()) return;

    if (ImGui::BeginMenu("Scene")) {
        if (ImGui::MenuItem("Save", "F5", false, !isPlaying))
            SceneSerializer::Save(scene, scenePath);
        ImGui::Separator();
        if (ImGui::MenuItem("Import Model...", nullptr, false, !isPlaying))
            m_ShowImportDlg = true;
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
        std::string undoLabel = "Undo";
        std::string redoLabel = "Redo";
        if (UndoManager::CanUndo()) undoLabel += " (" + UndoManager::TopUndoName() + ")";
        if (UndoManager::CanRedo()) redoLabel += " (" + UndoManager::TopRedoName() + ")";
        if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, UndoManager::CanUndo() && !isPlaying))
            UndoManager::Undo();
        if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y", false, UndoManager::CanRedo() && !isPlaying))
            UndoManager::Redo();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("Fullscreen", "F11"))
            ToggleFullscreen();
        ImGui::Separator();
        ImGui::MenuItem("Hierarchy",       nullptr, &m_ShowHierarchy);
        ImGui::MenuItem("Inspector",       nullptr, &m_ShowInspector);
        ImGui::MenuItem("Asset Browser",   nullptr, &m_ShowAssetBrowser);
        ImGui::MenuItem("Scene Viewport",  nullptr, &m_ShowSceneViewport);
        ImGui::EndMenu();
    }

    // Play / Stop button — centered
    const char* btnLabel = isPlaying ? "  Stop  " : "  Play  ";
    float btnW   = ImGui::CalcTextSize(btnLabel).x + 16.0f;
    float centerX = (ImGui::GetWindowWidth() - btnW) * 0.5f;
    ImGui::SameLine(centerX);

    if (isPlaying) {
        ImGui::PushStyleColor(ImGuiCol_Button,        {0.7f, 0.15f, 0.15f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {1.0f, 0.25f, 0.25f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {0.5f, 0.05f, 0.05f, 1.0f});
        if (ImGui::Button(btnLabel)) m_StopRequested = true;
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button,        {0.15f, 0.55f, 0.15f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.25f, 0.80f, 0.25f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {0.05f, 0.40f, 0.05f, 1.0f});
        if (ImGui::Button(btnLabel)) m_PlayRequested = true;
    }
    ImGui::PopStyleColor(3);

    // Exposure slider
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.0f);
    float exp = Renderer::GetExposure();
    if (ImGui::SliderFloat("##exp", &exp, 0.1f, 4.0f, "Exp %.2f"))
        Renderer::SetExposure(exp);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Exposure");

#ifdef USE_DX11_BACKEND
    // Bloom threshold
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.0f);
    float bt = Renderer::GetBloomThreshold();
    if (ImGui::SliderFloat("##bt", &bt, 0.1f, 3.0f, "BT %.2f"))
        Renderer::SetBloomThreshold(bt);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Bloom Threshold");

    // Bloom intensity
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.0f);
    float bi = Renderer::GetBloomIntensity();
    if (ImGui::SliderFloat("##bi", &bi, 0.0f, 2.0f, "BI %.2f"))
        Renderer::SetBloomIntensity(bi);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Bloom Intensity");
#endif

    // FPS
    char fps[32];
    snprintf(fps, sizeof(fps), "%.1f FPS", ImGui::GetIO().Framerate);
    float fpsX = ImGui::GetWindowWidth() - ImGui::CalcTextSize(fps).x - 50.0f;
    ImGui::SetCursorPosX(fpsX);
    ImGui::TextDisabled("%s", fps);

    // X close button
    ImGui::SameLine(ImGui::GetWindowWidth() - 30.0f);
    ImGui::PushStyleColor(ImGuiCol_Button,        {0.7f, 0.1f, 0.1f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {1.0f, 0.2f, 0.2f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {0.5f, 0.0f, 0.0f, 1.0f});
    if (ImGui::Button(" X "))
        glfwSetWindowShouldClose(m_Window, true);
    ImGui::PopStyleColor(3);

    ImGui::EndMainMenuBar();
}

void EditorLayer::ToggleFullscreen() {
    if (!m_Window) return;
    GLFWmonitor* monitor = glfwGetWindowMonitor(m_Window);
    if (monitor) {
        glfwSetWindowMonitor(m_Window, nullptr, m_WinX, m_WinY, m_WinW, m_WinH, 0);
    } else {
        glfwGetWindowPos (m_Window, &m_WinX, &m_WinY);
        glfwGetWindowSize(m_Window, &m_WinW, &m_WinH);
        GLFWmonitor*       primary = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode    = glfwGetVideoMode(primary);
        glfwSetWindowMonitor(m_Window, primary, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
}
