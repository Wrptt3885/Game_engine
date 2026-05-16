#version 330 core
layout(location = 0) in vec3  aPos;
layout(location = 5) in uvec4 aBoneIds;
layout(location = 6) in vec4  aBoneWeights;

uniform mat4 u_LightSpaceMatrix;
uniform mat4 u_Model;
uniform bool u_UseSkinning;
uniform mat4 u_BoneMatrices[100];

void main() {
    vec4 localPos = vec4(aPos, 1.0);
    if (u_UseSkinning) {
        mat4 skinMat = aBoneWeights.x * u_BoneMatrices[aBoneIds.x]
                     + aBoneWeights.y * u_BoneMatrices[aBoneIds.y]
                     + aBoneWeights.z * u_BoneMatrices[aBoneIds.z]
                     + aBoneWeights.w * u_BoneMatrices[aBoneIds.w];
        localPos = skinMat * localPos;
    }
    gl_Position = u_LightSpaceMatrix * u_Model * localPos;
}
