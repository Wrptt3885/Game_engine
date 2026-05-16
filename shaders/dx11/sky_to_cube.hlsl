// IBL: render the procedural sky into one cubemap face.
// Driver issues 6 draws — one per face — with the face basis in CBFace.

cbuffer CBFace : register(b0)
{
    float3 u_FaceForward; float _pad0;
    float3 u_FaceRight;   float _pad1;
    float3 u_FaceUp;      float _pad2;
    float3 u_SunDirection; float _pad3;
};

struct VSOut
{
    float4 pos : SV_POSITION;
    float2 ndc : NDC;
};

VSOut VS(uint id : SV_VertexID)
{
    VSOut o;
    o.ndc  = float2((id == 1) ? 3.0 : -1.0, (id == 2) ? -3.0 : 1.0);
    o.pos  = float4(o.ndc, 0.0, 1.0);
    return o;
}

float3 SampleSky(float3 dir, float3 sunDir)
{
    float  t       = saturate(dir.y * 0.5 + 0.5);
    float3 zenith  = float3(0.08, 0.18, 0.48);
    float3 horizon = float3(0.55, 0.72, 0.90);
    float3 sky     = lerp(horizon, zenith, pow(t, 0.6));

    float sun = pow(saturate(dot(dir, sunDir)), 256.0);
    sky += sun * float3(1.8, 1.5, 0.9);

    float haze = pow(1.0 - abs(dir.y), 4.0) * 0.3;
    sky = lerp(sky, float3(0.9, 0.85, 0.75), haze);
    return sky;
}

float4 PS(VSOut i) : SV_TARGET
{
    float3 dir = normalize(u_FaceForward + i.ndc.x * u_FaceRight + i.ndc.y * u_FaceUp);
    float3 sky = SampleSky(dir, normalize(-u_SunDirection));
    return float4(sky, 1.0);
}
