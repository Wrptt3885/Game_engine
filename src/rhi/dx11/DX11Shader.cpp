#include "rhi/dx11/DX11Shader.h"
#ifdef USE_DX11_BACKEND

#include "rhi/dx11/DX11Context.h"
#include "renderer/Mesh.h"
#include <d3dcompiler.h>
#include <glm/glm.hpp>
#include <iostream>
#include <cstring>
#include <algorithm>

// ---- Recursive type reflector -----------------------------------------------

static void ReflectType(ID3D11ShaderReflectionType* type,
                        const std::string& name,
                        UINT baseOffset, UINT totalSize,
                        bool isVS,
                        std::unordered_map<std::string, DX11Shader::VarInfo>& vars)
{
    D3D11_SHADER_TYPE_DESC td;
    type->GetDesc(&td);

    bool isArray  = (td.Elements > 0);
    bool isStruct = (td.Class == D3D_SVC_STRUCT);

    if (isArray && isStruct) {
        // Array of structs: e.g. DirLight u_DirLights[4]
        UINT elemSize = totalSize / td.Elements;
        for (UINT e = 0; e < td.Elements; e++) {
            std::string eName = name + "[" + std::to_string(e) + "]";
            UINT eBase = baseOffset + e * elemSize;
            for (UINT m = 0; m < td.Members; m++) {
                auto* mt = type->GetMemberTypeByIndex(m);
                D3D11_SHADER_TYPE_DESC md;
                mt->GetDesc(&md);
                UINT mSize = (md.Rows * md.Columns * 4 > 0) ? md.Rows * md.Columns * 4 : 4;
                ReflectType(mt, eName + "." + type->GetMemberTypeName(m),
                            eBase + md.Offset, mSize, isVS, vars);
            }
        }
    } else if (isArray) {
        // Array of scalars/vectors: e.g. float u_Arr[4]
        UINT elemSize = (totalSize / td.Elements > 0) ? totalSize / td.Elements : 16;
        for (UINT e = 0; e < td.Elements; e++) {
            vars[name + "[" + std::to_string(e) + "]"] =
                { isVS, baseOffset + e * elemSize, elemSize };
        }
    } else if (isStruct) {
        // Plain struct
        for (UINT m = 0; m < td.Members; m++) {
            auto* mt = type->GetMemberTypeByIndex(m);
            D3D11_SHADER_TYPE_DESC md;
            mt->GetDesc(&md);
            UINT mSize = (md.Rows * md.Columns * 4 > 0) ? md.Rows * md.Columns * 4 : 4;
            ReflectType(mt, name + "." + type->GetMemberTypeName(m),
                        baseOffset + md.Offset, mSize, isVS, vars);
        }
    } else {
        // Leaf: scalar, vector, or matrix
        UINT size = (td.Rows * td.Columns * 4 > 0) ? td.Rows * td.Columns * 4 : 4;
        vars[name] = { isVS, baseOffset, size };
    }
}

// ---- DX11Shader -------------------------------------------------------------

DX11Shader::DX11Shader(const char* vsPath, const char* psPath) {
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;

    CompileShader(vsPath, "VS", "vs_5_0", &vsBlob);
    CompileShader(psPath, "PS", "ps_5_0", &psBlob);

    auto* dev = DX11Context::GetDevice();

    if (vsBlob) {
        HRESULT hvs = dev->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_VS);
        if (FAILED(hvs)) {
            std::cerr << "[DX11Shader] CreateVertexShader failed 0x" << std::hex << hvs << " path=" << vsPath << "\n";
            if (FILE* f = fopen("shader_compile_error.txt", "a")) {
                fprintf(f, "CreateVertexShader FAILED 0x%x path=%s\n", (unsigned)hvs, vsPath);
                fclose(f);
            }
        }
        ReflectCBuffer(vsBlob, true);

        D3D11_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, position),  D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, normal),    D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(Vertex, texCoord),  D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, tangent),   D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, bitangent), D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        dev->CreateInputLayout(layout, 5, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_IL);
        vsBlob->Release();
    }

    if (psBlob) {
        HRESULT hps = dev->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_PS);
        if (FAILED(hps)) {
            std::cerr << "[DX11Shader] CreatePixelShader failed 0x" << std::hex << hps << " path=" << psPath << "\n";
            if (FILE* f = fopen("shader_compile_error.txt", "a")) {
                fprintf(f, "CreatePixelShader FAILED 0x%x path=%s\n", (unsigned)hps, psPath);
                fclose(f);
            }
        }
        ReflectCBuffer(psBlob, false);
        psBlob->Release();
    }
}

DX11Shader::~DX11Shader() {
    if (m_VS)    { m_VS->Release();    m_VS    = nullptr; }
    if (m_PS)    { m_PS->Release();    m_PS    = nullptr; }
    if (m_IL)    { m_IL->Release();    m_IL    = nullptr; }
    if (m_CB_VS) { m_CB_VS->Release(); m_CB_VS = nullptr; }
    if (m_CB_PS) { m_CB_PS->Release(); m_CB_PS = nullptr; }
}

