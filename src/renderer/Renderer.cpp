#include "RendererState.h"
#include "rhi/RHI.h"
#include "renderer/Mesh.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <random>
#include <cmath>

// ---- Class-static definitions -----------------------------------------------

std::shared_ptr<Shader> Renderer::s_Shader                = nullptr;
std::shared_ptr<Shader> Renderer::s_DepthShader           = nullptr;
std::shared_ptr<Shader> Renderer::s_TonemapShader         = nullptr;
std::shared_ptr<Shader> Renderer::s_BloomThresholdShader  = nullptr;
std::shared_ptr<Shader> Renderer::s_BloomBlurShader       = nullptr;
std::shared_ptr<Shader> Renderer::s_PointDepthShader      = nullptr;
glm::mat4               Renderer::s_LightSpaceMatrix      = glm::mat4(1.0f);
float                   Renderer::s_Exposure              = 1.0f;
float                   Renderer::s_BloomThreshold        = 1.0f;
float                   Renderer::s_BloomIntensity        = 0.5f;
glm::vec3               Renderer::s_PointShadowLightPos   = glm::vec3(0.0f);
float                   Renderer::s_PointShadowFarPlane   = 10.0f;
bool                    Renderer::s_HasPointShadow        = false;
int                     Renderer::s_HDR_W                 = 0;
int                     Renderer::s_HDR_H                 = 0;

// ---- File-scope state (extern-declared in RendererState.h) ------------------

#ifdef USE_DX11_BACKEND
DX11ShadowMap*            s_DX11Shadow        = nullptr;
DX11Skybox*               s_DX11Sky           = nullptr;
ID3D11RasterizerState*    s_ShadowRS          = nullptr;
DX11PointShadow*          s_DX11PointShadow   = nullptr;
ID3D11Texture2D*          s_HDR_Tex           = nullptr;
ID3D11RenderTargetView*   s_HDR_RTV           = nullptr;
ID3D11ShaderResourceView* s_HDR_SRV           = nullptr;
ID3D11SamplerState*       s_TmSampler         = nullptr;
ID3D11Texture2D*          s_GBuf_PosTex       = nullptr;
ID3D11RenderTargetView*   s_GBuf_PosRTV       = nullptr;
ID3D11ShaderResourceView* s_GBuf_PosSRV       = nullptr;
ID3D11Texture2D*          s_GBuf_NorTex       = nullptr;
ID3D11RenderTargetView*   s_GBuf_NorRTV       = nullptr;
ID3D11ShaderResourceView* s_GBuf_NorSRV       = nullptr;
ID3D11Texture2D*          s_SSAO_Tex          = nullptr;
ID3D11RenderTargetView*   s_SSAO_RTV          = nullptr;
ID3D11ShaderResourceView* s_SSAO_SRV          = nullptr;
ID3D11Texture2D*          s_SSAOBlur_Tex      = nullptr;
ID3D11RenderTargetView*   s_SSAOBlur_RTV      = nullptr;
ID3D11ShaderResourceView* s_SSAOBlur_SRV      = nullptr;
ID3D11Texture2D*          s_NoiseTex          = nullptr;
ID3D11ShaderResourceView* s_NoiseSRV          = nullptr;
std::shared_ptr<Shader>   s_GPassShader       = nullptr;
std::shared_ptr<Shader>   s_SSAOShader        = nullptr;
std::shared_ptr<Shader>   s_SSAOBlurShader    = nullptr;
ID3D11SamplerState*       s_PointClampSampler = nullptr;
ID3D11SamplerState*       s_PointWrapSampler  = nullptr;
std::vector<glm::vec3>    s_SSAOKernel;
int                       s_GBuf_W = 0, s_GBuf_H = 0;

ID3D11Texture2D*          s_BloomA_Tex  = nullptr;
ID3D11RenderTargetView*   s_BloomA_RTV  = nullptr;
ID3D11ShaderResourceView* s_BloomA_SRV  = nullptr;
ID3D11Texture2D*          s_BloomB_Tex  = nullptr;
ID3D11RenderTargetView*   s_BloomB_RTV  = nullptr;
ID3D11ShaderResourceView* s_BloomB_SRV  = nullptr;
int                       s_Bloom_W = 0, s_Bloom_H = 0;
#else
ShadowMap*   s_ShadowMap = nullptr;
Skybox*      s_Skybox    = nullptr;
unsigned int s_HDR_FBO   = 0;
unsigned int s_HDR_Tex   = 0;
unsigned int s_HDR_RBO   = 0;
unsigned int s_HDR_VAO   = 0;
#endif

