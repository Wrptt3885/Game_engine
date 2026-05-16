#include "RendererState.h"
#include "core/Scene.h"
#include "core/GameObject.h"
#include "renderer/MeshRenderer.h"
#include "core/Camera.h"

#ifdef USE_DX11_BACKEND

static void CreateSSAOResources(int w, int h) {
    auto* dev = DX11Context::GetDevice();
    auto rel = [](auto*& p) { if (p) { p->Release(); p = nullptr; } };
    rel(s_GBuf_PosTex); rel(s_GBuf_PosRTV); rel(s_GBuf_PosSRV);
    rel(s_GBuf_NorTex); rel(s_GBuf_NorRTV); rel(s_GBuf_NorSRV);
    rel(s_SSAO_Tex);    rel(s_SSAO_RTV);    rel(s_SSAO_SRV);
    rel(s_SSAOBlur_Tex);rel(s_SSAOBlur_RTV);rel(s_SSAOBlur_SRV);

    auto makeRT = [&](DXGI_FORMAT fmt,
                      ID3D11Texture2D** tex,
                      ID3D11RenderTargetView** rtv,
                      ID3D11ShaderResourceView** srv) -> bool {
        D3D11_TEXTURE2D_DESC td = {};
        td.Width = (UINT)w; td.Height = (UINT)h;
        td.MipLevels = 1; td.ArraySize = 1; td.Format = fmt;
        td.SampleDesc.Count = 1; td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        if (FAILED(dev->CreateTexture2D(&td, nullptr, tex)))            return false;
        if (FAILED(dev->CreateRenderTargetView(*tex, nullptr, rtv)))    return false;
        if (FAILED(dev->CreateShaderResourceView(*tex, nullptr, srv)))  return false;
        return true;
    };

    bool ok = true;
    ok &= makeRT(DXGI_FORMAT_R32G32B32A32_FLOAT, &s_GBuf_PosTex, &s_GBuf_PosRTV, &s_GBuf_PosSRV);
    ok &= makeRT(DXGI_FORMAT_R16G16B16A16_FLOAT, &s_GBuf_NorTex, &s_GBuf_NorRTV, &s_GBuf_NorSRV);
    ok &= makeRT(DXGI_FORMAT_R8_UNORM,           &s_SSAO_Tex,     &s_SSAO_RTV,     &s_SSAO_SRV);
    ok &= makeRT(DXGI_FORMAT_R8_UNORM,           &s_SSAOBlur_Tex, &s_SSAOBlur_RTV, &s_SSAOBlur_SRV);

    if (!ok) DX_CHECK(E_FAIL, "SSAO G-buffer resource creation");
    else     { s_GBuf_W = w; s_GBuf_H = h; }
}

#endif // USE_DX11_BACKEND

