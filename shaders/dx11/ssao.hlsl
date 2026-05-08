// SSAO compute pass — fullscreen triangle, reads G-buffer position + normal

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

// ---- PS -----------------------------------------------------------------

Texture2D    t_Position : register(t0);  // view-space position (RGBA32F)
Texture2D    t_Normal   : register(t1);  // view-space normal   (RGBA16F)
Texture2D    t_Noise    : register(t2);  // 4x4 random rotation (RGBA32F)
SamplerState s_Clamp    : register(s0);  // point-clamp for G-buffer
SamplerState s_Wrap     : register(s1);  // point-wrap  for noise tiling

cbuffer SSAOParams : register(b0)
{
    float4x4 u_Projection;
    float4   u_Kernel[32];
    float    u_NoiseScaleX;
    float    u_NoiseScaleY;
    float    u_Radius;
    float    u_Bias;
};

static const int NUM_SAMPLES = 32;

float4 PS(VSOut i) : SV_TARGET
{
    float3 fragPos = t_Position.SampleLevel(s_Clamp, i.uv, 0).xyz;
    // Background pixels have z=0 — no occlusion there
    if (fragPos.z == 0.0) return float4(1.0, 1.0, 1.0, 1.0);

    float3 normal = normalize(t_Normal.SampleLevel(s_Clamp, i.uv, 0).xyz);
    float2 rvec2  = t_Noise.SampleLevel(s_Wrap, i.uv * float2(u_NoiseScaleX, u_NoiseScaleY), 0).xy;
    float3 rvec   = normalize(float3(rvec2, 0.0));

    // TBN from noise-rotated tangent
    float3 tangent   = normalize(rvec - normal * dot(rvec, normal));
    float3 bitangent = cross(normal, tangent);
    float3x3 TBN     = float3x3(tangent, bitangent, normal);

    float occlusion = 0.0;
    [loop]
    for (int k = 0; k < NUM_SAMPLES; k++)
    {
        // Sample position in view space
        float3 sampleVS = mul(u_Kernel[k].xyz, TBN);
        sampleVS = fragPos + sampleVS * u_Radius;

        // Project to screen UV — DX11: NDC y=1 is top → UV y=0
        float4 offset = mul(u_Projection, float4(sampleVS, 1.0));
        offset.xyz   /= offset.w;
        float2 sampleUV = float2(offset.x * 0.5 + 0.5, 0.5 - offset.y * 0.5);

        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 ||
            sampleUV.y < 0.0 || sampleUV.y > 1.0) continue;

        float sampleDepth = t_Position.SampleLevel(s_Clamp, sampleUV, 0).z;
        if (sampleDepth == 0.0) continue;

        // In right-handed view space: less-negative Z = closer to camera
        // Occluded when actual geometry (sampleDepth) is closer than our sample
        float rangeCheck = smoothstep(0.0, 1.0, u_Radius / (abs(fragPos.z - sampleDepth) + 0.0001));
        occlusion += (sampleDepth >= sampleVS.z + u_Bias ? 1.0 : 0.0) * rangeCheck;
    }

    float ao = 1.0 - (occlusion / float(NUM_SAMPLES));
    return float4(ao, ao, ao, 1.0);
}
