// G-Buffer pre-pass: outputs view-space position and normal (MRT)

cbuffer CBPerFrame : register(b0)
{
    float4x4 u_Model;
    float4x4 u_View;
    float4x4 u_Projection;
    float4x4 u_LightSpaceMatrix;
};

struct VSIn
{
    float3 position  : POSITION;
    float3 normal    : NORMAL;
    float2 texCoord  : TEXCOORD;
    float3 tangent   : TANGENT;
    float3 bitangent : BITANGENT;
};

struct VSOut
{
    float4 position   : SV_POSITION;
    float3 fragPos_vs : TEXCOORD0;
    float3 normal_vs  : TEXCOORD1;
};

VSOut VS(VSIn input)
{
    VSOut output;
    float4 worldPos   = mul(u_Model, float4(input.position, 1.0));
    float4 viewPos    = mul(u_View, worldPos);
    output.position   = mul(u_Projection, viewPos);
    output.fragPos_vs = viewPos.xyz;

    float3x3 normalMat = (float3x3)mul(u_View, u_Model);
    output.normal_vs   = normalize(mul(normalMat, input.normal));
    return output;
}

struct PSOut
{
    float4 position : SV_TARGET0;  // view-space position
    float4 normal   : SV_TARGET1;  // view-space normal
};

PSOut PS(VSOut input)
{
    PSOut o;
    o.position = float4(input.fragPos_vs, 1.0);
    o.normal   = float4(normalize(input.normal_vs), 0.0);
    return o;
}
