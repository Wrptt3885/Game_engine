#pragma once

#include "core/Component.h"
#include "renderer/Mesh.h"
#include "graphics/Material.h"
#include <memory>

class MeshRenderer : public Component {
public:
    MeshRenderer() = default;
    MeshRenderer(std::shared_ptr<Mesh> mesh) : m_Mesh(mesh) {}

    void SetMesh(std::shared_ptr<Mesh> mesh)          { m_Mesh = mesh; }
    void SetMaterial(const Material& material)        { m_Material = material; }
    void SetTexture(std::shared_ptr<Texture> tex)     { m_Material.texture   = tex; }
    void SetNormalMap(std::shared_ptr<Texture> tex)   { m_Material.normalMap = tex; }

    std::shared_ptr<Mesh> GetMesh()     const { return m_Mesh; }
    Material&       GetMaterial()             { return m_Material; }
    const Material& GetMaterial()       const { return m_Material; }

    void Render(const Camera& camera) override;

private:
    std::shared_ptr<Mesh> m_Mesh;
    Material              m_Material;
};
