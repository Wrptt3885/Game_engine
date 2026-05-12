#pragma once

#include <glm/glm.hpp>
#include <string>

class UIElement {
public:
    std::string name    = "Element";
    glm::vec2   pos     = {0.0f, 0.0f};   // screen pixels from top-left
    bool        visible = true;

    virtual ~UIElement() = default;
    virtual void Draw() = 0;
    virtual const char* GetTypeName() const = 0;
};
