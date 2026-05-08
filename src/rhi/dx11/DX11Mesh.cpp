#include "rhi/dx11/DX11Mesh.h"
#ifdef USE_DX11_BACKEND

#include "rhi/dx11/DX11Context.h"

DX11Mesh::DX11Mesh(const std::vector<Vertex>& v, const std::vector<unsigned int>& i)
    : Mesh(v, i) {
    Setup();
}

DX11Mesh::~DX11Mesh() {
    if (m_VB) { m_VB->Release(); m_VB = nullptr; }
    if (m_IB) { m_IB->Release(); m_IB = nullptr; }
}

void DX11Mesh::Setup() {
    auto* dev = DX11Context::GetDevice();

    D3D11_BUFFER_DESC bd  = {};
    bd.Usage              = D3D11_USAGE_IMMUTABLE;
    bd.BindFlags          = D3D11_BIND_VERTEX_BUFFER;
    bd.ByteWidth          = (UINT)(m_Vertices.size() * sizeof(Vertex));

    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem = m_Vertices.data();
    dev->CreateBuffer(&bd, &sd, &m_VB);

    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.ByteWidth = (UINT)(m_Indices.size() * sizeof(unsigned int));
    sd.pSysMem   = m_Indices.data();
    dev->CreateBuffer(&bd, &sd, &m_IB);
}

void DX11Mesh::Draw() const {
    auto*  ctx    = DX11Context::GetContext();
    UINT   stride = sizeof(Vertex);
    UINT   offset = 0;
    ctx->IASetVertexBuffers(0, 1, &m_VB, &stride, &offset);
    ctx->IASetIndexBuffer(m_IB, DXGI_FORMAT_R32_UINT, 0);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->DrawIndexed((UINT)m_Indices.size(), 0, 0);
}

#endif // USE_DX11_BACKEND
