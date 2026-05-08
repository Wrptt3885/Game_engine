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

struct VSOut
{
    float4 pos      : SV_POSITION;
    float3 worldPos : WORLDPOS;
};

VSOut VS(float3 position : POSITION,
         float3 normal   : NORMAL,
         float2 texCoord : TEXCOORD,
         float3 tangent  : TANGENT,
         float3 bitangent: BITANGENT)
{
    float4 world = mul(u_Model, float4(position, 1.0));
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
