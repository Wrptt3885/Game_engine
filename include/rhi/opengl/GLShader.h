#pragma once

#include "renderer/Shader.h"

class GLShader : public Shader {
public:
    GLShader(const char* vertexPath, const char* fragmentPath);
    ~GLShader() override;

    void use() const override;
    void setBool (const std::string& name, bool            value) const override;
    void setInt  (const std::string& name, int             value) const override;
    void setFloat(const std::string& name, float           value) const override;
    void setVec3 (const std::string& name, const glm::vec3& vec) const override;
    void setMat4     (const std::string& name, const glm::mat4& mat) const override;
    void setMat4Array(const std::string& name, const glm::mat4* mats, int count) const override;

private:
    unsigned int m_ID = 0;

    std::string readFile(const char* filePath) const;
    void checkCompileErrors(unsigned int shader, const std::string& type) const;
};
