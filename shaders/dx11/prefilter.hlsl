// IBL: prefilter the environment cubemap for specular IBL (split-sum approximation).
// Importance-sample the GGX distribution at the given roughness.

cbuffer CBPrefilter : register(b0)
{
    float3 u_FaceForward; float u_Roughness;
    float3 u_FaceRight;   float _pad1;
    float3 u_FaceUp;      float _pad2;
};

TextureCube  t_Env    : register(t0);
SamplerState s_Linear : register(s0);

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

static const float PI = 3.14159265359;

// Hammersley low-discrepancy sequence
float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

float2 Hammersley(uint i, uint N) {
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

float3 ImportanceSampleGGX(float2 xi, float3 N, float roughness) {
    float a = roughness * roughness;
    float phi      = 2.0 * PI * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    float3 H = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

    float3 up    = (abs(N.z) < 0.999) ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);
    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

float4 PS(VSOut i) : SV_TARGET
{
    float3 N = normalize(u_FaceForward + i.ndc.x * u_FaceRight + i.ndc.y * u_FaceUp);
    float3 V = N;  // split-sum approximation

    const uint SAMPLE_COUNT = 1024u;
    float3 color = float3(0, 0, 0);
    float  total = 0.0;

    for (uint s = 0u; s < SAMPLE_COUNT; s++) {
        float2 xi = Hammersley(s, SAMPLE_COUNT);
        float3 H  = ImportanceSampleGGX(xi, N, u_Roughness);
        float3 L  = normalize(reflect(-V, H));

        float NdotL = saturate(dot(N, L));
        if (NdotL > 0.0) {
            color += t_Env.SampleLevel(s_Linear, L, 0).rgb * NdotL;
            total += NdotL;
        }
    }
    color /= max(total, 0.0001);
    return float4(color, 1.0);
}
