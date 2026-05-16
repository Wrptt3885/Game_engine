// ---- Constant Buffers -------------------------------------------------------

cbuffer CBPerFrame : register(b0)
{
    float4x4 u_Model;
    float4x4 u_View;
    float4x4 u_Projection;
    float4x4 u_LightSpaceMatrix;
};

struct DirLight
{
    float3 direction;
    float  _pad0;
    float3 color;
    float  intensity;
};

struct PointLight
{
    float3 position;
    float  _pad0;
    float3 color;
    float  intensity;
    float  radius;
    float3 _pad1;
};

cbuffer CBLighting : register(b0)
{
    float3   u_ViewPos;
    float    _pad0;
    int      u_DirLightCount;
    int      u_PointLightCount;
    int      u_UseTexture;
    int      u_UseNormalMap;
    int      u_UseWorldUV;
    float    u_WorldUVTile;
    float    u_Roughness;
    float    u_Metallic;
    float3   u_Color;
    float    _pad2;
    DirLight   u_DirLights[4];
    PointLight u_PointLights[8];
    float3   u_PointShadowPos;
    float    u_PointShadowFar;
    int      u_HasPointShadow;
    float3   _pad3;
};

// ---- Skinning ---------------------------------------------------------------

cbuffer CBSkinning : register(b2)
{
    int      u_UseSkinning;
    float3   _skinPad;
    float4x4 u_BoneMatrices[100];
};

// ---- Textures / Samplers ----------------------------------------------------

Texture2D    t_Diffuse      : register(t0);
Texture2D    t_NormalMap    : register(t1);
Texture2D    t_ShadowMap    : register(t2);
Texture2D    t_SSAO         : register(t3);
TextureCube  t_PointShadow  : register(t4);
SamplerState           s_Linear  : register(s0);
SamplerComparisonState s_Shadow  : register(s1);

// ---- VS ---------------------------------------------------------------------

struct VSIn
{
    float3 position    : POSITION;
    float3 normal      : NORMAL;
    float2 texCoord    : TEXCOORD;
    float3 tangent     : TANGENT;
    float3 bitangent   : BITANGENT;
    uint4  boneIds     : BLENDINDICES;
    float4 boneWeights : BLENDWEIGHT;
};

struct VSOut
{
    float4 position          : SV_POSITION;
    float3 fragPos           : FRAGPOS;
    float2 texCoord          : TEXCOORD;
    float4 fragPosLightSpace : LIGHTSPACEPOS;
    float3 normal            : NORMAL;
    float3 tangent           : TANGENT;
    float3 bitangent         : BITANGENT;
};

VSOut VS(VSIn input)
{
    VSOut output;

    float4 localPos    = float4(input.position, 1.0);
    float3 localNormal  = input.normal;
    float3 localTangent = input.tangent;

    [branch]
    if (u_UseSkinning) {
        float4x4 skinMat = input.boneWeights.x * u_BoneMatrices[input.boneIds.x]
                         + input.boneWeights.y * u_BoneMatrices[input.boneIds.y]
                         + input.boneWeights.z * u_BoneMatrices[input.boneIds.z]
                         + input.boneWeights.w * u_BoneMatrices[input.boneIds.w];
        float3x3 skinRot  = (float3x3)skinMat;
        localPos     = mul(skinMat,  float4(input.position, 1.0));
        localNormal   = mul(skinRot, input.normal);
        localTangent  = mul(skinRot, input.tangent);
    }

    float4 worldPos          = mul(u_Model, localPos);
    output.fragPos           = worldPos.xyz;
    output.texCoord          = input.texCoord;
    output.fragPosLightSpace = mul(u_LightSpaceMatrix, worldPos);

    float3x3 normalMat = (float3x3)u_Model;

    float3 N = normalize(mul(normalMat, localNormal));
    float3 T = normalize(mul(normalMat, localTangent));
    T = normalize(T - dot(T, N) * N);
    float3 B = cross(N, T);

    output.normal    = N;
    output.tangent   = T;
    output.bitangent = B;

    output.position = mul(u_Projection, mul(u_View, worldPos));
    return output;
}

// ---- Shadow -----------------------------------------------------------------

static const float2 poissonDisk[16] = {
    float2(-0.94201624, -0.39906216), float2( 0.94558609, -0.76890725),
    float2(-0.09418410, -0.92938870), float2( 0.34495938,  0.29387760),
    float2(-0.91588581,  0.45771432), float2(-0.81544232, -0.87912464),
    float2(-0.38277543,  0.27676845), float2( 0.97484398,  0.75648379),
    float2( 0.44323325, -0.97511554), float2( 0.53742981, -0.47373420),
    float2(-0.26496911, -0.41893023), float2( 0.79197514,  0.19090188),
    float2(-0.24188840,  0.99706507), float2(-0.81409955,  0.91437590),
    float2( 0.19984126,  0.78641367), float2( 0.14383161, -0.14100790)
};

float ShadowFactor(float4 fragPosLS, float3 normal, float3 lightDir)
{
    float3 proj = fragPosLS.xyz / fragPosLS.w;
    // Convert NDC [-1,1] to texture [0,1]; DX11 Y is already correct
    proj.xy = proj.xy * 0.5 + 0.5;
    proj.y  = 1.0 - proj.y;
    if (proj.z > 1.0) return 0.0;

    float bias  = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    float spread = 2.5;

    uint w, h;
    t_ShadowMap.GetDimensions(w, h);
    float2 texel = spread / float2(w, h);

    float shadow = 0.0;
    [unroll]
    for (int i = 0; i < 16; i++) {
        shadow += t_ShadowMap.SampleCmpLevelZero(s_Shadow, proj.xy + poissonDisk[i] * texel, proj.z - bias);
    }
    return shadow / 16.0;
}

