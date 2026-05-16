#version 330 core
layout(location = 0) in vec3  aPos;
layout(location = 1) in vec3  aNormal;
layout(location = 2) in vec2  aTexCoord;
layout(location = 3) in vec3  aTangent;
layout(location = 4) in vec3  aBitangent;
layout(location = 5) in uvec4 aBoneIds;
layout(location = 6) in vec4  aBoneWeights;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat4 u_LightSpaceMatrix;

uniform bool u_UseSkinning;
uniform mat4 u_BoneMatrices[100];

out vec3 v_Normal;
out vec3 v_FragPos;
out vec2 v_TexCoord;
out vec4 v_FragPosLightSpace;
out mat3 v_TBN;

void main() {
    vec4 localPos    = vec4(aPos, 1.0);
    vec3 localNormal  = aNormal;
    vec3 localTangent = aTangent;

    if (u_UseSkinning) {
        mat4 skinMat = aBoneWeights.x * u_BoneMatrices[aBoneIds.x]
                     + aBoneWeights.y * u_BoneMatrices[aBoneIds.y]
                     + aBoneWeights.z * u_BoneMatrices[aBoneIds.z]
                     + aBoneWeights.w * u_BoneMatrices[aBoneIds.w];
        mat3 skinRot  = mat3(skinMat);
        localPos      = skinMat * localPos;
        localNormal   = skinRot * localNormal;
        localTangent  = skinRot * localTangent;
    }

    vec4 worldPos = u_Model * localPos;
    v_FragPos     = worldPos.xyz;
    v_TexCoord    = aTexCoord;
    v_FragPosLightSpace = u_LightSpaceMatrix * worldPos;

    mat3 normalMatrix = mat3(transpose(inverse(u_Model)));
    vec3 N = normalize(normalMatrix * localNormal);
    vec3 T = normalize(normalMatrix * localTangent);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    v_Normal = N;
    v_TBN    = mat3(T, B, N);

    gl_Position = u_Projection * u_View * worldPos;
}
