#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Input {
public:
    static void Init(GLFWwindow* window);
    static void Update();

    static bool IsKeyPressed(int key);
    static bool IsKeyReleased(int key);
    static bool IsKeyDown(int key);

    static bool IsMouseButtonPressed(int button);
    static glm::vec2 GetMousePosition();
    static float ConsumeScrollDelta(); // returns scroll and resets to 0

    static glm::vec3 GetMovementInput();

private:
    static GLFWwindow* s_Window;
    static bool s_Keys[512];
    static bool s_KeysPressed[512];
    static glm::vec2 s_MousePos;
    static float s_ScrollDelta;
};
