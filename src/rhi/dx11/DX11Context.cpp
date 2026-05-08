#include "rhi/dx11/DX11Context.h"
#ifdef USE_DX11_BACKEND

#include "rhi/dx11/DX11Debug.h"
#include <dxgi.h>
#include <iostream>

ID3D11Device*            DX11Context::s_Device            = nullptr;
ID3D11DeviceContext*     DX11Context::s_Context           = nullptr;
IDXGISwapChain*          DX11Context::s_SwapChain         = nullptr;
ID3D11RenderTargetView*  DX11Context::s_RTV               = nullptr;
ID3D11DepthStencilView*  DX11Context::s_DSV               = nullptr;
ID3D11Texture2D*         DX11Context::s_DepthStencilTex   = nullptr;
D3D11_VIEWPORT           DX11Context::s_Viewport          = {};
ID3D11RasterizerState*   DX11Context::s_RasterizerState   = nullptr;
ID3D11DepthStencilState* DX11Context::s_DepthStencilState = nullptr;
ID3D11SamplerState*      DX11Context::s_SamplerState      = nullptr;

bool DX11Context::Init(void* hwnd, int width, int height) {
    DXGI_SWAP_CHAIN_DESC sd               = {};
    sd.BufferCount                        = 1;
    sd.BufferDesc.Width                   = (UINT)width;
    sd.BufferDesc.Height                  = (UINT)height;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 0;
    sd.BufferDesc.RefreshRate.Denominator = 0;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = (HWND)hwnd;
    sd.SampleDesc.Count                   = 1;
    sd.SampleDesc.Quality                 = 0;
    sd.Windowed                           = TRUE;
    sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

    UINT deviceFlags = 0;
#ifdef DX11_DEBUG_LAYER
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, deviceFlags,
        nullptr, 0, D3D11_SDK_VERSION,
        &sd, &s_SwapChain, &s_Device, &featureLevel, &s_Context);

    if (FAILED(hr)) {
        std::cerr << "[DX11] D3D11CreateDeviceAndSwapChain failed: 0x" << std::hex << hr << "\n";
        return false;
    }

#ifdef DX11_DEBUG_LAYER
    // Make validation errors break loudly instead of silently continuing
    ID3D11InfoQueue* infoQueue = nullptr;
    if (SUCCEEDED(s_Device->QueryInterface(__uuidof(ID3D11InfoQueue),
                                           reinterpret_cast<void**>(&infoQueue)))) {
        infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR,      TRUE);
        infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING,    TRUE);
        infoQueue->Release();
    }
    std::cout << "[DX11] Debug layer active — validation errors will print to stderr\n";
#endif

    CreateRenderTargetAndDepth(width, height);

    D3D11_RASTERIZER_DESC rd  = {};
    rd.FillMode               = D3D11_FILL_SOLID;
    rd.CullMode               = D3D11_CULL_BACK;
    rd.FrontCounterClockwise  = TRUE;  // match OpenGL CCW convention
    rd.DepthClipEnable        = TRUE;
    DX_CHECK(s_Device->CreateRasterizerState(&rd, &s_RasterizerState), "main RasterizerState");
    s_Context->RSSetState(s_RasterizerState);

    D3D11_DEPTH_STENCIL_DESC dsd = {};
    dsd.DepthEnable              = TRUE;
    dsd.DepthWriteMask           = D3D11_DEPTH_WRITE_MASK_ALL;
    dsd.DepthFunc                = D3D11_COMPARISON_LESS;
    DX_CHECK(s_Device->CreateDepthStencilState(&dsd, &s_DepthStencilState), "main DepthStencilState");
    s_Context->OMSetDepthStencilState(s_DepthStencilState, 0);

    D3D11_SAMPLER_DESC smpd = {};
    smpd.Filter             = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    smpd.AddressU           = D3D11_TEXTURE_ADDRESS_WRAP;
    smpd.AddressV           = D3D11_TEXTURE_ADDRESS_WRAP;
    smpd.AddressW           = D3D11_TEXTURE_ADDRESS_WRAP;
    smpd.MaxLOD             = D3D11_FLOAT32_MAX;
    smpd.ComparisonFunc     = D3D11_COMPARISON_NEVER;
    DX_CHECK(s_Device->CreateSamplerState(&smpd, &s_SamplerState), "main SamplerState (WRAP)");
    s_Context->PSSetSamplers(0, 1, &s_SamplerState);

    std::cout << "[DX11] Initialized. Feature level: 0x" << std::hex << featureLevel << "\n";
    return true;
}

void DX11Context::CreateRenderTargetAndDepth(int w, int h) {
    ID3D11Texture2D* backBuffer = nullptr;
    DX_CHECK(s_SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)), "SwapChain GetBuffer");
    DX_CHECK(s_Device->CreateRenderTargetView(backBuffer, nullptr, &s_RTV), "backbuffer RTV");
    if (backBuffer) backBuffer->Release();

    D3D11_TEXTURE2D_DESC dtd   = {};
    dtd.Width                  = (UINT)w;
    dtd.Height                 = (UINT)h;
    dtd.MipLevels              = 1;
    dtd.ArraySize              = 1;
    dtd.Format                 = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dtd.SampleDesc.Count       = 1;
    dtd.Usage                  = D3D11_USAGE_DEFAULT;
    dtd.BindFlags              = D3D11_BIND_DEPTH_STENCIL;
    DX_CHECK(s_Device->CreateTexture2D(&dtd, nullptr, &s_DepthStencilTex), "depth-stencil texture");
    DX_CHECK(s_Device->CreateDepthStencilView(s_DepthStencilTex, nullptr, &s_DSV), "DSV");

    s_Context->OMSetRenderTargets(1, &s_RTV, s_DSV);

    s_Viewport = { 0.f, 0.f, (float)w, (float)h, 0.f, 1.f };
    s_Context->RSSetViewports(1, &s_Viewport);
}

void DX11Context::ReleaseRenderTargetAndDepth() {
    if (s_RTV)             { s_RTV->Release();             s_RTV             = nullptr; }
    if (s_DSV)             { s_DSV->Release();             s_DSV             = nullptr; }
    if (s_DepthStencilTex) { s_DepthStencilTex->Release(); s_DepthStencilTex = nullptr; }
}

void DX11Context::ResizeBuffers(int width, int height) {
    s_Context->OMSetRenderTargets(0, nullptr, nullptr);
    ReleaseRenderTargetAndDepth();
    s_SwapChain->ResizeBuffers(0, (UINT)width, (UINT)height, DXGI_FORMAT_UNKNOWN, 0);
    CreateRenderTargetAndDepth(width, height);
}

void DX11Context::Present(bool vsync) {
    s_SwapChain->Present(vsync ? 1 : 0, 0);
}

void DX11Context::Shutdown() {
    if (s_SamplerState)      { s_SamplerState->Release();      s_SamplerState      = nullptr; }
    if (s_DepthStencilState) { s_DepthStencilState->Release(); s_DepthStencilState = nullptr; }
    if (s_RasterizerState)   { s_RasterizerState->Release();   s_RasterizerState   = nullptr; }
    ReleaseRenderTargetAndDepth();
    if (s_SwapChain) { s_SwapChain->Release(); s_SwapChain = nullptr; }
    if (s_Context)   { s_Context->Release();   s_Context   = nullptr; }
    if (s_Device)    { s_Device->Release();    s_Device    = nullptr; }
}

#endif // USE_DX11_BACKEND
