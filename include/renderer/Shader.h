#pragma once

#include <glm/glm.hpp>
#include <string>
#include <memory>

class Shader {
public:
    virtual ~Shader() = default;

    virtual void use() const = 0;
    virtual void setBool (const std::string& name, bool           value) const = 0;
    virtual void setInt  (const std::string& name, int            value) const = 0;
    virtual void setFloat(const std::string& name, float          value) const = 0;
    virtual void setVec3 (const std::string& name, const glm::vec3& vec) const = 0;
    virtual void setMat4     (const std::string& name, const glm::mat4& mat) const = 0;
    virtual void setMat4Array(const std::string& name, const glm::mat4* mats, int count) const = 0;

    static std::shared_ptr<Shader> Create(const char* vertPath, const char* fragPath);
};
