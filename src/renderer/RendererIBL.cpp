#include "RendererState.h"
#include <iostream>
#include <vector>

#ifdef USE_DX11_BACKEND

namespace {

// Cubemap face basis: (forward, right, up) for converting NDC.xy to world dir
// in the fullscreen-triangle face shaders. Standard DX11 cubemap layout.
struct FaceBasis { glm::vec3 fwd, right, up; };
static const FaceBasis kFaces[6] = {
    { { 1,  0,  0}, { 0,  0, -1}, { 0,  1,  0} }, // +X
    { {-1,  0,  0}, { 0,  0,  1}, { 0,  1,  0} }, // -X
    { { 0,  1,  0}, { 1,  0,  0}, { 0,  0,  1} }, // +Y
    { { 0, -1,  0}, { 1,  0,  0}, { 0,  0, -1} }, // -Y
    { { 0,  0,  1}, { 1,  0,  0}, { 0,  1,  0} }, // +Z
    { { 0,  0, -1}, {-1,  0,  0}, { 0,  1,  0} }, // -Z
};

bool CreateCubemap(int size, int mipLevels, DXGI_FORMAT fmt,
                   ID3D11Texture2D*& outTex, ID3D11ShaderResourceView*& outSRV) {
    auto* dev = DX11Context::GetDevice();
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = td.Height = (UINT)size;
    td.MipLevels = (UINT)mipLevels;
    td.ArraySize = 6;
    td.Format    = fmt;
    td.SampleDesc.Count = 1;
    td.Usage     = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    td.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
    if (mipLevels > 1) td.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
    if (FAILED(dev->CreateTexture2D(&td, nullptr, &outTex))) return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
    sd.Format        = fmt;
    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    sd.TextureCube.MipLevels       = mipLevels;
    sd.TextureCube.MostDetailedMip = 0;
    return SUCCEEDED(dev->CreateShaderResourceView(outTex, &sd, &outSRV));
}

bool CreateFaceRTV(ID3D11Texture2D* tex, int face, int mip, DXGI_FORMAT fmt,
                   ID3D11RenderTargetView*& outRTV) {
    D3D11_RENDER_TARGET_VIEW_DESC rd = {};
    rd.Format        = fmt;
    rd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
    rd.Texture2DArray.MipSlice        = (UINT)mip;
    rd.Texture2DArray.FirstArraySlice = (UINT)face;
    rd.Texture2DArray.ArraySize       = 1;
    return SUCCEEDED(DX11Context::GetDevice()->CreateRenderTargetView(tex, &rd, &outRTV));
}

// Fullscreen-triangle draw with no VB / no IA. Caller must have set RT, viewport,
// shader, and any required SRVs/samplers/cbuffers before calling.
void DrawFullscreenTriangle(ID3D11DeviceContext* ctx) {
    UINT stride = 0, offset = 0;
    ID3D11Buffer* nullBuf = nullptr;
    ctx->IASetVertexBuffers(0, 1, &nullBuf, &stride, &offset);
    ctx->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->RSSetState(nullptr);
    ctx->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    ctx->OMSetDepthStencilState(DX11Context::GetDepthStencilState(), 0);
    ctx->Draw(3, 0);
}

void SetViewport(ID3D11DeviceContext* ctx, int w, int h) {
    D3D11_VIEWPORT vp = {};
    vp.Width = (float)w; vp.Height = (float)h;
    vp.MinDepth = 0.0f;  vp.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &vp);
}

// Render the procedural sky into 6 faces of s_EnvCube_Tex.
void RenderEnvCube(int size, const glm::vec3& sunDir) {
    auto* ctx     = DX11Context::GetContext();
    auto* sampler = DX11Context::GetSamplerState();
    s_SkyToCubeShader->use();
    ctx->PSSetSamplers(0, 1, &sampler);
    SetViewport(ctx, size, size);

    for (int f = 0; f < 6; f++) {
        ID3D11RenderTargetView* rtv = nullptr;
        if (!CreateFaceRTV(s_EnvCube_Tex, f, 0, DXGI_FORMAT_R16G16B16A16_FLOAT, rtv)) continue;

        s_SkyToCubeShader->setVec3("u_FaceForward",  kFaces[f].fwd);
        s_SkyToCubeShader->setVec3("u_FaceRight",    kFaces[f].right);
        s_SkyToCubeShader->setVec3("u_FaceUp",       kFaces[f].up);
        s_SkyToCubeShader->setVec3("u_SunDirection", sunDir);

        ctx->OMSetRenderTargets(1, &rtv, nullptr);
        DrawFullscreenTriangle(ctx);
        rtv->Release();
    }
}

