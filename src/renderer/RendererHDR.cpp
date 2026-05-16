#include "RendererState.h"
#include <iostream>

#ifdef USE_DX11_BACKEND
void EnsureSceneDepth(int w, int h) {
    if (w == s_Scene_DSV_W && h == s_Scene_DSV_H && s_Scene_DSV) return;
    auto* dev = DX11Context::GetDevice();
    auto rel = [](auto*& p) { if (p) { p->Release(); p = nullptr; } };
    rel(s_Scene_DSV); rel(s_Scene_DepthTex);

    D3D11_TEXTURE2D_DESC dd = {};
    dd.Width = (UINT)w; dd.Height = (UINT)h;
    dd.MipLevels = 1; dd.ArraySize = 1;
    dd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dd.SampleDesc.Count = 1;
    dd.Usage     = D3D11_USAGE_DEFAULT;
    dd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    HRESULT h1 = dev->CreateTexture2D(&dd, nullptr, &s_Scene_DepthTex);
    HRESULT h2 = s_Scene_DepthTex ? dev->CreateDepthStencilView(s_Scene_DepthTex, nullptr, &s_Scene_DSV) : E_FAIL;
    DX_CHECK(h1, "Scene depth tex"); DX_CHECK(h2, "Scene DSV");
    if (SUCCEEDED(h1) && SUCCEEDED(h2)) { s_Scene_DSV_W = w; s_Scene_DSV_H = h; }
}
#else
void EnsureSceneDepth(int, int) {}
#endif

