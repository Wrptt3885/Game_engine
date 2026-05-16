#include "editor/EditorLayer.h"
#include "core/Scene.h"
#include "core/Camera.h"
#include "core/SceneSerializer.h"
#include "ui/UILabel.h"
#include "ui/UIImage.h"

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

// ---- UI element bounding box ------------------------------------------------

static bool UIElementBounds(const std::shared_ptr<UIElement>& el,
                             ImVec2& outMin, ImVec2& outMax) {
    if (!el) return false;
    if (auto lbl = std::dynamic_pointer_cast<UILabel>(el)) {
        ImVec2 ts = ImGui::GetFont()->CalcTextSizeA(lbl->fontSize, FLT_MAX, 0.0f, lbl->text.c_str());
        float ox = lbl->centered ? lbl->pos.x - ts.x * 0.5f : lbl->pos.x;
        float oy = lbl->centered ? lbl->pos.y - ts.y * 0.5f : lbl->pos.y;
        outMin = {ox, oy};
        outMax = {ox + ts.x, oy + ts.y};
        return true;
    }
    if (auto img = std::dynamic_pointer_cast<UIImage>(el)) {
        outMin = {el->pos.x, el->pos.y};
        outMax = {el->pos.x + img->size.x, el->pos.y + img->size.y};
        return true;
    }
    return false;
}

// ---- Render (panel dispatch) ------------------------------------------------

void EditorLayer::Render(Scene& scene, Camera& camera, const std::string& scenePath, bool isPlaying) {
    m_Scene     = &scene;
    m_Camera    = &camera;
    m_IsPlaying = isPlaying;
    ImVec2 display = ImGui::GetIO().DisplaySize;

    // ---- UI element viewport interaction ------------------------------------
    ImGuiIO& io = ImGui::GetIO();
    if (!io.MouseDown[0]) m_UIDragging = false;

    if (!isPlaying) {
        if (m_UIDragging && m_SelectedUI) {
            m_SelectedUI->pos.x += io.MouseDelta.x;
            m_SelectedUI->pos.y += io.MouseDelta.y;
        } else if (!io.WantCaptureMouse && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            glm::vec2 mp = {io.MousePos.x, io.MousePos.y};
            for (auto& el : scene.GetUILayer().GetElements()) {
                if (!el || !el->visible) continue;
                ImVec2 bmin, bmax;
                if (!UIElementBounds(el, bmin, bmax)) continue;
                if (mp.x >= bmin.x && mp.x <= bmax.x && mp.y >= bmin.y && mp.y <= bmax.y) {
                    m_SelectedUI = el;
                    m_Selected   = nullptr;
                    m_UIDragging = true;
                    break;
                }
            }
        }
    }

    // ---- Selection highlight ------------------------------------------------
    if (!isPlaying && m_SelectedUI) {
        ImVec2 bmin, bmax;
        if (UIElementBounds(m_SelectedUI, bmin, bmax)) {
            ImGui::GetForegroundDrawList()->AddRect(
                {bmin.x - 2, bmin.y - 2}, {bmax.x + 2, bmax.y + 2},
                IM_COL32(255, 200, 0, 220), 0, 0, 1.5f);
        }
    }

    DrawMenuBar(scene, scenePath, isPlaying);

    if (m_ShowHierarchy) {
        ImGui::SetNextWindowPos ({0, MENUBAR_H}, ImGuiCond_Always);
        ImGui::SetNextWindowSize({HIERARCHY_W, display.y - MENUBAR_H}, ImGuiCond_Always);
        ImGui::Begin("Hierarchy", &m_ShowHierarchy,
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        DrawHierarchy(scene);
        ImGui::End();
    }

    if (m_ShowInspector) {
        ImGui::SetNextWindowPos ({display.x - INSPECTOR_W, MENUBAR_H}, ImGuiCond_Always);
        ImGui::SetNextWindowSize({INSPECTOR_W, display.y - MENUBAR_H}, ImGuiCond_Always);
        ImGui::Begin("Inspector", &m_ShowInspector,
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        DrawInspector(isPlaying);
        ImGui::End();
    }

    DrawImportDialog();
    DrawTexturePickerDialog();
    DrawScriptPickerDialog();
    DrawAddClipDialog();
    DrawAudioPickerDialog();
    if (!isPlaying) DrawGizmo(camera);
}
