#pragma once
#ifdef USE_DX11_BACKEND

#include <d3d11.h>
#include <dxgi.h>

class DX11Context {
public:
    static bool Init(void* hwnd, int width, int height);
    static void Shutdown();
    static void Present(bool vsync = true);
    static void ResizeBuffers(int width, int height);

    static ID3D11Device*            GetDevice()   { return s_Device;   }
    static ID3D11DeviceContext*     GetContext()  { return s_Context;  }
    static IDXGISwapChain*          GetSwapChain(){ return s_SwapChain;}
    static ID3D11RenderTargetView*  GetRTV()              { return s_RTV;              }
    static ID3D11DepthStencilView*  GetDSV()              { return s_DSV;              }
    static const D3D11_VIEWPORT&    GetViewport()         { return s_Viewport;         }
    static ID3D11RasterizerState*   GetRasterizerState()  { return s_RasterizerState;  }
    static ID3D11DepthStencilState* GetDepthStencilState(){ return s_DepthStencilState;}
    static ID3D11SamplerState*      GetSamplerState()     { return s_SamplerState;     }

private:
    static void CreateRenderTargetAndDepth(int w, int h);
    static void ReleaseRenderTargetAndDepth();

    static ID3D11Device*            s_Device;
    static ID3D11DeviceContext*     s_Context;
    static IDXGISwapChain*          s_SwapChain;
    static ID3D11RenderTargetView*  s_RTV;
    static ID3D11DepthStencilView*  s_DSV;
    static ID3D11Texture2D*         s_DepthStencilTex;
    static D3D11_VIEWPORT           s_Viewport;
    static ID3D11RasterizerState*   s_RasterizerState;
    static ID3D11DepthStencilState* s_DepthStencilState;
    static ID3D11SamplerState*      s_SamplerState;
};

#endif // USE_DX11_BACKEND
