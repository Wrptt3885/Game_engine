#pragma once
#ifdef USE_DX11_BACKEND

#include "renderer/Texture.h"
#include <d3d11.h>
#include <string>

class DX11Texture : public Texture {
public:
    explicit DX11Texture(const std::string& filepath);
    DX11Texture(int width, int height, const unsigned char* rgba, const std::string& name);
    ~DX11Texture() override;

    void Bind(unsigned int slot) const override;

    int  GetWidth()  const override { return m_Width;  }
    int  GetHeight() const override { return m_Height; }
    bool IsLoaded()  const override { return m_SRV != nullptr; }
    const std::string& GetPath() const override { return m_Path; }

private:
    ID3D11ShaderResourceView* m_SRV     = nullptr;
    ID3D11Texture2D*          m_Texture = nullptr;
    std::string               m_Path;
    int                       m_Width   = 0;
    int                       m_Height  = 0;
};

#endif // USE_DX11_BACKEND