// ---- Init -------------------------------------------------------------------

void Renderer::Init() {
#ifdef USE_DX11_BACKEND
    RHI::Init(GraphicsAPI::DirectX11);
    s_Shader      = Shader::Create(SHADER_DIR "/dx11/standard.hlsl", SHADER_DIR "/dx11/standard.hlsl");
    s_DepthShader = Shader::Create(SHADER_DIR "/dx11/depth.hlsl",    SHADER_DIR "/dx11/depth.hlsl");
    s_DX11Shadow    = new DX11ShadowMap(4096, 4096);
    s_DX11Sky       = new DX11Skybox();
    s_TonemapShader = Shader::Create(SHADER_DIR "/dx11/tonemap.hlsl", SHADER_DIR "/dx11/tonemap.hlsl");

    D3D11_SAMPLER_DESC sd = {};
    sd.Filter   = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    DX_CHECK(DX11Context::GetDevice()->CreateSamplerState(&sd, &s_TmSampler), "tonemap sampler");

    s_GPassShader          = Shader::Create(SHADER_DIR "/dx11/gpass.hlsl",           SHADER_DIR "/dx11/gpass.hlsl");
    s_SSAOShader           = Shader::Create(SHADER_DIR "/dx11/ssao.hlsl",            SHADER_DIR "/dx11/ssao.hlsl");
    s_SSAOBlurShader       = Shader::Create(SHADER_DIR "/dx11/ssao_blur.hlsl",       SHADER_DIR "/dx11/ssao_blur.hlsl");
    s_BloomThresholdShader = Shader::Create(SHADER_DIR "/dx11/bloom_threshold.hlsl", SHADER_DIR "/dx11/bloom_threshold.hlsl");
    s_BloomBlurShader      = Shader::Create(SHADER_DIR "/dx11/bloom_blur.hlsl",      SHADER_DIR "/dx11/bloom_blur.hlsl");
    s_PointDepthShader     = Shader::Create(SHADER_DIR "/dx11/point_shadow.hlsl",    SHADER_DIR "/dx11/point_shadow.hlsl");
    s_DX11PointShadow      = new DX11PointShadow(512);

    {
        D3D11_SAMPLER_DESC sd2 = {};
        sd2.Filter             = D3D11_FILTER_MIN_MAG_MIP_POINT;
        sd2.AddressU = sd2.AddressV = sd2.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd2.ComparisonFunc     = D3D11_COMPARISON_NEVER;
        sd2.MaxLOD             = D3D11_FLOAT32_MAX;
        DX_CHECK(DX11Context::GetDevice()->CreateSamplerState(&sd2, &s_PointClampSampler), "SSAO clamp sampler");
        sd2.AddressU = sd2.AddressV = sd2.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        DX_CHECK(DX11Context::GetDevice()->CreateSamplerState(&sd2, &s_PointWrapSampler), "SSAO wrap sampler");
    }

    {
        std::default_random_engine gen(42);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        for (int i = 0; i < 32; i++) {
            glm::vec3 s(dist(gen)*2.0f-1.0f, dist(gen)*2.0f-1.0f, dist(gen));
            s = glm::normalize(s) * dist(gen);
            float t = (float)i / 32.0f;
            s_SSAOKernel.push_back(s * (0.1f + t * t * 0.9f));
        }
    }

    {
        std::default_random_engine gen(123);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::vector<float> nd;
        for (int i = 0; i < 16; i++) {
            float a = dist(gen) * 2.0f * 3.14159265f;
            nd.push_back(cosf(a)); nd.push_back(sinf(a));
            nd.push_back(0.0f);   nd.push_back(0.0f);
        }
        D3D11_TEXTURE2D_DESC ntd = {};
        ntd.Width = ntd.Height = 4;
        ntd.MipLevels = ntd.ArraySize = 1;
        ntd.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        ntd.SampleDesc.Count = 1;
        ntd.Usage     = D3D11_USAGE_IMMUTABLE;
        ntd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA nsd = { nd.data(), 4 * 4 * sizeof(float), 0 };
        if (SUCCEEDED(DX11Context::GetDevice()->CreateTexture2D(&ntd, &nsd, &s_NoiseTex)))
            DX_CHECK(DX11Context::GetDevice()->CreateShaderResourceView(s_NoiseTex, nullptr, &s_NoiseSRV),
                     "SSAO noise SRV");
    }

    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode              = D3D11_FILL_SOLID;
    rd.CullMode              = D3D11_CULL_FRONT;
    rd.FrontCounterClockwise = TRUE;
    rd.DepthClipEnable       = TRUE;
    rd.DepthBias             = 0;
    rd.SlopeScaledDepthBias  = 0.5f;
    DX_CHECK(DX11Context::GetDevice()->CreateRasterizerState(&rd, &s_ShadowRS), "shadow RS");
#else
    RHI::Init(GraphicsAPI::OpenGL);
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return;
    }
    glEnable(GL_DEPTH_TEST);
    s_Shader        = Shader::Create(SHADER_DIR "/standard/default.vert", SHADER_DIR "/standard/default.frag");
    s_DepthShader   = Shader::Create(SHADER_DIR "/depth/depth.vert",      SHADER_DIR "/depth/depth.frag");
    s_TonemapShader = Shader::Create(SHADER_DIR "/postprocess/tonemap.vert", SHADER_DIR "/postprocess/tonemap.frag");
    s_ShadowMap     = new ShadowMap(2048, 2048);
    s_Skybox        = new Skybox();
    glGenVertexArrays(1, &s_HDR_VAO);
