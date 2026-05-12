#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>

class Texture;

class UICanvas {
public:
    static void BeginFrame(float screenW, float screenH);

    static void DrawText(const std::string& text, glm::vec2 pos,
                         glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f},
                         float fontSize = 16.0f, bool centered = false);

    static void DrawRect(glm::vec2 pos, glm::vec2 size,
                         glm::vec4 color = {0.0f, 0.0f, 0.0f, 0.5f});

    static void DrawImage(std::shared_ptr<Texture> texture, glm::vec2 pos,
                          glm::vec2 size,
                          glm::vec4 tint = {1.0f, 1.0f, 1.0f, 1.0f});

    static float GetScreenW() { return s_W; }
    static float GetScreenH() { return s_H; }

private:
    static float s_W;
    static float s_H;
};
