#include "rhi/opengl/GLShader.h"
#include "rhi/RHI.h"
#ifdef USE_DX11_BACKEND
#include "rhi/dx11/DX11Shader.h"
#endif
#include <glad/gl.h>
#include <fstream>
#include <iostream>
#include <sstream>

// --- factory -----------------------------------------------------------------

std::shared_ptr<Shader> Shader::Create(const char* vertPath, const char* fragPath) {
    switch (RHI::GetAPI()) {
        case GraphicsAPI::OpenGL:    return std::make_shared<GLShader>(vertPath, fragPath);
        case GraphicsAPI::DirectX11:
#ifdef USE_DX11_BACKEND
            { return std::make_shared<DX11Shader>(vertPath, fragPath); }
#else
            break;
#endif
        default: return nullptr;
    }
}

// --- GLShader ----------------------------------------------------------------

GLShader::GLShader(const char* vertexPath, const char* fragmentPath) {
    std::string vertCode = readFile(vertexPath);
    std::string fragCode = readFile(fragmentPath);
    const char* vSrc = vertCode.c_str();
    const char* fSrc = fragCode.c_str();

    unsigned int vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vSrc, nullptr);
    glCompileShader(vert);
    checkCompileErrors(vert, "VERTEX");

    unsigned int frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fSrc, nullptr);
    glCompileShader(frag);
    checkCompileErrors(frag, "FRAGMENT");

    m_ID = glCreateProgram();
    glAttachShader(m_ID, vert);
    glAttachShader(m_ID, frag);
    glLinkProgram(m_ID);
    checkCompileErrors(m_ID, "PROGRAM");

    glDeleteShader(vert);
    glDeleteShader(frag);
}

GLShader::~GLShader() { glDeleteProgram(m_ID); }

void GLShader::use() const { glUseProgram(m_ID); }

void GLShader::setBool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(m_ID, name.c_str()), (int)value);
}
void GLShader::setInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(m_ID, name.c_str()), value);
}
void GLShader::setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(m_ID, name.c_str()), value);
}
void GLShader::setVec3(const std::string& name, const glm::vec3& vec) const {
    glUniform3fv(glGetUniformLocation(m_ID, name.c_str()), 1, &vec[0]);
}
void GLShader::setMat4(const std::string& name, const glm::mat4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(m_ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

std::string GLShader::readFile(const char* filePath) const {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[Shader] Cannot open: " << filePath << "\n";
        return {};
    }
    std::stringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

void GLShader::checkCompileErrors(unsigned int shader, const std::string& type) const {
    int success;
    char log[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, log);
            std::cerr << "[Shader] " << type << " compile error:\n" << log << "\n";
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, nullptr, log);
            std::cerr << "[Shader] Link error:\n" << log << "\n";
        }
    }
}
