#include "core/EditorLayer.h"
#include "core/Scene.h"
#include "core/Camera.h"
#include "core/SceneSerializer.h"

#include <imgui.h>
#include <ImGuizmo.h>
#include <backends/imgui_impl_glfw.h>
#ifdef USE_DX11_BACKEND
#  include <backends/imgui_impl_dx11.h>
#  include "rhi/dx11/DX11Context.h"
#else
#  include <backends/imgui_impl_opengl3.h>
#endif
#include <GLFW/glfw3.h>

// ---- Init / Shutdown --------------------------------------------------------

void EditorLayer::Init(GLFWwindow* window) {
    m_Window        = window;
    m_BrowsePath    = ASSET_DIR;
    m_TexBrowsePath = ASSET_DIR;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename  = nullptr;

    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding   = 4.0f;
    s.FrameRounding    = 3.0f;
    s.GrabRounding     = 3.0f;
    s.WindowBorderSize = 1.0f;
    s.Colors[ImGuiCol_Header]        = {0.20f, 0.45f, 0.70f, 0.80f};
    s.Colors[ImGuiCol_HeaderHovered] = {0.26f, 0.59f, 0.98f, 0.80f};
    s.Colors[ImGuiCol_HeaderActive]  = {0.26f, 0.59f, 0.98f, 1.00f};

#ifdef USE_DX11_BACKEND
    ImGui_ImplGlfw_InitForOther(window, true);
    ImGui_ImplDX11_Init(DX11Context::GetDevice(), DX11Context::GetContext());
#else
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
#endif
}

void EditorLayer::Shutdown() {
#ifdef USE_DX11_BACKEND
    ImGui_ImplDX11_Shutdown();
#else
    ImGui_ImplOpenGL3_Shutdown();
#endif
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

// ---- Frame ------------------------------------------------------------------

void EditorLayer::BeginFrame() {
#ifdef USE_DX11_BACKEND
    ImGui_ImplDX11_NewFrame();
#else
    ImGui_ImplOpenGL3_NewFrame();
#endif
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

void EditorLayer::EndFrame() {
    ImGui::Render();
#ifdef USE_DX11_BACKEND
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#else
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif
}

bool EditorLayer::WantsCaptureMouse()    const { return ImGui::GetIO().WantCaptureMouse; }
bool EditorLayer::WantsCaptureKeyboard() const { return ImGui::GetIO().WantCaptureKeyboard; }

// ---- Render (panel dispatch) ------------------------------------------------

void EditorLayer::Render(Scene& scene, Camera& camera, const std::string& scenePath, bool isPlaying) {
    m_Scene     = &scene;
    m_Camera    = &camera;
    m_IsPlaying = isPlaying;
    ImVec2 display = ImGui::GetIO().DisplaySize;

    DrawMenuBar(scene, scenePath, isPlaying);

    ImGui::SetNextWindowPos ({0, MENUBAR_H}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({HIERARCHY_W, display.y - MENUBAR_H}, ImGuiCond_Always);
    ImGui::Begin("Hierarchy", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    DrawHierarchy(scene);
    ImGui::End();

    ImGui::SetNextWindowPos ({display.x - INSPECTOR_W, MENUBAR_H}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({INSPECTOR_W, display.y - MENUBAR_H}, ImGuiCond_Always);
    ImGui::Begin("Inspector", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    DrawInspector(isPlaying);
    ImGui::End();

    DrawImportDialog();
    DrawTexturePickerDialog();
    if (!isPlaying) DrawGizmo(camera);
}
