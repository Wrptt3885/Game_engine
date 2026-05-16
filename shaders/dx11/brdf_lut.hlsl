// IBL: 2D BRDF integration lookup (split-sum scale + bias).
// Output: RG16F, x = scale, y = bias. Sample at (NdotV, roughness).

struct VSOut
{
    float4 pos : SV_POSITION;
    float2 uv  : UV;
};

VSOut VS(uint id : SV_VertexID)
{
    VSOut o;
    float2 ndc = float2((id == 1) ? 3.0 : -1.0, (id == 2) ? -3.0 : 1.0);
    o.pos = float4(ndc, 0.0, 1.0);
    o.uv  = float2(ndc.x * 0.5 + 0.5, 1.0 - (ndc.y * 0.5 + 0.5));
    return o;
}

static const float PI = 3.14159265359;

float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}
float2 Hammersley(uint i, uint N) { return float2(float(i) / float(N), RadicalInverse_VdC(i)); }

float3 ImportanceSampleGGX(float2 xi, float3 N, float roughness) {
    float a = roughness * roughness;
    float phi      = 2.0 * PI * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    float3 H = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

    float3 up    = (abs(N.z) < 0.999) ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 t = normalize(cross(up, N));
    float3 b = cross(N, t);
    return normalize(t * H.x + b * H.y + N * H.z);
}

float GeometrySchlickIBL(float NdotV, float roughness) {
    float a = roughness;
    float k = (a * a) * 0.5;  // IBL geometry uses k=a^2/2 (vs direct: k=(a+1)^2/8)
    return NdotV / (NdotV * (1.0 - k) + k);
}
float GeometrySmithIBL(float NdotV, float NdotL, float roughness) {
    return GeometrySchlickIBL(NdotV, roughness) * GeometrySchlickIBL(NdotL, roughness);
}

float2 IntegrateBRDF(float NdotV, float roughness)
{
    float3 V = float3(sqrt(1.0 - NdotV * NdotV), 0.0, NdotV);
    float A = 0.0, B = 0.0;
    float3 N = float3(0, 0, 1);

    const uint SAMPLE_COUNT = 1024u;
    for (uint i = 0u; i < SAMPLE_COUNT; i++) {
        float2 xi = Hammersley(i, SAMPLE_COUNT);
        float3 H  = ImportanceSampleGGX(xi, N, roughness);
        float3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = saturate(L.z);
        float NdotH = saturate(H.z);
        float VdotH = saturate(dot(V, H));

        if (NdotL > 0.0) {
            float G  = GeometrySmithIBL(NdotV, NdotL, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);
            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    return float2(A, B) / float(SAMPLE_COUNT);
}

float4 PS(VSOut i) : SV_TARGET
{
    return float4(IntegrateBRDF(i.uv.x, i.uv.y), 0.0, 1.0);
}
