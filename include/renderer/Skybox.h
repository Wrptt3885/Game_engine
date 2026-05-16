#pragma once
#include <glm/glm.hpp>
#include <memory>

class Shader;

class Skybox {
public:
    Skybox();
    ~Skybox();

    void Draw(const glm::mat4& view, const glm::mat4& projection,
              const glm::vec3& sunDirection);

private:
    unsigned int           m_VAO, m_VBO;
    std::shared_ptr<Shader> m_Shader;
};
