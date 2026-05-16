cbuffer CBDepth : register(b0)
{
    float4x4 u_Model;
    float4x4 u_LightSpaceMatrix;
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
struct VSOut { float4 pos : SV_POSITION; };

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
    VSOut o;
    o.pos = mul(u_LightSpaceMatrix, mul(u_Model, localPos));
    return o;
}

void PS(VSOut input) {}
