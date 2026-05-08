// SSAO blur pass — 5x5 box blur to smooth raw SSAO noise

struct VSOut {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD;
};

VSOut VS(uint id : SV_VertexID)
{
    float2 uv = float2((id << 1) & 2, id & 2);
    VSOut o;
    o.uv  = uv;
    o.pos = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 0.0, 1.0);
    return o;
}

Texture2D    t_SSAO  : register(t0);
SamplerState s_Point : register(s0);

float4 PS(VSOut i) : SV_TARGET
{
    uint w, h;
    t_SSAO.GetDimensions(w, h);
    float2 texel = 1.0 / float2(w, h);

    float result = 0.0;
    [unroll]
    for (int x = -2; x <= 2; x++) {
        [unroll]
        for (int y = -2; y <= 2; y++) {
            result += t_SSAO.SampleLevel(s_Point, i.uv + float2(x, y) * texel, 0).r;
        }
    }
    float ao = result / 25.0;
    return float4(ao, ao, ao, 1.0);
}