// ---- Point shadow -----------------------------------------------------------

float PointShadowFactor(float3 fragPos)
{
    if (!u_HasPointShadow) return 0.0;
    float3 L    = fragPos - u_PointShadowPos;
    float  dist = length(L) / u_PointShadowFar;
    float  closestDist = t_PointShadow.Sample(s_Linear, L).r;
    float  bias = 0.005;
    return (dist - bias > closestDist) ? 1.0 : 0.0;
}

// ---- Cook-Torrance BRDF helpers ---------------------------------------------

static const float PI = 3.14159265359;

float DistributionGGX(float NdotH, float a2) {
    float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

float GeometrySchlick(float NdotX, float k) {
    return NdotX / (NdotX * (1.0 - k) + k);
}

float GeometrySmith(float NdotV, float NdotL, float roughness) {
    float k = (roughness + 1.0) * (roughness + 1.0) * 0.125;
    return GeometrySchlick(NdotV, k) * GeometrySchlick(NdotL, k);
}

float3 FresnelSchlick(float cosTheta, float3 F0) {
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

float3 FresnelRoughness(float cosTheta, float3 F0, float roughness) {
    return F0 + (max(1.0 - roughness, F0) - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

float3 PBR(float3 N, float3 V, float3 L, float3 radiance,
           float3 albedo, float roughness, float metallic, float3 F0)
{
    float3 H    = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    float a2 = roughness * roughness * roughness * roughness;
    float D  = DistributionGGX(NdotH, a2);
    float G  = GeometrySmith(NdotV, NdotL, roughness);
    float3 F = FresnelSchlick(HdotV, F0);

    float3 kD      = (1.0 - F) * (1.0 - metallic);
    float3 specular = D * G * F / max(4.0 * NdotV * NdotL, 0.0001);

    return (kD * albedo / PI + specular) * radiance * NdotL;
}

// ---- PS ---------------------------------------------------------------------

float4 PS(VSOut input) : SV_TARGET
{
    float2 texCoord = u_UseWorldUV ? input.fragPos.xz * u_WorldUVTile : input.texCoord;

    float3 albedo = u_UseTexture
        ? t_Diffuse.Sample(s_Linear, texCoord).rgb * u_Color
        : u_Color;

    float3 N;
    if (u_UseNormalMap) {
        float3x3 TBN = float3x3(input.tangent, input.bitangent, input.normal);
        float3 n = t_NormalMap.Sample(s_Linear, texCoord).rgb * 2.0 - 1.0;
        N = normalize(mul(n, TBN));
    } else {
        N = normalize(input.normal);
    }

    float3 V = normalize(u_ViewPos - input.fragPos);

    float roughness = max(u_Roughness, 0.04);
    float metallic  = clamp(u_Metallic, 0.0, 1.0);
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);

    float shadow = 0.0;
    if (u_DirLightCount > 0) {
        float3 L0 = normalize(-u_DirLights[0].direction);
        shadow = ShadowFactor(input.fragPosLightSpace, N, L0);
    }

    float3 Lo = float3(0.0, 0.0, 0.0);

    for (int i = 0; i < u_DirLightCount; i++) {
        float3 L      = normalize(-u_DirLights[i].direction);
        float  s      = (i == 0) ? shadow : 0.0;
        float3 radiance = u_DirLights[i].color * u_DirLights[i].intensity;
        Lo += (1.0 - s) * PBR(N, V, L, radiance, albedo, roughness, metallic, F0);
    }

    for (int j = 0; j < u_PointLightCount; j++) {
        float3 toLight  = u_PointLights[j].position - input.fragPos;
        float  dist     = length(toLight);
        float  atten    = saturate(1.0 - dist / u_PointLights[j].radius);
        atten          *= atten;
        float3 L        = normalize(toLight);
        float3 radiance = u_PointLights[j].color * u_PointLights[j].intensity * atten;
        float  s        = (j == 0) ? PointShadowFactor(input.fragPos) : 0.0;
        Lo += (1.0 - s) * PBR(N, V, L, radiance, albedo, roughness, metallic, F0);
    }

    // Sample SSAO — GetDimensions returns 0 when SRV is null (no SSAO yet)
    uint aoW, aoH;
    t_SSAO.GetDimensions(aoW, aoH);
    float ao = (aoW > 0) ? t_SSAO.Sample(s_Linear, input.position.xy / float2(aoW, aoH)).r : 1.0;

    // Hemisphere ambient with Fresnel-based specular approximation
    float3 skyColor    = float3(0.20, 0.32, 0.58);
    float3 groundColor = float3(0.10, 0.08, 0.05);
    float  hemi     = N.y * 0.5 + 0.5;
    float3 envColor = lerp(groundColor, skyColor, hemi);

    float3 kS = FresnelRoughness(max(dot(N, V), 0.0), F0, roughness);
    float3 kD = (1.0 - kS) * (1.0 - metallic);
    float shadowedAmbient = 1.0 - shadow * 0.4;
    float3 ambient = (kD * envColor * albedo
                   + kS * envColor * (1.0 - roughness * roughness) * 0.5) * ao * shadowedAmbient;

    return float4(ambient + Lo, 1.0);
}
