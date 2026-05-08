#pragma once
#ifdef USE_DX11_BACKEND

#include "renderer/Mesh.h"
#include <d3d11.h>

class DX11Mesh : public Mesh {
public:
    DX11Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    ~DX11Mesh() override;
    void Draw() const override;

private:
    void Setup();
    ID3D11Buffer* m_VB = nullptr;
    ID3D11Buffer* m_IB = nullptr;
};

#endif // USE_DX11_BACKEND
