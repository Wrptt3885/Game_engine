#pragma once

#include <memory>
#include <string>
#include <vector>

struct GLFWwindow;
class Scene;
class Camera;
class GameObject;
class MeshRenderer;
class SkinnedMeshRenderer;
class AudioSource;
class UIImage;
class UIElement;
class LuaScript;

enum class GizmoOp { Translate, Rotate, Scale };

class EditorLayer {
public:
    void Init(GLFWwindow* window);
    void Shutdown();

    void BeginFrame();
    void Render(Scene& scene, Camera& camera, const std::string& scenePath, bool isPlaying = false);
    void EndFrame();

    bool WantsCaptureMouse()    const;
    bool WantsCaptureKeyboard() const;
    void ToggleFullscreen();

    // Scene viewport (render-to-texture mode)
    bool IsSceneViewportEnabled() const { return m_ShowSceneViewport; }
    bool IsSceneViewportHovered() const { return m_ShowSceneViewport && m_ViewportHovered; }
    bool WantsCaptureMouseForCamera() const;  // false when hovering scene viewport
    int  GetSceneViewportWidth()  const { return m_ViewportW; }
    int  GetSceneViewportHeight() const { return m_ViewportH; }

    // Polled each frame by Application::Update
    bool ConsumePlayRequest()  { bool v = m_PlayRequested;  m_PlayRequested  = false; return v; }
    bool ConsumeStopRequest()  { bool v = m_StopRequested;  m_StopRequested  = false; return v; }
    void ClearSelection()      { m_Selected = nullptr; m_SelectedUI = nullptr;
                               m_ClipTarget = nullptr; m_ShowAddClipDlg = false; }

    // Layout constants shared across panel files
    static constexpr float HIERARCHY_W   = 200.0f;
    static constexpr float INSPECTOR_W   = 280.0f;
    static constexpr float MENUBAR_H     = 20.0f;
    static constexpr float ASSETBROWSER_H = 200.0f;

private:
    void DrawMenuBar(Scene& scene, const std::string& scenePath, bool isPlaying);
    void DrawHierarchy(Scene& scene);
    void DrawInspector(bool isPlaying);
    void DrawAssetBrowser(bool isPlaying);
    void DrawSceneViewport();
    void DrawImportDialog();
    void DrawTexturePickerDialog();
    void DrawScriptPickerDialog();
    void DrawAddClipDialog();
    void DrawAudioPickerDialog();
    void DrawGizmo(Camera& camera);

    std::shared_ptr<GameObject>   m_Selected;
    std::shared_ptr<UIElement>    m_SelectedUI;
    GLFWwindow*                   m_Window = nullptr;
    int                           m_WinX = 100, m_WinY = 100, m_WinW = 800, m_WinH = 600;

    // refs set each frame in Render()
    Scene*  m_Scene  = nullptr;
    Camera* m_Camera = nullptr;

    // gizmo state
    GizmoOp m_GizmoOp    = GizmoOp::Translate;
    bool    m_GizmoLocal = false;

    // OBJ import dialog
    bool        m_ShowImportDlg = false;
    std::string m_BrowsePath;
    std::string m_SelectedOBJ;

    // panel visibility
    bool        m_ShowHierarchy     = true;
    bool        m_ShowInspector     = true;
    bool        m_ShowAssetBrowser  = true;
    bool        m_ShowSceneViewport = false;

    // scene viewport state (only meaningful when m_ShowSceneViewport)
    int         m_ViewportW       = 0;
    int         m_ViewportH       = 0;
    float       m_ViewportPosX    = 0.0f;
    float       m_ViewportPosY    = 0.0f;
    bool        m_ViewportHovered = false;

    // asset browser
    std::string m_AssetBrowsePath;
    int         m_AssetFilter      = 0;       // 0=All, 1=Models, 2=Textures, 3=Scripts, 4=Audio, 5=Scenes
    std::string m_AssetSelected;

    // play mode signals
    bool        m_PlayRequested  = false;
    bool        m_StopRequested  = false;
    bool        m_IsPlaying      = false;

    bool        m_UIDragging     = false;

    // texture picker dialog
    enum class TexSlot { Diffuse, NormalMap, UIImageTex };
    bool                          m_ShowTexDlg      = false;
    TexSlot                       m_TexSlot         = TexSlot::Diffuse;
    std::string                   m_TexBrowsePath;
    std::string                   m_SelectedTex;
    std::shared_ptr<MeshRenderer> m_TexTarget;
    std::shared_ptr<UIImage>      m_UIImageTexTarget;

    // script picker dialog
    bool                       m_ShowScriptDlg    = false;
    std::string                m_ScriptBrowsePath;
    std::string                m_SelectedScript;
    std::shared_ptr<LuaScript> m_ScriptTarget;

    // add animation clip dialog
    bool                              m_ShowAddClipDlg   = false;
    std::string                       m_AddClipBrowsePath;
    std::string                       m_SelectedClipFile;
    std::shared_ptr<SkinnedMeshRenderer> m_ClipTarget;

    // audio clip picker dialog
    bool                           m_ShowAudioDlg    = false;
    std::string                    m_AudioBrowsePath;
    std::string                    m_SelectedAudio;
    std::shared_ptr<AudioSource>   m_AudioTarget;
};
