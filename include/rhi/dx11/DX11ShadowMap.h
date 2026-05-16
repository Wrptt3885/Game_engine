#pragma once
#ifdef USE_DX11_BACKEND

#include <d3d11.h>

class DX11ShadowMap {
public:
    DX11ShadowMap(int width = 4096, int height = 4096);
    ~DX11ShadowMap();

    void Bind()   const;
    void Unbind() const;

    ID3D11ShaderResourceView* GetSRV()    const { return m_SRV;    }
    ID3D11SamplerState*       GetSampler()const { return m_Sampler;}
    int GetWidth()  const { return m_Width;  }
    int GetHeight() const { return m_Height; }

private:
    ID3D11Texture2D*          m_Texture = nullptr;
    ID3D11DepthStencilView*   m_DSV     = nullptr;
    ID3D11ShaderResourceView* m_SRV     = nullptr;
    ID3D11SamplerState*       m_Sampler = nullptr;
    int m_Width, m_Height;
};

#endif // USE_DX11_BACKEND
