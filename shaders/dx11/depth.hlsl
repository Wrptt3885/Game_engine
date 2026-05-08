cbuffer CBDepth : register(b0)
{
    float4x4 u_Model;
    float4x4 u_LightSpaceMatrix;
};

struct VSIn  { float3 position : POSITION; float3 normal : NORMAL; float2 tc : TEXCOORD; float3 t : TANGENT; float3 b : BITANGENT; };
struct VSOut { float4 pos : SV_POSITION; };

VSOut VS(VSIn input)
{
    VSOut o;
    o.pos = mul(u_LightSpaceMatrix, mul(u_Model, float4(input.position, 1.0)));
    return o;
}

void PS(VSOut input) {}
