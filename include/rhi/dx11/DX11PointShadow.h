#pragma once
#ifdef USE_DX11_BACKEND

#include <d3d11.h>

class DX11PointShadow {
public:
    explicit DX11PointShadow(int size = 512);
    ~DX11PointShadow();

    void BindFace(int face) const;
    void Unbind() const;

    ID3D11ShaderResourceView* GetSRV()  const { return m_SRV; }
    int                       GetSize() const { return m_Size; }

private:
    ID3D11Texture2D*          m_ColorTex   = nullptr;  // R16_FLOAT, 6-slice array
    ID3D11RenderTargetView*   m_RTVs[6]    = {};        // one per face
    ID3D11Texture2D*          m_DepthTex   = nullptr;  // shared D32_FLOAT depth
    ID3D11DepthStencilView*   m_DSV        = nullptr;
    ID3D11ShaderResourceView* m_SRV        = nullptr;  // TextureCube SRV
    int                       m_Size;
};

#endif // USE_DX11_BACKEND
