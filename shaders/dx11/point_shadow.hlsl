// Point light shadow depth pass — renders linear distance / farPlane into R16_FLOAT cubemap face

cbuffer CBPointDepth : register(b0)  // VS
{
    float4x4 u_Model;
    float4x4 u_ViewProj;
};

cbuffer CBPointLight : register(b0)  // PS
{
    float3 u_LightPos;
    float  u_FarPlane;
};

cbuffer CBSkinning : register(b2)
{
    int      u_UseSkinning;
    float3   _skinPad;
    float4x4 u_BoneMatrices[100];
};

struct VSIn
{
    float3 position    : POSITION;
    float3 normal      : NORMAL;
    float2 tc          : TEXCOORD;
    float3 t           : TANGENT;
    float3 b           : BITANGENT;
    uint4  boneIds     : BLENDINDICES;
    float4 boneWeights : BLENDWEIGHT;
};

struct VSOut
{
    float4 pos      : SV_POSITION;
    float3 worldPos : WORLDPOS;
};

VSOut VS(VSIn input)
{
    float4 localPos = float4(input.position, 1.0);
    if (u_UseSkinning) {
        float4x4 skinMat = input.boneWeights.x * u_BoneMatrices[input.boneIds.x]
                         + input.boneWeights.y * u_BoneMatrices[input.boneIds.y]
                         + input.boneWeights.z * u_BoneMatrices[input.boneIds.z]
                         + input.boneWeights.w * u_BoneMatrices[input.boneIds.w];
        localPos = mul(skinMat, localPos);
    }
    float4 world = mul(u_Model, localPos);
    VSOut o;
    o.pos      = mul(u_ViewProj, world);
    o.worldPos = world.xyz;
    return o;
}

float4 PS(VSOut i) : SV_TARGET
{
    float dist = length(i.worldPos - u_LightPos) / u_FarPlane;
    return float4(dist, dist, dist, 1.0);
}