void RenderIrradianceCube(int size) {
    auto* ctx     = DX11Context::GetContext();
    auto* sampler = DX11Context::GetSamplerState();
    s_IrradianceShader->use();
    ctx->PSSetShaderResources(0, 1, &s_EnvCube_SRV);
    ctx->PSSetSamplers(0, 1, &sampler);
    SetViewport(ctx, size, size);

    for (int f = 0; f < 6; f++) {
        ID3D11RenderTargetView* rtv = nullptr;
        if (!CreateFaceRTV(s_Irradiance_Tex, f, 0, DXGI_FORMAT_R16G16B16A16_FLOAT, rtv)) continue;

        s_IrradianceShader->setVec3("u_FaceForward", kFaces[f].fwd);
        s_IrradianceShader->setVec3("u_FaceRight",   kFaces[f].right);
        s_IrradianceShader->setVec3("u_FaceUp",      kFaces[f].up);

        ctx->OMSetRenderTargets(1, &rtv, nullptr);
        DrawFullscreenTriangle(ctx);
        rtv->Release();
    }
    ID3D11ShaderResourceView* nullSRV = nullptr;
    ctx->PSSetShaderResources(0, 1, &nullSRV);
}

void RenderPrefilterCube(int baseSize, int mipLevels) {
    auto* ctx     = DX11Context::GetContext();
    auto* sampler = DX11Context::GetSamplerState();
    s_PrefilterShader->use();
    ctx->PSSetShaderResources(0, 1, &s_EnvCube_SRV);
    ctx->PSSetSamplers(0, 1, &sampler);

    for (int mip = 0; mip < mipLevels; mip++) {
        int mipSize = std::max(1, baseSize >> mip);
        float roughness = (float)mip / (float)(mipLevels - 1);
        SetViewport(ctx, mipSize, mipSize);
        s_PrefilterShader->setFloat("u_Roughness", roughness);

        for (int f = 0; f < 6; f++) {
            ID3D11RenderTargetView* rtv = nullptr;
            if (!CreateFaceRTV(s_Prefilter_Tex, f, mip, DXGI_FORMAT_R16G16B16A16_FLOAT, rtv)) continue;

            s_PrefilterShader->setVec3("u_FaceForward", kFaces[f].fwd);
            s_PrefilterShader->setVec3("u_FaceRight",   kFaces[f].right);
            s_PrefilterShader->setVec3("u_FaceUp",      kFaces[f].up);

            ctx->OMSetRenderTargets(1, &rtv, nullptr);
            DrawFullscreenTriangle(ctx);
            rtv->Release();
        }
    }
    ID3D11ShaderResourceView* nullSRV = nullptr;
    ctx->PSSetShaderResources(0, 1, &nullSRV);
}

void RenderBrdfLUT(int size) {
    auto* dev = DX11Context::GetDevice();
    auto* ctx = DX11Context::GetContext();

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = td.Height = (UINT)size;
    td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R16G16_FLOAT;
    td.SampleDesc.Count = 1;
    td.Usage     = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(dev->CreateTexture2D(&td, nullptr, &s_BrdfLUT_Tex))) return;
    if (FAILED(dev->CreateShaderResourceView(s_BrdfLUT_Tex, nullptr, &s_BrdfLUT_SRV))) return;

    ID3D11RenderTargetView* rtv = nullptr;
    if (FAILED(dev->CreateRenderTargetView(s_BrdfLUT_Tex, nullptr, &rtv))) return;

    SetViewport(ctx, size, size);
    s_BrdfLUTShader->use();
    ctx->OMSetRenderTargets(1, &rtv, nullptr);
    DrawFullscreenTriangle(ctx);
    rtv->Release();
}

} // namespace

void PrecomputeIBL(const glm::vec3& sunDirection) {
    if (s_IBL_Ready) return;
    if (!s_SkyToCubeShader || !s_IrradianceShader || !s_PrefilterShader || !s_BrdfLUTShader) {
        std::cerr << "[IBL] missing shaders, skipping precompute\n";
        return;
    }

    const int ENV_SIZE        = 256;
    const int IRR_SIZE        = 32;
    const int PREFILTER_SIZE  = 128;
    const int PREFILTER_MIPS  = 5;
    const int BRDF_SIZE       = 512;

    if (!CreateCubemap(ENV_SIZE,       1,               DXGI_FORMAT_R16G16B16A16_FLOAT, s_EnvCube_Tex,    s_EnvCube_SRV))    return;
    if (!CreateCubemap(IRR_SIZE,       1,               DXGI_FORMAT_R16G16B16A16_FLOAT, s_Irradiance_Tex, s_Irradiance_SRV)) return;
    if (!CreateCubemap(PREFILTER_SIZE, PREFILTER_MIPS,  DXGI_FORMAT_R16G16B16A16_FLOAT, s_Prefilter_Tex,  s_Prefilter_SRV))  return;

    auto* ctx = DX11Context::GetContext();
    GPU_MARKER(ctx, "IBL Precompute");

    RenderEnvCube      (ENV_SIZE,       sunDirection);
    RenderIrradianceCube(IRR_SIZE);
    RenderPrefilterCube (PREFILTER_SIZE, PREFILTER_MIPS);
    RenderBrdfLUT       (BRDF_SIZE);

    s_IBL_Ready = true;
    std::cout << "[IBL] Precompute complete (env=" << ENV_SIZE
              << " irr=" << IRR_SIZE
              << " prefilter=" << PREFILTER_SIZE << "x" << PREFILTER_MIPS
              << " brdf=" << BRDF_SIZE << ")\n";
}

#endif // USE_DX11_BACKEND
