#include "rhi/dx11/DX11Skybox.h"
#ifdef USE_DX11_BACKEND

#include "rhi/dx11/DX11Context.h"
#include "renderer/Shader.h"
#include <d3dcompiler.h>

static float s_Verts[] = {
    -1,  1, -1,  -1, -1, -1,   1, -1, -1,   1, -1, -1,   1,  1, -1,  -1,  1, -1,
    -1, -1,  1,  -1, -1, -1,  -1,  1, -1,  -1,  1, -1,  -1,  1,  1,  -1, -1,  1,
     1, -1, -1,   1, -1,  1,   1,  1,  1,   1,  1,  1,   1,  1, -1,   1, -1, -1,
    -1, -1,  1,  -1,  1,  1,   1,  1,  1,   1,  1,  1,   1, -1,  1,  -1, -1,  1,
    -1,  1, -1,   1,  1, -1,   1,  1,  1,   1,  1,  1,  -1,  1,  1,  -1,  1, -1,
    -1, -1, -1,  -1, -1,  1,   1, -1, -1,   1, -1, -1,  -1, -1,  1,   1, -1,  1
};

DX11Skybox::DX11Skybox() {
    m_Shader = Shader::Create(SHADER_DIR "/dx11/skybox.hlsl", SHADER_DIR "/dx11/skybox.hlsl");

    auto* dev = DX11Context::GetDevice();

    D3D11_BUFFER_DESC bd = {};
    bd.Usage             = D3D11_USAGE_IMMUTABLE;
    bd.BindFlags         = D3D11_BIND_VERTEX_BUFFER;
    bd.ByteWidth         = sizeof(s_Verts);
    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem = s_Verts;
    dev->CreateBuffer(&bd, &sd, &m_VB);

    // Input layout: POSITION only
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    // Need VS blob for layout creation — compile separately
    ID3DBlob* vsBlob  = nullptr;
    ID3DBlob* errBlob = nullptr;
    std::string path  = std::string(SHADER_DIR) + "/dx11/skybox.hlsl";
    std::wstring wpath(path.begin(), path.end());
    D3DCompileFromFile(wpath.c_str(), nullptr, nullptr, "VS", "vs_5_0",
                       D3DCOMPILE_ENABLE_STRICTNESS, 0, &vsBlob, &errBlob);
    if (vsBlob) {
        dev->CreateInputLayout(layout, 1, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_IL);
        vsBlob->Release();
    }
    if (errBlob) errBlob->Release();

    // Cache skybox DSS (depth read, no write, LESS_EQUAL so skybox at far plane passes)
    D3D11_DEPTH_STENCIL_DESC dsd = {};
    dsd.DepthEnable              = TRUE;
    dsd.DepthWriteMask           = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsd.DepthFunc                = D3D11_COMPARISON_LESS_EQUAL;
    dev->CreateDepthStencilState(&dsd, &m_SkyDSS);
}

DX11Skybox::~DX11Skybox() {
    if (m_SkyDSS) { m_SkyDSS->Release(); m_SkyDSS = nullptr; }
    if (m_VB)     { m_VB->Release();     m_VB     = nullptr; }
    if (m_IL)     { m_IL->Release();     m_IL     = nullptr; }
}

void DX11Skybox::Draw(const glm::mat4& view, const glm::mat4& projection,
                      const glm::vec3& sunDirection) {
    auto* ctx = DX11Context::GetContext();

    m_Shader->setMat4("u_View",       view);
    m_Shader->setMat4("u_Projection", projection);
    m_Shader->setVec3("u_SunDirection", sunDirection);
    m_Shader->use();

    // Override input layout for skybox (position only)
    ctx->IASetInputLayout(m_IL);

    UINT stride = sizeof(float) * 3;
    UINT offset = 0;
    ctx->IASetVertexBuffers(0, 1, &m_VB, &stride, &offset);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ctx->OMSetDepthStencilState(m_SkyDSS, 0);
    ctx->Draw(36, 0);
    ctx->OMSetDepthStencilState(DX11Context::GetDepthStencilState(), 0);
}

#endif // USE_DX11_BACKEND
