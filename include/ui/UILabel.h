#pragma once

#include "ui/UIElement.h"
#include <glm/glm.hpp>
#include <string>

class UILabel : public UIElement {
public:
    std::string text     = "Label";
    glm::vec4   color    = {1.0f, 1.0f, 1.0f, 1.0f};
    float       fontSize = 16.0f;
    bool        centered = false;

    void Draw() override;
    const char* GetTypeName() const override { return "UILabel"; }
};
