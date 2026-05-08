#pragma once

#include "renderer/Texture.h"
#include <glm/glm.hpp>
#include <memory>

struct Material {
    glm::vec3              albedo    = glm::vec3(1.0f);
    float                  roughness = 0.5f;
    float                  metallic  = 0.0f;
    std::shared_ptr<Texture> texture;
    std::shared_ptr<Texture> normalMap;
    bool  worldSpaceUV = false;
    float worldUVTile  = 0.1f;

    static Material Default() { return Material{}; }
    static Material FromColor(glm::vec3 color) {
        Material m;
        m.albedo = color;
        return m;
    }
    static Material FromTexture(std::shared_ptr<Texture> tex) {
        Material m;
        m.texture = tex;
        return m;
    }
};
