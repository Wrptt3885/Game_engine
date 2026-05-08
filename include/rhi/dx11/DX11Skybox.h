#pragma once
#ifdef USE_DX11_BACKEND

#include <glm/glm.hpp>
#include <memory>
#include <d3d11.h>

class Shader;

class DX11Skybox {
public:
    DX11Skybox();
    ~DX11Skybox();

    void Draw(const glm::mat4& view, const glm::mat4& projection,
              const glm::vec3& sunDirection);

private:
    ID3D11Buffer*           m_VB      = nullptr;
    ID3D11InputLayout*      m_IL      = nullptr;
    ID3D11DepthStencilState* m_SkyDSS = nullptr;
    std::shared_ptr<Shader>  m_Shader;
};

#endif // USE_DX11_BACKEND
