#pragma once

#include "renderer/Mesh.h"

class GLMesh : public Mesh {
public:
    GLMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    ~GLMesh() override;

    void Draw() const override;

private:
    void Setup();
    unsigned int m_VAO = 0, m_VBO = 0, m_EBO = 0;
};
