#include "rhi/dx11/DX11ShadowMap.h"
#ifdef USE_DX11_BACKEND

#include "rhi/dx11/DX11Context.h"
#include <iostream>

DX11ShadowMap::DX11ShadowMap(int width, int height)
    : m_Width(width), m_Height(height) {

    auto* dev = DX11Context::GetDevice();

    // Depth texture (typeless so we can use as both DSV and SRV)
    D3D11_TEXTURE2D_DESC td   = {};
    td.Width                  = (UINT)width;
    td.Height                 = (UINT)height;
    td.MipLevels              = 1;
    td.ArraySize              = 1;
    td.Format                 = DXGI_FORMAT_R32_TYPELESS;
    td.SampleDesc.Count       = 1;
    td.Usage                  = D3D11_USAGE_DEFAULT;
    td.BindFlags              = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    dev->CreateTexture2D(&td, nullptr, &m_Texture);

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format             = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
    dev->CreateDepthStencilView(m_Texture, &dsvDesc, &m_DSV);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                          = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension                   = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels             = 1;
    dev->CreateShaderResourceView(m_Texture, &srvDesc, &m_SRV);

    // Point sampler for manual PCF — HLSL uses .Sample(), not .SampleCmp()
    D3D11_SAMPLER_DESC smpd   = {};
    smpd.Filter               = D3D11_FILTER_MIN_MAG_MIP_POINT;
    smpd.AddressU             = D3D11_TEXTURE_ADDRESS_BORDER;
    smpd.AddressV             = D3D11_TEXTURE_ADDRESS_BORDER;
    smpd.AddressW             = D3D11_TEXTURE_ADDRESS_BORDER;
    smpd.BorderColor[0]       = 1.0f;
    smpd.BorderColor[1]       = 1.0f;
    smpd.BorderColor[2]       = 1.0f;
    smpd.BorderColor[3]       = 1.0f;
    smpd.ComparisonFunc       = D3D11_COMPARISON_NEVER;
    smpd.MaxLOD               = D3D11_FLOAT32_MAX;
    dev->CreateSamplerState(&smpd, &m_Sampler);
}

DX11ShadowMap::~DX11ShadowMap() {
    if (m_Sampler) { m_Sampler->Release(); m_Sampler = nullptr; }
    if (m_SRV)     { m_SRV->Release();     m_SRV     = nullptr; }
    if (m_DSV)     { m_DSV->Release();     m_DSV     = nullptr; }
    if (m_Texture) { m_Texture->Release(); m_Texture = nullptr; }
}

void DX11ShadowMap::Bind() const {
    auto* ctx = DX11Context::GetContext();
    D3D11_VIEWPORT vp = { 0.f, 0.f, (float)m_Width, (float)m_Height, 0.f, 1.f };
    ctx->RSSetViewports(1, &vp);
    ctx->OMSetRenderTargets(0, nullptr, m_DSV);
    ctx->ClearDepthStencilView(m_DSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void DX11ShadowMap::Unbind() const {
    auto* ctx = DX11Context::GetContext();
    auto* rtv = DX11Context::GetRTV();
    auto* dsv = DX11Context::GetDSV();
    ctx->OMSetRenderTargets(1, &rtv, dsv);
    const auto& vp = DX11Context::GetViewport();
    ctx->RSSetViewports(1, &vp);
}

#endif // USE_DX11_BACKEND
