#include "rhi/dx11/DX11Texture.h"
#ifdef USE_DX11_BACKEND

#include "rhi/dx11/DX11Context.h"
#include <stb_image.h>
#include <iostream>

DX11Texture::DX11Texture(const std::string& filepath) : m_Path(filepath) {
    stbi_set_flip_vertically_on_load(false); // DX11: V=0 is top, matches stb_image default
    int channels;
    unsigned char* data = stbi_load(filepath.c_str(), &m_Width, &m_Height, &channels, 4);
    if (!data) {
        std::cerr << "[DX11Texture] Failed to load: " << filepath << "\n"
                  << "  " << stbi_failure_reason() << "\n";
        return;
    }

    auto* dev = DX11Context::GetDevice();

    D3D11_TEXTURE2D_DESC td   = {};
    td.Width                  = (UINT)m_Width;
    td.Height                 = (UINT)m_Height;
    td.MipLevels              = 0; // auto mips
    td.ArraySize              = 1;
    td.Format                 = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count       = 1;
    td.Usage                  = D3D11_USAGE_DEFAULT;
    td.BindFlags              = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    td.MiscFlags              = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    HRESULT hr = dev->CreateTexture2D(&td, nullptr, &m_Texture);
    if (FAILED(hr)) {
        std::cerr << "[DX11Texture] CreateTexture2D failed: 0x" << std::hex << hr << "\n";
        stbi_image_free(data);
        return;
    }

    // Upload mip 0
    DX11Context::GetContext()->UpdateSubresource(
        m_Texture, 0, nullptr, data, (UINT)(m_Width * 4), 0);
    stbi_image_free(data);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                          = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension                   = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels             = (UINT)-1;
    hr = dev->CreateShaderResourceView(m_Texture, &srvDesc, &m_SRV);
    if (FAILED(hr)) {
        std::cerr << "[DX11Texture] CreateSRV failed: 0x" << std::hex << hr << "\n";
        return;
    }

    DX11Context::GetContext()->GenerateMips(m_SRV);

    std::cout << "[DX11Texture] Loaded: " << filepath
              << " (" << m_Width << "x" << m_Height << ")\n";
}

DX11Texture::DX11Texture(int width, int height, const unsigned char* rgba, const std::string& name)
    : m_Path(name), m_Width(width), m_Height(height)
{
    auto* dev = DX11Context::GetDevice();
    D3D11_TEXTURE2D_DESC td   = {};
    td.Width                  = (UINT)width;
    td.Height                 = (UINT)height;
    td.MipLevels              = 0;
    td.ArraySize              = 1;
    td.Format                 = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count       = 1;
    td.Usage                  = D3D11_USAGE_DEFAULT;
    td.BindFlags              = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    td.MiscFlags              = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    HRESULT hr = dev->CreateTexture2D(&td, nullptr, &m_Texture);
    if (FAILED(hr)) return;

    DX11Context::GetContext()->UpdateSubresource(m_Texture, 0, nullptr, rgba, (UINT)(width * 4), 0);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                          = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension                   = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels             = (UINT)-1;
    hr = dev->CreateShaderResourceView(m_Texture, &srvDesc, &m_SRV);
    if (FAILED(hr)) return;
    DX11Context::GetContext()->GenerateMips(m_SRV);
}

DX11Texture::~DX11Texture() {
    if (m_SRV)     { m_SRV->Release();     m_SRV     = nullptr; }
    if (m_Texture) { m_Texture->Release(); m_Texture = nullptr; }
}

void DX11Texture::Bind(unsigned int slot) const {
    DX11Context::GetContext()->PSSetShaderResources(slot, 1, &m_SRV);
}

#endif // USE_DX11_BACKEND
