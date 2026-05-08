#pragma once
#ifdef USE_DX11_BACKEND

#include "renderer/Shader.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <string>
#include <unordered_map>
#include <vector>

class DX11Shader : public Shader {
public:
    DX11Shader(const char* vsPath, const char* psPath);
    ~DX11Shader() override;

    void use()  const override;
    void setBool (const std::string& name, bool  value) const override;
    void setInt  (const std::string& name, int   value) const override;
    void setFloat(const std::string& name, float value) const override;
    void setVec3 (const std::string& name, const glm::vec3& v) const override;
    void setMat4 (const std::string& name, const glm::mat4& m) const override;

public:
    struct VarInfo { bool isVS; UINT offset; UINT size; };
private:

    void CompileShader(const char* path, const char* entry, const char* target,
                       ID3DBlob** blob);
    void ReflectCBuffer(ID3DBlob* blob, bool isVS);
    void CreateCBuffer(UINT size, ID3D11Buffer** out);
    void WriteVar(const std::string& name, const void* data, UINT bytes) const;
    void FlushCBuffers() const;

    ID3D11VertexShader* m_VS = nullptr;
    ID3D11PixelShader*  m_PS = nullptr;
    ID3D11InputLayout*  m_IL = nullptr;
    ID3D11Buffer*       m_CB_VS = nullptr;
    ID3D11Buffer*       m_CB_PS = nullptr;

    mutable std::vector<uint8_t> m_VSData;
    mutable std::vector<uint8_t> m_PSData;
    mutable bool                 m_VSDirty = true;
    mutable bool                 m_PSDirty = true;

    std::unordered_map<std::string, VarInfo> m_Vars;
};

#endif // USE_DX11_BACKEND
