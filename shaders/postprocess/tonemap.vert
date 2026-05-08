#version 330 core
out vec2 v_TexCoord;

// Fullscreen triangle from vertex ID — no vertex buffer needed
void main() {
    const vec2 pos[3] = vec2[](vec2(-1.0,-1.0), vec2(3.0,-1.0), vec2(-1.0, 3.0));
    const vec2 uv [3] = vec2[](vec2( 0.0, 0.0), vec2(2.0, 0.0), vec2( 0.0, 2.0));
    gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);
    v_TexCoord  = uv[gl_VertexID];
}
