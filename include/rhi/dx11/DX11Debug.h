#pragma once
#ifdef USE_DX11_BACKEND

#include <d3d11.h>
#include <iostream>

// ---- DX_CHECK ---------------------------------------------------------------
// Wrap any HRESULT-returning DX11 call to log file+line on failure.
// Usage: DX_CHECK(dev->CreateTexture2D(&td, nullptr, &tex), "HDR texture");

#define DX_CHECK(hr, msg) \
    ::DX11_CheckResult((hr), (msg), __FILE__, __LINE__)

inline void DX11_CheckResult(HRESULT hr, const char* msg,
                              const char* file, int line) {
    if (FAILED(hr))
        std::cerr << "[DX11] FAILED " << msg
                  << "  hr=0x" << std::hex << static_cast<unsigned>(hr)
                  << "  (" << file << ":" << std::dec << line << ")\n";
}

// ---- GpuMarker --------------------------------------------------------------
// RAII scope that pushes a named event to RenderDoc / PIX.
// When no capture tool is attached the calls are no-ops (< 1 µs overhead).
// Usage: GPU_MARKER(ctx, "Shadow Pass");  // active until end of scope

// Forward-declare the interface manually so it works on MinGW where
// d3d11_1.h may not be present or may not expose this type.
// The guard matches the Windows SDK convention — skipped if already defined.
#ifndef __ID3DUserDefinedAnnotation_INTERFACE_DEFINED__
#define __ID3DUserDefinedAnnotation_INTERFACE_DEFINED__
struct ID3DUserDefinedAnnotation : public IUnknown {
    virtual INT  STDMETHODCALLTYPE BeginEvent(LPCWSTR Name) = 0;
    virtual INT  STDMETHODCALLTYPE EndEvent()               = 0;
    virtual void STDMETHODCALLTYPE SetMarker(LPCWSTR Name)  = 0;
    virtual BOOL STDMETHODCALLTYPE GetStatus()              = 0;
};
#endif

// {B2DEB449-547B-47F3-81C7-AE5E6C041F84}
static constexpr GUID kIID_DX11UserAnnotation =
    {0xb2deb449, 0x547b, 0x47f3, {0x81, 0xc7, 0xae, 0x5e, 0x6c, 0x04, 0x1f, 0x84}};

struct GpuMarker {
    ID3DUserDefinedAnnotation* m_ann = nullptr;

    GpuMarker(ID3D11DeviceContext* ctx, const wchar_t* name) {
        if (!ctx) return;
        void* ptr = nullptr;
        if (SUCCEEDED(ctx->QueryInterface(kIID_DX11UserAnnotation, &ptr))) {
            m_ann = static_cast<ID3DUserDefinedAnnotation*>(ptr);
            m_ann->BeginEvent(name);
        }
    }
    ~GpuMarker() {
        if (m_ann) { m_ann->EndEvent(); m_ann->Release(); }
    }
    GpuMarker(const GpuMarker&)            = delete;
    GpuMarker& operator=(const GpuMarker&) = delete;
};

// Two-level paste so __LINE__ expands before concatenation
#define GPU_MARKER_CAT_(a, b)  a##b
#define GPU_MARKER_CAT(a, b)   GPU_MARKER_CAT_(a, b)
#define GPU_MARKER(ctx, name) \
    GpuMarker GPU_MARKER_CAT(_gm_, __LINE__)((ctx), L##name)

#endif // USE_DX11_BACKEND
