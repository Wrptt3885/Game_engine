// Bloom threshold — downsample HDR to half-res, extract bright pixels

struct VSOut {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD;
};

VSOut VS(uint id : SV_VertexID) {
    float2 uv = float2((id << 1) & 2, id & 2);
    VSOut o;
    o.uv  = uv;
    o.pos = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 0.0, 1.0);
    return o;
}

Texture2D    t_HDR    : register(t0);
SamplerState s_Linear : register(s0);

cbuffer BloomCB : register(b0) {
    float u_Threshold;
    float u_Knee;
    float2 _pad;
};

float Luminance(float3 c) {
    return dot(c, float3(0.2126, 0.7152, 0.0722));
}

float4 PS(VSOut i) : SV_TARGET {
    float3 hdr = t_HDR.Sample(s_Linear, i.uv).rgb;
    float  lum = Luminance(hdr);

    // Soft knee: smooth ramp around threshold
    float knee = u_Threshold * u_Knee;
    float rq   = clamp(lum - u_Threshold + knee, 0.0, 2.0 * knee);
    rq         = (rq * rq) / (4.0 * knee + 0.00001);
    float w    = max(rq, lum - u_Threshold) / max(lum, 0.00001);

    return float4(hdr * saturate(w), 1.0);
}