#endif
}

// ---- Shutdown ---------------------------------------------------------------

void Renderer::Shutdown() {
    s_Shader.reset(); s_DepthShader.reset(); s_TonemapShader.reset();
#ifdef USE_DX11_BACKEND
    delete s_DX11Shadow; s_DX11Shadow = nullptr;
    delete s_DX11Sky;    s_DX11Sky    = nullptr;
    auto rel = [](auto*& p) { if (p) { p->Release(); p = nullptr; } };
    rel(s_ShadowRS);
    rel(s_HDR_RTV); rel(s_HDR_SRV); rel(s_HDR_Tex); rel(s_TmSampler);
    s_GPassShader.reset(); s_SSAOShader.reset(); s_SSAOBlurShader.reset();
    rel(s_GBuf_PosTex); rel(s_GBuf_PosRTV); rel(s_GBuf_PosSRV);
    rel(s_GBuf_NorTex); rel(s_GBuf_NorRTV); rel(s_GBuf_NorSRV);
    rel(s_SSAO_Tex);     rel(s_SSAO_RTV);     rel(s_SSAO_SRV);
    rel(s_SSAOBlur_Tex); rel(s_SSAOBlur_RTV); rel(s_SSAOBlur_SRV);
    rel(s_NoiseTex); rel(s_NoiseSRV);
    rel(s_PointClampSampler); rel(s_PointWrapSampler);
    s_SSAOKernel.clear();
    s_BloomThresholdShader.reset(); s_BloomBlurShader.reset();
    rel(s_BloomA_Tex); rel(s_BloomA_RTV); rel(s_BloomA_SRV);
    rel(s_BloomB_Tex); rel(s_BloomB_RTV); rel(s_BloomB_SRV);
    s_PointDepthShader.reset();
    delete s_DX11PointShadow; s_DX11PointShadow = nullptr;
#else
    delete s_ShadowMap; s_ShadowMap = nullptr;
    delete s_Skybox;    s_Skybox    = nullptr;
    if (s_HDR_FBO) { glDeleteFramebuffers(1, &s_HDR_FBO);     s_HDR_FBO = 0; }
    if (s_HDR_Tex) { glDeleteTextures(1, &s_HDR_Tex);         s_HDR_Tex = 0; }
    if (s_HDR_RBO) { glDeleteRenderbuffers(1, &s_HDR_RBO);    s_HDR_RBO = 0; }
    if (s_HDR_VAO) { glDeleteVertexArrays(1, &s_HDR_VAO);     s_HDR_VAO = 0; }
#endif
}

// ---- Clear ------------------------------------------------------------------

void Renderer::Clear() {
#ifdef USE_DX11_BACKEND
    auto* ctx = DX11Context::GetContext();
    float color[] = { 0.1f, 0.1f, 0.15f, 1.0f };
    auto* rtv = s_HDR_RTV ? s_HDR_RTV : DX11Context::GetRTV();
    ctx->ClearRenderTargetView(rtv, color);
    ctx->ClearDepthStencilView(DX11Context::GetDSV(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    ctx->OMSetRenderTargets(1, &rtv, DX11Context::GetDSV());
    const auto& vp = DX11Context::GetViewport();
    ctx->RSSetViewports(1, &vp);
#else
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
}
