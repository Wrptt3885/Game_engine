cbuffer CBSkybox : register(b0)
{
    float4x4 u_View;
    float4x4 u_Projection;
    float3   u_SunDirection;
    float    _pad;
};

struct VSIn  { float3 pos : POSITION; };
struct VSOut { float4 svPos : SV_POSITION; float3 localPos : LOCALPOS; };

VSOut VS(VSIn input)
{
    VSOut o;
    o.localPos = input.pos;

    // w=0 means translation column of the view matrix has no effect
    float4 viewPos = mul(u_View, float4(input.pos, 0.0));
    float4 clipPos = mul(u_Projection, float4(viewPos.xyz, 1.0));

    o.svPos = clipPos.xyww; // force depth = 1.0 (far plane)
    return o;
}

float4 PS(VSOut input) : SV_TARGET
{
    float3 dir    = normalize(input.localPos);
    float3 sunDir = normalize(-u_SunDirection);

    float  t      = saturate(dir.y * 0.5 + 0.5);
    float3 zenith = float3(0.08, 0.18, 0.48);
    float3 horizon= float3(0.55, 0.72, 0.90);
    float3 sky    = lerp(horizon, zenith, pow(t, 0.6));

    float sun = pow(saturate(dot(dir, sunDir)), 256.0);
    sky += sun * float3(1.8, 1.5, 0.9);

    float haze = pow(1.0 - abs(dir.y), 4.0) * 0.3;
    sky = lerp(sky, float3(0.9, 0.85, 0.75), haze);

    return float4(sky, 1.0);
}
