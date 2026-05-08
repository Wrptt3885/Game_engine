#version 330 core
in  vec2 v_TexCoord;
out vec4 FragColor;

uniform sampler2D u_HDRBuffer;
uniform float     u_Exposure;

// ACES filmic tone mapping approximation (Hill 2014)
vec3 ACES(vec3 x) {
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x*(a*x+b)) / (x*(c*x+d)+e), 0.0, 1.0);
}

void main() {
    vec3 hdr    = texture(u_HDRBuffer, v_TexCoord).rgb * u_Exposure;
    vec3 mapped = ACES(hdr);
    mapped      = pow(mapped, vec3(1.0 / 2.2)); // gamma 2.2
    FragColor   = vec4(mapped, 1.0);
}