void Renderer::BeginHDRPass(int w, int h) {
#ifdef USE_DX11_BACKEND
    EnsureSceneDepth(w, h);
    if (w != s_HDR_W || h != s_HDR_H) {
        auto* dev = DX11Context::GetDevice();
        auto rel = [](auto*& p) { if (p) { p->Release(); p = nullptr; } };
        rel(s_HDR_RTV); rel(s_HDR_SRV); rel(s_HDR_Tex);

        D3D11_TEXTURE2D_DESC td = {};
        td.Width = (UINT)w; td.Height = (UINT)h;
        td.MipLevels = 1; td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        td.SampleDesc.Count = 1;
        td.Usage     = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        HRESULT h1 = dev->CreateTexture2D(&td, nullptr, &s_HDR_Tex);
        HRESULT h2 = s_HDR_Tex ? dev->CreateRenderTargetView(s_HDR_Tex, nullptr, &s_HDR_RTV)  : E_FAIL;
        HRESULT h3 = s_HDR_Tex ? dev->CreateShaderResourceView(s_HDR_Tex, nullptr, &s_HDR_SRV) : E_FAIL;
        DX_CHECK(h1, "HDR texture"); DX_CHECK(h2, "HDR RTV"); DX_CHECK(h3, "HDR SRV");

        if (SUCCEEDED(h1) && SUCCEEDED(h2) && SUCCEEDED(h3)) { s_HDR_W = w; s_HDR_H = h; }
    }

    auto* ctx = DX11Context::GetContext();
    GPU_MARKER(ctx, "HDR Scene");

    float clear[] = { 0.1f, 0.1f, 0.15f, 1.0f };
    ctx->ClearRenderTargetView(s_HDR_RTV, clear);
    ctx->ClearDepthStencilView(s_Scene_DSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    ctx->OMSetRenderTargets(1, &s_HDR_RTV, s_Scene_DSV);
    D3D11_VIEWPORT vp = {};
    vp.Width    = (float)w;
    vp.Height   = (float)h;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &vp);
    ctx->RSSetState(DX11Context::GetRasterizerState());
#else
    if (w != s_HDR_W || h != s_HDR_H) {
        if (s_HDR_FBO) {
            glDeleteFramebuffers(1, &s_HDR_FBO);
            glDeleteTextures(1, &s_HDR_Tex);
            glDeleteRenderbuffers(1, &s_HDR_RBO);
        }
        glGenFramebuffers(1, &s_HDR_FBO);
        glGenTextures(1, &s_HDR_Tex);
        glGenRenderbuffers(1, &s_HDR_RBO);

        glBindTexture(GL_TEXTURE_2D, s_HDR_Tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindRenderbuffer(GL_RENDERBUFFER, s_HDR_RBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);

        glBindFramebuffer(GL_FRAMEBUFFER, s_HDR_FBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_HDR_Tex, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, s_HDR_RBO);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        s_HDR_W = w; s_HDR_H = h;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, s_HDR_FBO);
    glViewport(0, 0, w, h);
#endif
}

void Renderer::EndHDRPass() {
#ifdef USE_DX11_BACKEND
    auto* ctx = DX11Context::GetContext();
    auto* rtv = DX11Context::GetRTV();
    ctx->OMSetRenderTargets(1, &rtv, DX11Context::GetDSV());
    const auto& vp = DX11Context::GetViewport();
    ctx->RSSetViewports(1, &vp);
#else
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
}

// ---- Scene viewport (render-to-texture) ------------------------------------

void Renderer::ResizeViewport(int w, int h) {
    if (w <= 0 || h <= 0) return;
#ifdef USE_DX11_BACKEND
    if (w == s_View_W && h == s_View_H && s_View_Tex) return;
    auto* dev = DX11Context::GetDevice();
    auto rel = [](auto*& p) { if (p) { p->Release(); p = nullptr; } };
    rel(s_View_RTV); rel(s_View_SRV); rel(s_View_Tex);

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = (UINT)w; td.Height = (UINT)h;
    td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage     = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    HRESULT h1 = dev->CreateTexture2D(&td, nullptr, &s_View_Tex);
    HRESULT h2 = s_View_Tex ? dev->CreateRenderTargetView(s_View_Tex, nullptr, &s_View_RTV)  : E_FAIL;
    HRESULT h3 = s_View_Tex ? dev->CreateShaderResourceView(s_View_Tex, nullptr, &s_View_SRV) : E_FAIL;
    DX_CHECK(h1, "View texture"); DX_CHECK(h2, "View RTV"); DX_CHECK(h3, "View SRV");
    if (SUCCEEDED(h1) && SUCCEEDED(h2) && SUCCEEDED(h3)) { s_View_W = w; s_View_H = h; }
#else
    if (w == s_View_W && h == s_View_H && s_View_FBO) return;
    if (s_View_FBO) { glDeleteFramebuffers(1, &s_View_FBO); s_View_FBO = 0; }
    if (s_View_Tex) { glDeleteTextures(1, &s_View_Tex);     s_View_Tex = 0; }
    glGenFramebuffers(1, &s_View_FBO);
    glGenTextures(1, &s_View_Tex);

    glBindTexture(GL_TEXTURE_2D, s_View_Tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, s_View_FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_View_Tex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    s_View_W = w; s_View_H = h;
#endif
}

void Renderer::ApplyTonemapToViewport() {
    if (!s_TonemapShader) return;
    if (s_View_W <= 0 || s_View_H <= 0) return;
#ifdef USE_DX11_BACKEND
    if (!s_View_RTV || !s_HDR_SRV) return;
    auto* ctx = DX11Context::GetContext();
    GPU_MARKER(ctx, "Tonemap->Viewport");

    float clear[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    ctx->ClearRenderTargetView(s_View_RTV, clear);
    ctx->OMSetRenderTargets(1, &s_View_RTV, nullptr);

    D3D11_VIEWPORT vp = {};
    vp.Width    = (float)s_View_W;
    vp.Height   = (float)s_View_H;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &vp);

    s_TonemapShader->setFloat("u_Exposure",      s_Exposure);
    s_TonemapShader->setFloat("u_BloomIntensity", s_BloomIntensity);
    s_TonemapShader->use();
    ctx->PSSetShaderResources(0, 1, &s_HDR_SRV);
    ctx->PSSetShaderResources(1, 1, &s_BloomA_SRV);
    ctx->PSSetSamplers(0, 1, &s_TmSampler);

    ctx->RSSetState(nullptr);
    ctx->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    ctx->OMSetDepthStencilState(DX11Context::GetDepthStencilState(), 0);

    UINT stride = 0, offset = 0;
    ID3D11Buffer* nullBuf = nullptr;
    ctx->IASetVertexBuffers(0, 1, &nullBuf, &stride, &offset);
    ctx->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->Draw(3, 0);

    ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
    ctx->PSSetShaderResources(0, 2, nullSRVs);

    // Restore backbuffer as render target so ImGui draws onto window
    auto* rtv = DX11Context::GetRTV();
    ctx->OMSetRenderTargets(1, &rtv, DX11Context::GetDSV());
    const auto& bbvp = DX11Context::GetViewport();
    ctx->RSSetViewports(1, &bbvp);
#else
    if (!s_View_FBO) return;
    glBindFramebuffer(GL_FRAMEBUFFER, s_View_FBO);
    glViewport(0, 0, s_View_W, s_View_H);
    glDisable(GL_DEPTH_TEST);
    s_TonemapShader->use();
    s_TonemapShader->setInt("u_HDRBuffer", 0);
    s_TonemapShader->setFloat("u_Exposure", s_Exposure);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_HDR_Tex);
    glBindVertexArray(s_HDR_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
}

void* Renderer::GetViewportTextureID() {
#ifdef USE_DX11_BACKEND
    return (void*)s_View_SRV;
#else
    return (void*)(uintptr_t)s_View_Tex;
#endif
}

int Renderer::GetViewportWidth()  { return s_View_W; }
int Renderer::GetViewportHeight() { return s_View_H; }

void Renderer::ClearBackbuffer(int w, int h) {
#ifdef USE_DX11_BACKEND
    (void)w; (void)h;
    auto* ctx = DX11Context::GetContext();
    auto* rtv = DX11Context::GetRTV();
    float clr[] = { 0.07f, 0.07f, 0.08f, 1.0f };
    ctx->ClearRenderTargetView(rtv, clr);
    ctx->OMSetRenderTargets(1, &rtv, DX11Context::GetDSV());
#else
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, w, h);
    glClearColor(0.07f, 0.07f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
}

void Renderer::ApplyTonemap() {
    if (!s_TonemapShader) return;
#ifdef USE_DX11_BACKEND
    auto* ctx = DX11Context::GetContext();
    GPU_MARKER(ctx, "Tonemap");

    if (!s_HDR_SRV) { std::cerr << "[Tonemap] HDR SRV is null\n"; return; }

    auto* rtv = DX11Context::GetRTV();
    float clear[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    ctx->ClearRenderTargetView(rtv, clear);

    s_TonemapShader->setFloat("u_Exposure", s_Exposure);
    s_TonemapShader->setFloat("u_BloomIntensity", s_BloomIntensity);
    s_TonemapShader->use();
    ctx->PSSetShaderResources(0, 1, &s_HDR_SRV);
    ctx->PSSetShaderResources(1, 1, &s_BloomA_SRV);
    ctx->PSSetSamplers(0, 1, &s_TmSampler);

    ctx->RSSetState(nullptr);  // default: CW=front for NDC-CW fullscreen triangle
    ctx->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    ctx->OMSetDepthStencilState(DX11Context::GetDepthStencilState(), 0);
    ctx->OMSetRenderTargets(1, &rtv, nullptr);

    UINT stride = 0, offset = 0;
    ID3D11Buffer* nullBuf = nullptr;
    ctx->IASetVertexBuffers(0, 1, &nullBuf, &stride, &offset);
    ctx->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->Draw(3, 0);

    ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
    ctx->PSSetShaderResources(0, 2, nullSRVs);
    auto* wrapSampler = DX11Context::GetSamplerState();
    ctx->PSSetSamplers(0, 1, &wrapSampler);
#else
    glDisable(GL_DEPTH_TEST);
    s_TonemapShader->use();
    s_TonemapShader->setInt("u_HDRBuffer", 0);
    s_TonemapShader->setFloat("u_Exposure", s_Exposure);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_HDR_Tex);
    glBindVertexArray(s_HDR_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
#endif
}
