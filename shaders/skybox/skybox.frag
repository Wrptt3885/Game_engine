#version 330 core
in vec3 v_Dir;

uniform vec3 u_SunDirection;

out vec4 FragColor;

void main() {
    vec3 dir = normalize(v_Dir);
    float t  = dir.y; // -1 = ground, 0 = horizon, 1 = zenith

    vec3 zenith  = vec3(0.08, 0.28, 0.70);
    vec3 horizon = vec3(0.58, 0.76, 0.92);
    vec3 ground  = vec3(0.14, 0.11, 0.08);

    vec3 sky;
    if (t > 0.0)
        sky = mix(horizon, zenith, pow(t, 0.45));
    else
        sky = mix(horizon, ground, clamp(-t * 4.0, 0.0, 1.0));

    // Sun disk + glow
    vec3  sunDir  = normalize(-u_SunDirection);
    float sunDot  = max(dot(dir, sunDir), 0.0);
    sky += vec3(1.0, 0.95, 0.7)  * pow(sunDot, 256.0) * 8.0;  // sharp disk
    sky += vec3(1.0, 0.6,  0.2)  * pow(sunDot, 12.0)  * 0.4;  // orange glow
    sky += vec3(0.8, 0.85, 1.0)  * pow(sunDot, 3.0)   * 0.08; // wide scatter

    FragColor = vec4(sky, 1.0);
}
