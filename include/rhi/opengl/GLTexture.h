#pragma once

#include "renderer/Texture.h"

class GLTexture : public Texture {
public:
    explicit GLTexture(const std::string& path);
    GLTexture(int width, int height, const unsigned char* rgba, const std::string& name);
    ~GLTexture() override;

    void Bind(unsigned int slot = 0) const override;
    int  GetWidth()  const override { return m_Width; }
    int  GetHeight() const override { return m_Height; }
    bool IsLoaded()  const override { return m_ID != 0; }
    const std::string& GetPath() const override { return m_Path; }

private:
    unsigned int m_ID       = 0;
    int          m_Width    = 0;
    int          m_Height   = 0;
    int          m_Channels = 0;
    std::string  m_Path;
};
