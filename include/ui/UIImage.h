#pragma once

#include "ui/UIElement.h"
#include "renderer/Texture.h"
#include <glm/glm.hpp>
#include <memory>

class UIImage : public UIElement {
public:
    std::shared_ptr<Texture> texture;
    glm::vec2 size = {100.0f, 100.0f};
    glm::vec4 tint = {1.0f, 1.0f, 1.0f, 1.0f};

    void Draw() override;
    const char* GetTypeName() const override { return "UIImage"; }
};
