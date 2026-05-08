#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat4 u_LightSpaceMatrix;

out vec3 v_Normal;
out vec3 v_FragPos;
out vec2 v_TexCoord;
out vec4 v_FragPosLightSpace;
out mat3 v_TBN;

void main() {
    vec4 worldPos = u_Model * vec4(aPos, 1.0);
    v_FragPos     = worldPos.xyz;
    v_TexCoord    = aTexCoord;
    v_FragPosLightSpace = u_LightSpaceMatrix * worldPos;

    mat3 normalMatrix = mat3(transpose(inverse(u_Model)));
    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T = normalize(normalMatrix * aTangent);
    T = normalize(T - dot(T, N) * N); // re-orthogonalize
    vec3 B = cross(N, T);

    v_Normal = N;
    v_TBN    = mat3(T, B, N);

    gl_Position = u_Projection * u_View * worldPos;
}
