#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 u_View;
uniform mat4 u_Projection;

out vec3 v_Dir;

void main() {
    v_Dir = aPos;
    // Remove translation from view matrix so skybox stays centered on camera
    vec4 pos = u_Projection * mat4(mat3(u_View)) * vec4(aPos, 1.0);
    // Set z = w so depth = 1.0 (max) — skybox always behind everything
    gl_Position = pos.xyww;
}
