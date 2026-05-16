// Implementation-only header: shared static state across Renderer modules.
// Include ONLY in src/renderer/Renderer*.cpp — never in user-facing headers.
#pragma once

#include "renderer/Renderer.h"
#include "renderer/Shader.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

// Pipeline state shared across passes — file-scope by convention to keep
// public Renderer.h slim. Set by passes that own them, read by others.
extern std::shared_ptr<Shader> s_Shader;
extern std::shared_ptr<Shader> s_DepthShader;
extern std::shared_ptr<Shader> s_TonemapShader;
extern std::shared_ptr<Shader> s_BloomThresholdShader;
extern std::shared_ptr<Shader> s_BloomBlurShader;
extern std::shared_ptr<Shader> s_PointDepthShader;
extern glm::mat4               s_LightSpaceMatrix;
extern glm::vec3               s_PointShadowLightPos;
extern float                   s_PointShadowFarPlane;
extern bool                    s_HasPointShadow;
extern int                     s_HDR_W, s_HDR_H;

#ifdef USE_DX11_BACKEND
#  include "rhi/dx11/DX11Context.h"
#  include "rhi/dx11/DX11Debug.h"
#  include "rhi/dx11/DX11ShadowMap.h"
#  include "rhi/dx11/DX11Skybox.h"
#  include <d3d11.h>

extern DX11ShadowMap*            s_DX11Shadow;
extern DX11Skybox*               s_DX11Sky;
extern ID3D11RasterizerState*    s_ShadowRS;
#  include "rhi/dx11/DX11PointShadow.h"
extern DX11PointShadow*          s_DX11PointShadow;

extern ID3D11Texture2D*          s_HDR_Tex;
extern ID3D11RenderTargetView*   s_HDR_RTV;
extern ID3D11ShaderResourceView* s_HDR_SRV;
extern ID3D11SamplerState*       s_TmSampler;

// Shared scene-sized depth target, used by GBufferPass and HDR pass so they
// don't have to fight over DX11Context::GetDSV() (which is backbuffer-sized).
extern ID3D11Texture2D*          s_Scene_DepthTex;
extern ID3D11DepthStencilView*   s_Scene_DSV;
extern int                       s_Scene_DSV_W, s_Scene_DSV_H;
void EnsureSceneDepth(int w, int h);

extern ID3D11Texture2D*          s_GBuf_PosTex;
extern ID3D11RenderTargetView*   s_GBuf_PosRTV;
extern ID3D11ShaderResourceView* s_GBuf_PosSRV;
extern ID3D11Texture2D*          s_GBuf_NorTex;
extern ID3D11RenderTargetView*   s_GBuf_NorRTV;
extern ID3D11ShaderResourceView* s_GBuf_NorSRV;
extern ID3D11Texture2D*          s_SSAO_Tex;
extern ID3D11RenderTargetView*   s_SSAO_RTV;
extern ID3D11ShaderResourceView* s_SSAO_SRV;
extern ID3D11Texture2D*          s_SSAOBlur_Tex;
extern ID3D11RenderTargetView*   s_SSAOBlur_RTV;
extern ID3D11ShaderResourceView* s_SSAOBlur_SRV;
extern ID3D11Texture2D*          s_NoiseTex;
extern ID3D11ShaderResourceView* s_NoiseSRV;

extern std::shared_ptr<Shader>   s_GPassShader;
extern std::shared_ptr<Shader>   s_SSAOShader;
extern std::shared_ptr<Shader>   s_SSAOBlurShader;
extern ID3D11SamplerState*       s_PointClampSampler;
extern ID3D11SamplerState*       s_PointWrapSampler;
extern std::vector<glm::vec3>    s_SSAOKernel;
extern int                       s_GBuf_W, s_GBuf_H;

extern ID3D11Texture2D*          s_BloomA_Tex;
extern ID3D11RenderTargetView*   s_BloomA_RTV;
extern ID3D11ShaderResourceView* s_BloomA_SRV;
extern ID3D11Texture2D*          s_BloomB_Tex;
extern ID3D11RenderTargetView*   s_BloomB_RTV;
extern ID3D11ShaderResourceView* s_BloomB_SRV;
extern int                       s_Bloom_W, s_Bloom_H;

extern ID3D11Texture2D*          s_View_Tex;
extern ID3D11RenderTargetView*   s_View_RTV;
extern ID3D11ShaderResourceView* s_View_SRV;
extern int                       s_View_W, s_View_H;

// IBL — diffuse irradiance cube, specular prefilter cube (with mips), BRDF LUT.
// Precomputed once at Init from the procedural sky and the configured sun dir.
extern ID3D11Texture2D*          s_EnvCube_Tex;
extern ID3D11ShaderResourceView* s_EnvCube_SRV;
extern ID3D11Texture2D*          s_Irradiance_Tex;
extern ID3D11ShaderResourceView* s_Irradiance_SRV;
extern ID3D11Texture2D*          s_Prefilter_Tex;
extern ID3D11ShaderResourceView* s_Prefilter_SRV;
extern ID3D11Texture2D*          s_BrdfLUT_Tex;
extern ID3D11ShaderResourceView* s_BrdfLUT_SRV;
extern std::shared_ptr<Shader>   s_SkyToCubeShader;
extern std::shared_ptr<Shader>   s_IrradianceShader;
extern std::shared_ptr<Shader>   s_PrefilterShader;
extern std::shared_ptr<Shader>   s_BrdfLUTShader;
extern bool                      s_IBL_Ready;

void PrecomputeIBL(const glm::vec3& sunDirection);
#else
#  include "rhi/opengl/GLShader.h"
#  include "renderer/ShadowMap.h"
#  include "renderer/Skybox.h"
#  include <glad/gl.h>
#  include <GLFW/glfw3.h>

extern ShadowMap*    s_ShadowMap;
extern Skybox*       s_Skybox;
extern unsigned int  s_HDR_FBO;
extern unsigned int  s_HDR_Tex;
extern unsigned int  s_HDR_RBO;
extern unsigned int  s_HDR_VAO;

extern unsigned int  s_View_FBO;
extern unsigned int  s_View_Tex;
extern int           s_View_W, s_View_H;
#endif
