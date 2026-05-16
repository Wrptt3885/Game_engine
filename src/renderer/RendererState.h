// Implementation-only header: shared static state across Renderer modules.
// Include ONLY in src/renderer/Renderer*.cpp — never in user-facing headers.
#pragma once

#include "renderer/Renderer.h"
#include "renderer/Shader.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

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
#endif
