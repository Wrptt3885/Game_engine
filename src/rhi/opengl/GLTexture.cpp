#include "rhi/opengl/GLTexture.h"
#include "rhi/RHI.h"
#ifdef USE_DX11_BACKEND
#include "rhi/dx11/DX11Texture.h"
#endif
#include <glad/gl.h>
#include <stb_image.h>
#include <iostream>

// --- factories ---------------------------------------------------------------

std::shared_ptr<Texture> Texture::CreateFromMemory(int width, int height,
    const unsigned char* rgba, const std::string& name)
{
    switch (RHI::GetAPI()) {
        case GraphicsAPI::OpenGL:    return std::make_shared<GLTexture>(width, height, rgba, name);
        case GraphicsAPI::DirectX11:
#ifdef USE_DX11_BACKEND
            { return std::make_shared<DX11Texture>(width, height, rgba, name); }
#else
            break;
#endif
        default: return nullptr;
    }
    return nullptr;
}

std::shared_ptr<Texture> Texture::Create(const std::string& path) {
    switch (RHI::GetAPI()) {
        case GraphicsAPI::OpenGL:    return std::make_shared<GLTexture>(path);
        case GraphicsAPI::DirectX11:
#ifdef USE_DX11_BACKEND
            { return std::make_shared<DX11Texture>(path); }
#else
            break;
#endif
        default: return nullptr;
    }
}

// --- GLTexture ---------------------------------------------------------------

GLTexture::GLTexture(const std::string& filepath) : m_Path(filepath) {
    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load(filepath.c_str(), &m_Width, &m_Height, &m_Channels, 0);
    if (!data) {
        std::cerr << "[Texture] Failed to load: " << filepath << "\n"
                  << "  " << stbi_failure_reason() << "\n";
        return;
    }

    GLenum format = GL_RGB;
    if (m_Channels == 1) format = GL_RED;
    if (m_Channels == 4) format = GL_RGBA;

    glGenTextures(1, &m_ID);
    glBindTexture(GL_TEXTURE_2D, m_ID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, format, m_Width, m_Height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    std::cout << "[Texture] Loaded: " << filepath
              << " (" << m_Width << "x" << m_Height << ", " << m_Channels << "ch)\n";
}

GLTexture::GLTexture(int width, int height, const unsigned char* rgba, const std::string& name)
    : m_Width(width), m_Height(height), m_Channels(4), m_Path(name)
{
    glGenTextures(1, &m_ID);
    glBindTexture(GL_TEXTURE_2D, m_ID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

GLTexture::~GLTexture() {
    if (m_ID) glDeleteTextures(1, &m_ID);
}

void GLTexture::Bind(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_ID);
}