void Renderer::GBufferPass(Scene& scene, Camera& cam, int w, int h) {
#ifdef USE_DX11_BACKEND
    if (!s_GPassShader) return;
    if (w != s_GBuf_W || h != s_GBuf_H) CreateSSAOResources(w, h);
    if (!s_GBuf_PosRTV || !s_GBuf_NorRTV) return;

    // Shared scene-sized depth target — backbuffer DSV would be the wrong
    // size in scene-viewport mode.
    EnsureSceneDepth(w, h);

    auto* ctx = DX11Context::GetContext();
    GPU_MARKER(ctx, "G-Buffer Pass");

    float zeros[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    ctx->ClearRenderTargetView(s_GBuf_PosRTV, zeros);
    ctx->ClearRenderTargetView(s_GBuf_NorRTV, zeros);
    ctx->ClearDepthStencilView(s_Scene_DSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

    ID3D11RenderTargetView* rtvs[] = { s_GBuf_PosRTV, s_GBuf_NorRTV };
    ctx->OMSetRenderTargets(2, rtvs, s_Scene_DSV);

    D3D11_VIEWPORT vp = {};
    vp.Width    = (float)w;
    vp.Height   = (float)h;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &vp);
    ctx->RSSetState(DX11Context::GetRasterizerState());

    s_GPassShader->setMat4("u_View",       cam.GetViewMatrix());
    s_GPassShader->setMat4("u_Projection", cam.GetProjectionMatrix());

    for (size_t i = 0; i < scene.GetGameObjectCount(); i++) {
        auto obj = scene.GetGameObject(i);
        if (!obj || !obj->IsActive()) continue;
        auto mr = obj->GetComponent<MeshRenderer>();
        if (!mr || !mr->GetMesh()) continue;
        s_GPassShader->setMat4("u_Model", obj->GetTransform().GetMatrix());
        s_GPassShader->use();
        mr->GetMesh()->Draw();
    }

    ID3D11RenderTargetView* nullRTVs[] = { nullptr, nullptr };
    ctx->OMSetRenderTargets(2, nullRTVs, nullptr);
#endif
}

void Renderer::SSAOPass(Camera& cam, int w, int h) {
#ifdef USE_DX11_BACKEND
    if (!s_SSAOShader || !s_GBuf_PosSRV || !s_GBuf_NorSRV || !s_SSAO_RTV) return;

    auto* ctx = DX11Context::GetContext();
    GPU_MARKER(ctx, "SSAO Pass");

    float ones[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    ctx->ClearRenderTargetView(s_SSAO_RTV, ones);
    ctx->OMSetRenderTargets(1, &s_SSAO_RTV, nullptr);
    ctx->RSSetState(nullptr);

    static const char* kKernelNames[32] = {
        "u_Kernel[0]", "u_Kernel[1]", "u_Kernel[2]", "u_Kernel[3]",
        "u_Kernel[4]", "u_Kernel[5]", "u_Kernel[6]", "u_Kernel[7]",
        "u_Kernel[8]", "u_Kernel[9]", "u_Kernel[10]","u_Kernel[11]",
        "u_Kernel[12]","u_Kernel[13]","u_Kernel[14]","u_Kernel[15]",
        "u_Kernel[16]","u_Kernel[17]","u_Kernel[18]","u_Kernel[19]",
        "u_Kernel[20]","u_Kernel[21]","u_Kernel[22]","u_Kernel[23]",
        "u_Kernel[24]","u_Kernel[25]","u_Kernel[26]","u_Kernel[27]",
        "u_Kernel[28]","u_Kernel[29]","u_Kernel[30]","u_Kernel[31]",
    };
    s_SSAOShader->setMat4("u_Projection", cam.GetProjectionMatrix());
    for (int k = 0; k < 32; k++)
        s_SSAOShader->setVec3(kKernelNames[k], s_SSAOKernel[k]);
    s_SSAOShader->setFloat("u_NoiseScaleX", (float)w / 4.0f);
    s_SSAOShader->setFloat("u_NoiseScaleY", (float)h / 4.0f);
    s_SSAOShader->setFloat("u_Radius",      0.5f);
    s_SSAOShader->setFloat("u_Bias",        0.025f);
    s_SSAOShader->use();

    ctx->PSSetShaderResources(0, 1, &s_GBuf_PosSRV);
    ctx->PSSetShaderResources(1, 1, &s_GBuf_NorSRV);
    ctx->PSSetShaderResources(2, 1, &s_NoiseSRV);
    ctx->PSSetSamplers(0, 1, &s_PointClampSampler);
    ctx->PSSetSamplers(1, 1, &s_PointWrapSampler);

    UINT stride = 0, offset = 0;
    ID3D11Buffer* nullBuf = nullptr;
    ctx->IASetVertexBuffers(0, 1, &nullBuf, &stride, &offset);
    ctx->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->Draw(3, 0);

    ID3D11ShaderResourceView* nullSRVs[3] = {};
    ctx->PSSetShaderResources(0, 3, nullSRVs);
    ID3D11RenderTargetView* nullRTV = nullptr;
    ctx->OMSetRenderTargets(1, &nullRTV, nullptr);
#endif
}

void Renderer::SSAOBlurPass() {
#ifdef USE_DX11_BACKEND
    if (!s_SSAOBlurShader || !s_SSAO_SRV || !s_SSAOBlur_RTV) return;

    auto* ctx = DX11Context::GetContext();
    GPU_MARKER(ctx, "SSAO Blur");

    float ones[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    ctx->ClearRenderTargetView(s_SSAOBlur_RTV, ones);
    ctx->OMSetRenderTargets(1, &s_SSAOBlur_RTV, nullptr);
    ctx->RSSetState(nullptr);

    s_SSAOBlurShader->use();
    ctx->PSSetShaderResources(0, 1, &s_SSAO_SRV);
    ctx->PSSetSamplers(0, 1, &s_PointClampSampler);

    UINT stride = 0, offset = 0;
    ID3D11Buffer* nullBuf = nullptr;
    ctx->IASetVertexBuffers(0, 1, &nullBuf, &stride, &offset);
    ctx->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->Draw(3, 0);

    ID3D11ShaderResourceView* nullSRV = nullptr;
    ctx->PSSetShaderResources(0, 1, &nullSRV);
    ID3D11RenderTargetView* nullRTV = nullptr;
    ctx->OMSetRenderTargets(1, &nullRTV, nullptr);

    auto* wrapSampler = DX11Context::GetSamplerState();
    ctx->PSSetSamplers(0, 1, &wrapSampler);
    ID3D11SamplerState* nullSamp = nullptr;
    ctx->PSSetSamplers(1, 1, &nullSamp);
#endif
}