void DX11Shader::CompileShader(const char* path, const char* entry,
                                const char* target, ID3DBlob** blob) {
    ID3DBlob* errBlob = nullptr;
    int len = (int)strlen(path);
    std::wstring wpath(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, path, len, &wpath[0], len);

    HRESULT hr = D3DCompileFromFile(
        wpath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entry, target, D3DCOMPILE_ENABLE_STRICTNESS, 0, blob, &errBlob);

    if (FAILED(hr)) {
        std::cerr << "[DX11Shader] Compile failed (" << entry << "): " << path << "\n";
        if (errBlob) {
            std::cerr << (char*)errBlob->GetBufferPointer() << "\n";
            // Write to file so it's visible even without stderr capture
            if (FILE* f = fopen("shader_compile_error.txt", "a")) {
                fprintf(f, "FAIL (%s): %s\n%s\n", entry, path,
                        (char*)errBlob->GetBufferPointer());
                fclose(f);
            }
            errBlob->Release();
        }
        return;
    }
    if (errBlob) errBlob->Release();
}

void DX11Shader::ReflectCBuffer(ID3DBlob* blob, bool isVS) {
    ID3D11ShaderReflection* refl = nullptr;
    if (FAILED(D3DReflect(blob->GetBufferPointer(), blob->GetBufferSize(),
                          IID_ID3D11ShaderReflection, (void**)&refl)))
        return;

    D3D11_SHADER_DESC sdesc;
    refl->GetDesc(&sdesc);

    for (UINT i = 0; i < sdesc.ConstantBuffers; i++) {
        auto* cb = refl->GetConstantBufferByIndex(i);
        D3D11_SHADER_BUFFER_DESC cbDesc;
        cb->GetDesc(&cbDesc);

        UINT cbSize = (cbDesc.Size + 15) & ~15u;

        auto*& cbBuf  = isVS ? m_CB_VS : m_CB_PS;
        auto&  cbData = isVS ? m_VSData : m_PSData;

        if (!cbBuf) {
            cbData.assign(cbSize, 0);
            CreateCBuffer(cbSize, &cbBuf);
        }

        // Recursively build name → offset map
        for (UINT j = 0; j < cbDesc.Variables; j++) {
            auto* var = cb->GetVariableByIndex(j);
            D3D11_SHADER_VARIABLE_DESC vdesc;
            var->GetDesc(&vdesc);
            ReflectType(var->GetType(), vdesc.Name,
                        vdesc.StartOffset, vdesc.Size, isVS, m_Vars);
        }
    }
    refl->Release();
}

void DX11Shader::CreateCBuffer(UINT size, ID3D11Buffer** out) {
    D3D11_BUFFER_DESC bd = {};
    bd.Usage             = D3D11_USAGE_DYNAMIC;
    bd.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;
    bd.ByteWidth         = size;
    DX11Context::GetDevice()->CreateBuffer(&bd, nullptr, out);
}

void DX11Shader::WriteVar(const std::string& name, const void* data, UINT bytes) const {
    auto it = m_Vars.find(name);
    if (it == m_Vars.end()) return;

    const auto& info = it->second;
    auto&  buf   = info.isVS ? m_VSData : m_PSData;
    bool&  dirty = info.isVS ? m_VSDirty : m_PSDirty;

    UINT end = info.offset + bytes;
    if (end <= (UINT)buf.size()) {
        memcpy(buf.data() + info.offset, data, bytes);
        dirty = true;
    }
}

void DX11Shader::FlushCBuffers() const {
    auto* ctx = DX11Context::GetContext();

    auto upload = [&](ID3D11Buffer* cb, const std::vector<uint8_t>& data, bool& dirty) {
        if (!cb || !dirty) return;
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(ctx->Map(cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            memcpy(mapped.pData, data.data(), data.size());
            ctx->Unmap(cb, 0);
        }
        dirty = false;
    };

    upload(m_CB_VS, m_VSData, m_VSDirty);
    upload(m_CB_PS, m_PSData, m_PSDirty);

    if (m_CB_VS) ctx->VSSetConstantBuffers(0, 1, &m_CB_VS);
    if (m_CB_PS) ctx->PSSetConstantBuffers(0, 1, &m_CB_PS);
}

void DX11Shader::use() const {
    auto* ctx = DX11Context::GetContext();
    FlushCBuffers();
    ctx->VSSetShader(m_VS, nullptr, 0);
    ctx->PSSetShader(m_PS, nullptr, 0);
    ctx->IASetInputLayout(m_IL);
}

void DX11Shader::setBool(const std::string& name, bool value) const {
    int v = value ? 1 : 0;
    WriteVar(name, &v, sizeof(int));
}
void DX11Shader::setInt(const std::string& name, int value) const {
    WriteVar(name, &value, sizeof(int));
}
void DX11Shader::setFloat(const std::string& name, float value) const {
    WriteVar(name, &value, sizeof(float));
}
void DX11Shader::setVec3(const std::string& name, const glm::vec3& v) const {
    WriteVar(name, &v[0], sizeof(glm::vec3));
}
void DX11Shader::setMat4(const std::string& name, const glm::mat4& m) const {
    // GLM column-major matches HLSL column-major cbuffer packing
    WriteVar(name, &m[0][0], sizeof(glm::mat4));
}

#endif // USE_DX11_BACKEND
