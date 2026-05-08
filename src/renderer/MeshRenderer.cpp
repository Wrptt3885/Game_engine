#include "renderer/MeshRenderer.h"
#include "renderer/Renderer.h"
#include "core/Camera.h"
#include "core/GameObject.h"

void MeshRenderer::Render(const Camera& camera) {
    if (!m_Mesh || !GetGameObject()) return;
    Renderer::DrawMesh(*m_Mesh, GetGameObject()->GetTransform().GetMatrix(), m_Material);
}
