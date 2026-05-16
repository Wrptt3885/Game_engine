#version 330 core

#define MAX_DIR_LIGHTS   4
#define MAX_POINT_LIGHTS 8
#define PI 3.14159265359

in vec3 v_Normal;
in vec3 v_FragPos;
in vec2 v_TexCoord;
in vec4 v_FragPosLightSpace;
in mat3 v_TBN;

uniform sampler2D u_Texture;
uniform bool      u_UseTexture;
uniform sampler2D u_NormalMap;
uniform bool      u_UseNormalMap;
uniform vec3      u_Color;
uniform bool      u_UseWorldUV;
uniform float     u_WorldUVTile;
uniform vec3      u_ViewPos;
uniform sampler2DShadow u_ShadowMap;
uniform float     u_Roughness;
uniform float     u_Metallic;

struct DirLight {
    vec3  direction;
    vec3  color;
    float intensity;
};

struct PointLight {
    vec3  position;
    vec3  color;
    float intensity;
    float radius;
};

uniform DirLight   u_DirLights[MAX_DIR_LIGHTS];
uniform int        u_DirLightCount;
uniform PointLight u_PointLights[MAX_POINT_LIGHTS];
uniform int        u_PointLightCount;

out vec4 FragColor;

const vec2 poissonDisk[16] = vec2[](
    vec2(-0.94201624, -0.39906216), vec2( 0.94558609, -0.76890725),
    vec2(-0.09418410, -0.92938870), vec2( 0.34495938,  0.29387760),
    vec2(-0.91588581,  0.45771432), vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543,  0.27676845), vec2( 0.97484398,  0.75648379),
    vec2( 0.44323325, -0.97511554), vec2( 0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023), vec2( 0.79197514,  0.19090188),
    vec2(-0.24188840,  0.99706507), vec2(-0.81409955,  0.91437590),
    vec2( 0.19984126,  0.78641367), vec2( 0.14383161, -0.14100790)
);

float ShadowFactor(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    vec3 proj = fragPosLightSpace.xyz / fragPosLightSpace.w;
    proj = proj * 0.5 + 0.5;
    if (proj.z > 1.0) return 0.0;
    float bias  = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    float spread = 2.5;
    vec2  texel  = spread / textureSize(u_ShadowMap, 0);
    float shadow = 0.0;
    for (int i = 0; i < 16; i++) {
        shadow += texture(u_ShadowMap, vec3(proj.xy + poissonDisk[i] * texel, proj.z - bias));
    }
    return shadow / 16.0;
}

// --- Cook-Torrance BRDF helpers -------------------------------------------

// Trowbridge-Reitz GGX normal distribution
float DistributionGGX(float NdotH, float a2) {
    float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

// Smith's Schlick-GGX geometry (k = (r+1)^2/8 for direct lighting)
float GeometrySchlick(float NdotX, float k) {
    return NdotX / (NdotX * (1.0 - k) + k);
}

float GeometrySmith(float NdotV, float NdotL, float roughness) {
    float k = (roughness + 1.0) * (roughness + 1.0) * 0.125;
    return GeometrySchlick(NdotV, k) * GeometrySchlick(NdotL, k);
}

// Fresnel-Schlick
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Fresnel-Schlick with roughness bias (for ambient)
vec3 FresnelRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Full Cook-Torrance specular + Lambert diffuse for one light
vec3 PBR(vec3 N, vec3 V, vec3 L, vec3 radiance,
         vec3 albedo, float roughness, float metallic, vec3 F0) {
    vec3  H     = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    float a2 = roughness * roughness * roughness * roughness; // remap r -> r^4 for perceptual linearity
    float D  = DistributionGGX(NdotH, a2);
    float G  = GeometrySmith(NdotV, NdotL, roughness);
    vec3  F  = FresnelSchlick(HdotV, F0);

    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    vec3 specular = D * G * F / max(4.0 * NdotV * NdotL, 0.0001);

    return (kD * albedo / PI + specular) * radiance * NdotL;
}

// --------------------------------------------------------------------------

void main() {
    vec2 texCoord = u_UseWorldUV ? v_FragPos.xz * u_WorldUVTile : v_TexCoord;

    vec3 albedo = u_UseTexture
        ? texture(u_Texture, texCoord).rgb * u_Color
        : u_Color;

    vec3 N;
    if (u_UseNormalMap) {
        N = texture(u_NormalMap, texCoord).rgb * 2.0 - 1.0;
        N = normalize(v_TBN * N);
    } else {
        N = normalize(v_Normal);
    }
    vec3 V = normalize(u_ViewPos - v_FragPos);

    float roughness = max(u_Roughness, 0.04);
    float metallic  = clamp(u_Metallic, 0.0, 1.0);
    vec3  F0 = mix(vec3(0.04), albedo, metallic);

    float shadow = 0.0;
    if (u_DirLightCount > 0) {
        vec3 L0 = normalize(-u_DirLights[0].direction);
        shadow = ShadowFactor(v_FragPosLightSpace, N, L0);
    }

    vec3 Lo = vec3(0.0);

    for (int i = 0; i < u_DirLightCount; i++) {
        vec3 L        = normalize(-u_DirLights[i].direction);
        float s       = (i == 0) ? shadow : 0.0;
        vec3 radiance = u_DirLights[i].color * u_DirLights[i].intensity;
        Lo += (1.0 - s) * PBR(N, V, L, radiance, albedo, roughness, metallic, F0);
    }

    for (int i = 0; i < u_PointLightCount; i++) {
        vec3  toLight = u_PointLights[i].position - v_FragPos;
        float dist    = length(toLight);
        float atten   = clamp(1.0 - dist / u_PointLights[i].radius, 0.0, 1.0);
        atten *= atten;
        vec3 L        = normalize(toLight);
        vec3 radiance = u_PointLights[i].color * u_PointLights[i].intensity * atten;
        Lo += PBR(N, V, L, radiance, albedo, roughness, metallic, F0);
    }

    // Hemisphere ambient — sky/ground gradient with Fresnel-based specular
    vec3 skyColor    = vec3(0.20, 0.32, 0.58);
    vec3 groundColor = vec3(0.10, 0.08, 0.05);
    float hemi    = N.y * 0.5 + 0.5;
    vec3 envColor = mix(groundColor, skyColor, hemi);

    vec3 kS = FresnelRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    float shadowedAmbient = 1.0 - shadow * 0.4;
    vec3 ambient = (kD * envColor * albedo
                 + kS * envColor * (1.0 - roughness * roughness) * 0.5) * shadowedAmbient;

    FragColor = vec4(ambient + Lo, 1.0);
}
