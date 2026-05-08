// Separable 9-tap Gaussian blur for bloom (used for both H and V passes)

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

Texture2D    t_Src    : register(t0);
SamplerState s_Linear : register(s0);

cbuffer BlurCB : register(b0) {
    float u_TexelW;  // 1.0 / texture_width
    float u_TexelH;  // 1.0 / texture_height
    float u_DirX;    // 1.0 for horizontal, 0.0 for vertical
    float u_DirY;    // 0.0 for horizontal, 1.0 for vertical
};

// Gaussian weights for 9-tap kernel (sigma ~1.5); sum = 1.0
static const float WEIGHTS[5] = { 0.227027, 0.194595, 0.121622, 0.054054, 0.016216 };

float4 PS(VSOut i) : SV_TARGET {
    // Scale by 2.0 so blur radius spans 8 half-res pixels (=16 full-res pixels)
    float2 step = float2(u_TexelW * u_DirX, u_TexelH * u_DirY) * 2.0;

    float3 result = t_Src.Sample(s_Linear, i.uv).rgb * WEIGHTS[0];
    [unroll]
    for (int j = 1; j < 5; j++) {
        result += t_Src.Sample(s_Linear, i.uv + step * j).rgb * WEIGHTS[j];
        result += t_Src.Sample(s_Linear, i.uv - step * j).rgb * WEIGHTS[j];
    }
    return float4(result, 1.0);
}
