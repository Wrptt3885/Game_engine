#include "RendererState.h"
#include <iostream>

void Renderer::BeginHDRPass(int w, int h) {
#ifdef USE_DX11_BACKEND
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
    ctx->ClearDepthStencilView(DX11Context::GetDSV(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    ctx->OMSetRenderTargets(1, &s_HDR_RTV, DX11Context::GetDSV());
    const auto& vp = DX11Context::GetViewport();
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

void Renderer::ApplyTonemap() {
    if (!s_TonemapShader) return;
#ifdef USE_DX11_BACKEND
    auto* ctx = DX11Context::GetContext();
    GPU_MARKER(ctx, "Tonemap");

    auto* rtv = DX11Context::GetRTV();
    float clear[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    ctx->ClearRenderTargetView(rtv, clear);
    ctx->OMSetRenderTargets(1, &rtv, DX11Context::GetDSV());

    if (!s_HDR_SRV) { std::cerr << "[Tonemap] HDR SRV is null\n"; return; }

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
