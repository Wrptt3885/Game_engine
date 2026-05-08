#include "rhi/dx11/DX11PointShadow.h"
#ifdef USE_DX11_BACKEND

#include "rhi/dx11/DX11Context.h"
#include "rhi/dx11/DX11Debug.h"
#include <cstring>

DX11PointShadow::DX11PointShadow(int size) : m_Size(size) {
    memset(m_RTVs, 0, sizeof(m_RTVs));
    auto* dev = DX11Context::GetDevice();

    // R16_FLOAT color cubemap — we write linear distance/farPlane as color
    D3D11_TEXTURE2D_DESC td = {};
    td.Width            = (UINT)size;
    td.Height           = (UINT)size;
    td.MipLevels        = 1;
    td.ArraySize        = 6;
    td.Format           = DXGI_FORMAT_R16_FLOAT;
    td.SampleDesc.Count = 1;
    td.Usage            = D3D11_USAGE_DEFAULT;
    td.BindFlags        = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    td.MiscFlags        = D3D11_RESOURCE_MISC_TEXTURECUBE;
    DX_CHECK(dev->CreateTexture2D(&td, nullptr, &m_ColorTex), "PointShadow color tex");

    if (m_ColorTex) {
        // 6 RTVs — one per face
        D3D11_RENDER_TARGET_VIEW_DESC rvd = {};
        rvd.Format                        = DXGI_FORMAT_R16_FLOAT;
        rvd.ViewDimension                 = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        rvd.Texture2DArray.MipSlice       = 0;
        rvd.Texture2DArray.ArraySize      = 1;
        for (int i = 0; i < 6; i++) {
            rvd.Texture2DArray.FirstArraySlice = (UINT)i;
            DX_CHECK(dev->CreateRenderTargetView(m_ColorTex, &rvd, &m_RTVs[i]),
                     "PointShadow RTV");
        }

        // TextureCube SRV for sampling in the main pass
        D3D11_SHADER_RESOURCE_VIEW_DESC svd = {};
        svd.Format                          = DXGI_FORMAT_R16_FLOAT;
        svd.ViewDimension                   = D3D11_SRV_DIMENSION_TEXTURECUBE;
        svd.TextureCube.MipLevels           = 1;
        DX_CHECK(dev->CreateShaderResourceView(m_ColorTex, &svd, &m_SRV),
                 "PointShadow SRV");
    }

    // Shared D32_FLOAT depth buffer — reused and cleared per face
    D3D11_TEXTURE2D_DESC dd = {};
    dd.Width            = (UINT)size;
    dd.Height           = (UINT)size;
    dd.MipLevels        = 1;
    dd.ArraySize        = 1;
    dd.Format           = DXGI_FORMAT_D32_FLOAT;
    dd.SampleDesc.Count = 1;
    dd.Usage            = D3D11_USAGE_DEFAULT;
    dd.BindFlags        = D3D11_BIND_DEPTH_STENCIL;
    DX_CHECK(dev->CreateTexture2D(&dd, nullptr, &m_DepthTex), "PointShadow depth tex");

    if (m_DepthTex) {
        D3D11_DEPTH_STENCIL_VIEW_DESC dvd = {};
        dvd.Format        = DXGI_FORMAT_D32_FLOAT;
        dvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        DX_CHECK(dev->CreateDepthStencilView(m_DepthTex, &dvd, &m_DSV),
                 "PointShadow DSV");
    }
}

DX11PointShadow::~DX11PointShadow() {
    if (m_SRV)     { m_SRV->Release();     m_SRV     = nullptr; }
    if (m_DSV)     { m_DSV->Release();     m_DSV     = nullptr; }
    if (m_DepthTex){ m_DepthTex->Release();m_DepthTex= nullptr; }
    for (auto*& rtv : m_RTVs) { if (rtv) { rtv->Release(); rtv = nullptr; } }
    if (m_ColorTex){ m_ColorTex->Release();m_ColorTex= nullptr; }
}

void DX11PointShadow::BindFace(int face) const {
    auto* ctx = DX11Context::GetContext();
    D3D11_VIEWPORT vp = { 0.f, 0.f, (float)m_Size, (float)m_Size, 0.f, 1.f };
    ctx->RSSetViewports(1, &vp);

    // Clear depth (reused) and color face
    float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    ctx->ClearRenderTargetView(m_RTVs[face], clearColor);
    ctx->ClearDepthStencilView(m_DSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
    ctx->OMSetRenderTargets(1, &m_RTVs[face], m_DSV);
}

void DX11PointShadow::Unbind() const {
    auto* ctx = DX11Context::GetContext();
    ID3D11RenderTargetView* nullRTV = nullptr;
    ctx->OMSetRenderTargets(1, &nullRTV, nullptr);
}

#endif // USE_DX11_BACKEND
