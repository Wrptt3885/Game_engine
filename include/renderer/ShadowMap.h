#pragma once

class ShadowMap {
public:
    ShadowMap(int width = 2048, int height = 2048);
    ~ShadowMap();

    void Bind() const;
    void Unbind() const;

    unsigned int GetDepthTexture() const { return m_DepthTexture; }
    int GetWidth()  const { return m_Width;  }
    int GetHeight() const { return m_Height; }

private:
    unsigned int m_FBO;
    unsigned int m_DepthTexture;
    int m_Width, m_Height;
};
