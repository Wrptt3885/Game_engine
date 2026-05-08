#include "RendererState.h"
#include <algorithm>

void Renderer::BloomPass(int w, int h) {
#ifdef USE_DX11_BACKEND
    if (!s_BloomThresholdShader || !s_BloomBlurShader || !s_HDR_SRV) return;

    int bw = std::max(1, w / 2), bh = std::max(1, h / 2);

    if (bw != s_Bloom_W || bh != s_Bloom_H) {
        auto* dev = DX11Context::GetDevice();
        auto rel  = [](auto*& p) { if (p) { p->Release(); p = nullptr; } };
        rel(s_BloomA_Tex); rel(s_BloomA_RTV); rel(s_BloomA_SRV);
        rel(s_BloomB_Tex); rel(s_BloomB_RTV); rel(s_BloomB_SRV);

        D3D11_TEXTURE2D_DESC td = {};
        td.Width  = (UINT)bw; td.Height = (UINT)bh;
        td.MipLevels = 1; td.ArraySize = 1;
        td.Format    = DXGI_FORMAT_R16G16B16A16_FLOAT;
        td.SampleDesc.Count = 1;
        td.Usage     = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        auto make = [&](ID3D11Texture2D*& tex, ID3D11RenderTargetView*& rtv,
                        ID3D11ShaderResourceView*& srv, const char* name) {
            if (SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &tex))) {
                DX_CHECK(dev->CreateRenderTargetView(tex, nullptr, &rtv), name);
                DX_CHECK(dev->CreateShaderResourceView(tex, nullptr, &srv), name);
            }
        };
        make(s_BloomA_Tex, s_BloomA_RTV, s_BloomA_SRV, "BloomA");
        make(s_BloomB_Tex, s_BloomB_RTV, s_BloomB_SRV, "BloomB");
        s_Bloom_W = bw; s_Bloom_H = bh;
    }

    auto* ctx = DX11Context::GetContext();
    GPU_MARKER(ctx, "Bloom");

    // Fullscreen triangle state — no vertex buffer, no depth
    UINT stride = 0, offset = 0;
    ID3D11Buffer* nullBuf = nullptr;
    ctx->IASetVertexBuffers(0, 1, &nullBuf, &stride, &offset);
    ctx->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->RSSetState(nullptr);
    ctx->OMSetDepthStencilState(DX11Context::GetDepthStencilState(), 0);
    ctx->OMSetBlendState(nullptr, nullptr, 0xffffffff);

    D3D11_VIEWPORT bvp = {};
    bvp.Width    = (float)bw; bvp.Height = (float)bh;
    bvp.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &bvp);

    ID3D11ShaderResourceView* nullSRV = nullptr;
    float clear[] = { 0.0f, 0.0f, 0.0f, 1.0f };

    // Pass 1: Threshold — HDR (full-res, bilinear downsample) → BloomA (half-res)
    s_BloomThresholdShader->setFloat("u_Threshold", s_BloomThreshold);
    s_BloomThresholdShader->setFloat("u_Knee", 0.1f);
    s_BloomThresholdShader->use();
    ctx->OMSetRenderTargets(1, &s_BloomA_RTV, nullptr);
    ctx->ClearRenderTargetView(s_BloomA_RTV, clear);
    ctx->PSSetShaderResources(0, 1, &s_HDR_SRV);
    ctx->PSSetSamplers(0, 1, &s_TmSampler);
    ctx->Draw(3, 0);
    ctx->PSSetShaderResources(0, 1, &nullSRV);

    // Pass 2: Horizontal Gaussian blur — BloomA → BloomB
    s_BloomBlurShader->setFloat("u_TexelW", 1.0f / bw);
    s_BloomBlurShader->setFloat("u_TexelH", 1.0f / bh);
    s_BloomBlurShader->setFloat("u_DirX", 1.0f);
    s_BloomBlurShader->setFloat("u_DirY", 0.0f);
    s_BloomBlurShader->use();
    ctx->OMSetRenderTargets(1, &s_BloomB_RTV, nullptr);
    ctx->ClearRenderTargetView(s_BloomB_RTV, clear);
    ctx->PSSetShaderResources(0, 1, &s_BloomA_SRV);
    ctx->Draw(3, 0);
    ctx->PSSetShaderResources(0, 1, &nullSRV);

    // Pass 3: Vertical Gaussian blur — BloomB → BloomA (final bloom in BloomA_SRV)
    s_BloomBlurShader->setFloat("u_DirX", 0.0f);
    s_BloomBlurShader->setFloat("u_DirY", 1.0f);
    s_BloomBlurShader->use();
    ctx->OMSetRenderTargets(1, &s_BloomA_RTV, nullptr);
    ctx->ClearRenderTargetView(s_BloomA_RTV, clear);
    ctx->PSSetShaderResources(0, 1, &s_BloomB_SRV);
    ctx->Draw(3, 0);
    ctx->PSSetShaderResources(0, 1, &nullSRV);

    // Restore viewport and render target for subsequent passes
    const auto& vp = DX11Context::GetViewport();
    ctx->RSSetViewports(1, &vp);
    auto* rtv = DX11Context::GetRTV();
    ctx->OMSetRenderTargets(1, &rtv, DX11Context::GetDSV());
#endif
}
