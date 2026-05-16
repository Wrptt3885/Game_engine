#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>

struct Vertex {
    glm::vec3  position;
    glm::vec3  normal;
    glm::vec2  texCoord;
    glm::vec3  tangent;
    glm::vec3  bitangent;
    glm::uvec4 boneIds     = {0, 0, 0, 0};
    glm::vec4  boneWeights = {1.0f, 0.0f, 0.0f, 0.0f};
};

class Mesh {
public:
    virtual ~Mesh() = default;

    virtual void Draw() const = 0;

    size_t GetVertexCount() const { return m_Vertices.size(); }
    size_t GetIndexCount()  const { return m_Indices.size(); }
    const std::string& GetPath() const { return m_Path; }
    void SetPath(const std::string& p)  { m_Path = p; }

    static std::shared_ptr<Mesh> Create(const std::vector<Vertex>& vertices,
                                        const std::vector<unsigned int>& indices);

protected:
    Mesh(const std::vector<Vertex>& v, const std::vector<unsigned int>& i)
        : m_Vertices(v), m_Indices(i) {}

    std::string                  m_Path;
    std::vector<Vertex>          m_Vertices;
    std::vector<unsigned int>    m_Indices;
};
