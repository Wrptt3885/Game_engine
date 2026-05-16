// IBL: convolve the environment cubemap into a diffuse irradiance cubemap.
// Riemann sum over the hemisphere around the face direction.

cbuffer CBFace : register(b0)
{
    float3 u_FaceForward; float _pad0;
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

float4 PS(VSOut i) : SV_TARGET
{
    float3 N = normalize(u_FaceForward + i.ndc.x * u_FaceRight + i.ndc.y * u_FaceUp);

    float3 up    = (abs(N.y) < 0.999) ? float3(0, 1, 0) : float3(0, 0, 1);
    float3 right = normalize(cross(up, N));
    up           = cross(N, right);

    float3 irradiance = float3(0, 0, 0);
    float  samples    = 0.0;

    const float dPhi   = 0.025;  // 2pi / dPhi steps
    const float dTheta = 0.025;  // pi/2 / dTheta steps

    for (float phi = 0.0; phi < 2.0 * PI; phi += dPhi) {
        for (float theta = 0.0; theta < 0.5 * PI; theta += dTheta) {
            float sinT = sin(theta);
            float cosT = cos(theta);
            float3 tanSample = float3(sinT * cos(phi), sinT * sin(phi), cosT);
            float3 sampleDir = tanSample.x * right + tanSample.y * up + tanSample.z * N;
            irradiance += t_Env.SampleLevel(s_Linear, sampleDir, 0).rgb * cosT * sinT;
            samples    += 1.0;
        }
    }
    irradiance = PI * irradiance / samples;
    return float4(irradiance, 1.0);
}
