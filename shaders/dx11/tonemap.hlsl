// Tone mapping post-process — fullscreen triangle from SV_VertexID
// Composites HDR scene + bloom before ACES tone map

struct VSOut {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD;
};

// VS generates a fullscreen triangle without a vertex buffer
VSOut VS(uint id : SV_VertexID) {
    // id=0 -> uv(0,0)  ndc(-1, 1)  [top-left]
    // id=1 -> uv(2,0)  ndc( 3, 1)  [top-right, clipped]
    // id=2 -> uv(0,2)  ndc(-1,-3)  [bottom-left, clipped]
    float2 uv = float2((id << 1) & 2, id & 2);
    VSOut o;
    o.uv  = uv;
    // DX11: Y is up in NDC, V=0 is top in UV — flip Y
    o.pos = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 0.0, 1.0);
    return o;
}

// ---- PS -----------------------------------------------------------------

Texture2D    t_HDRBuffer : register(t0);
Texture2D    t_Bloom     : register(t1);
SamplerState s_Linear    : register(s0);

cbuffer ExposureCB : register(b0) {
    float u_Exposure;
    float u_BloomIntensity;
    float2 _pad;
};

float3 ACES(float3 x) {
    float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return saturate((x*(a*x+b)) / (x*(c*x+d)+e));
}

float4 PS(VSOut i) : SV_TARGET {
    float3 hdr   = t_HDRBuffer.Sample(s_Linear, i.uv).rgb;
    float3 bloom = t_Bloom.Sample(s_Linear, i.uv).rgb;
    float3 combined = hdr + bloom * u_BloomIntensity;
    float3 mapped   = ACES(combined * u_Exposure);
    mapped          = pow(abs(mapped), 1.0 / 2.2);
    return float4(mapped, 1.0);
}
