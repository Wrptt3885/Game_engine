#include "platform/Input.h"

GLFWwindow* Input::s_Window      = nullptr;
bool        Input::s_Keys[512]   = {false};
bool        Input::s_KeysPressed[512] = {false};
glm::vec2   Input::s_MousePos    = glm::vec2(0.0f);
float       Input::s_ScrollDelta = 0.0f;

void Input::Init(GLFWwindow* window) {
    s_Window = window;

    glfwSetKeyCallback(window, [](GLFWwindow*, int key, int, int action, int) {
        if (key >= 0 && key < 512) {
            if (action == GLFW_PRESS)        { s_Keys[key] = true;  s_KeysPressed[key] = true; }
            else if (action == GLFW_RELEASE) { s_Keys[key] = false; s_KeysPressed[key] = false; }
        }
    });

    glfwSetScrollCallback(window, [](GLFWwindow*, double, double yOffset) {
        s_ScrollDelta += (float)yOffset;
    });
}

void Input::Update() {
    if (s_Window) {
        double x, y;
        glfwGetCursorPos(s_Window, &x, &y);
        s_MousePos = glm::vec2((float)x, (float)y);
    }
    for (int i = 0; i < 512; i++)
        s_KeysPressed[i] = false;
}

bool Input::IsKeyPressed(int key)  { return key >= 0 && key < 512 && s_Keys[key]; }
bool Input::IsKeyReleased(int key) { return key >= 0 && key < 512 && !s_Keys[key]; }
bool Input::IsKeyDown(int key)     { return key >= 0 && key < 512 && s_KeysPressed[key]; }

bool Input::IsMouseButtonPressed(int button) {
    return glfwGetMouseButton(s_Window, button) == GLFW_PRESS;
}

glm::vec2 Input::GetMousePosition() { return s_MousePos; }

float Input::ConsumeScrollDelta() {
    float d = s_ScrollDelta;
    s_ScrollDelta = 0.0f;
    return d;
}

glm::vec3 Input::GetMovementInput() {
    glm::vec3 movement(0.0f);
    if (IsKeyPressed(GLFW_KEY_W) || IsKeyPressed(GLFW_KEY_UP))    movement.z += 1.0f;
    if (IsKeyPressed(GLFW_KEY_S) || IsKeyPressed(GLFW_KEY_DOWN))  movement.z -= 1.0f;
    if (IsKeyPressed(GLFW_KEY_A) || IsKeyPressed(GLFW_KEY_LEFT))  movement.x -= 1.0f;
    if (IsKeyPressed(GLFW_KEY_D) || IsKeyPressed(GLFW_KEY_RIGHT)) movement.x += 1.0f;
    return movement;
}
