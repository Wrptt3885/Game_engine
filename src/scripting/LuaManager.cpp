#ifdef LUA_SCRIPTING
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

static sol::state s_Lua;
sol::state& GetLuaState() { return s_Lua; }
#endif

#include "scripting/LuaManager.h"
#include "core/Scene.h"
#include "core/GameObject.h"
#include "platform/Input.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <unordered_map>

static Scene* s_Scene     = nullptr;
static bool   s_Playing   = false;
static float  s_TotalTime = 0.0f;

static const std::unordered_map<std::string, int>& KeyMap() {
    static const std::unordered_map<std::string, int> km = {
        {"A",GLFW_KEY_A},{"B",GLFW_KEY_B},{"C",GLFW_KEY_C},{"D",GLFW_KEY_D},
        {"E",GLFW_KEY_E},{"F",GLFW_KEY_F},{"G",GLFW_KEY_G},{"H",GLFW_KEY_H},
        {"I",GLFW_KEY_I},{"J",GLFW_KEY_J},{"K",GLFW_KEY_K},{"L",GLFW_KEY_L},
        {"M",GLFW_KEY_M},{"N",GLFW_KEY_N},{"O",GLFW_KEY_O},{"P",GLFW_KEY_P},
        {"Q",GLFW_KEY_Q},{"R",GLFW_KEY_R},{"S",GLFW_KEY_S},{"T",GLFW_KEY_T},
        {"U",GLFW_KEY_U},{"V",GLFW_KEY_V},{"W",GLFW_KEY_W},{"X",GLFW_KEY_X},
        {"Y",GLFW_KEY_Y},{"Z",GLFW_KEY_Z},
        {"0",GLFW_KEY_0},{"1",GLFW_KEY_1},{"2",GLFW_KEY_2},{"3",GLFW_KEY_3},
        {"4",GLFW_KEY_4},{"5",GLFW_KEY_5},{"6",GLFW_KEY_6},{"7",GLFW_KEY_7},
        {"8",GLFW_KEY_8},{"9",GLFW_KEY_9},
        {"Space",GLFW_KEY_SPACE},{"Enter",GLFW_KEY_ENTER},{"Escape",GLFW_KEY_ESCAPE},
        {"Tab",GLFW_KEY_TAB},{"Backspace",GLFW_KEY_BACKSPACE},{"Delete",GLFW_KEY_DELETE},
        {"Left",GLFW_KEY_LEFT},{"Right",GLFW_KEY_RIGHT},{"Up",GLFW_KEY_UP},{"Down",GLFW_KEY_DOWN},
        {"LShift",GLFW_KEY_LEFT_SHIFT},{"RShift",GLFW_KEY_RIGHT_SHIFT},
        {"LCtrl",GLFW_KEY_LEFT_CONTROL},{"RCtrl",GLFW_KEY_RIGHT_CONTROL},
        {"LAlt",GLFW_KEY_LEFT_ALT},{"RAlt",GLFW_KEY_RIGHT_ALT},
        {"F1",GLFW_KEY_F1},{"F2",GLFW_KEY_F2},{"F3",GLFW_KEY_F3},{"F4",GLFW_KEY_F4},
        {"F5",GLFW_KEY_F5},{"F6",GLFW_KEY_F6},{"F7",GLFW_KEY_F7},{"F8",GLFW_KEY_F8},
        {"F9",GLFW_KEY_F9},{"F10",GLFW_KEY_F10},{"F11",GLFW_KEY_F11},{"F12",GLFW_KEY_F12},
    };
    return km;
}

void LuaManager::Init() {
#ifdef LUA_SCRIPTING
    s_Lua.open_libraries(
        sol::lib::base, sol::lib::math, sol::lib::table,
        sol::lib::string, sol::lib::io
    );

    s_Lua.set_function("Log", [](const std::string& msg) {
        std::cout << "[Lua] " << msg << "\n";
    });

    // Time
    s_Lua["Time"] = s_Lua.create_table_with(
        "dt", 0.0f, "t", 0.0f, "playMode", false
    );

    sol::table input = s_Lua.create_table();
    input.set_function("IsKeyPressed", [](const std::string& k) -> bool {
        auto it = KeyMap().find(k);
        return it != KeyMap().end() && Input::IsKeyPressed(it->second);
    });
    input.set_function("IsKeyDown", [](const std::string& k) -> bool {
        auto it = KeyMap().find(k);
        return it != KeyMap().end() && Input::IsKeyDown(it->second);
    });
    input.set_function("IsMouseButtonPressed", [](int btn) -> bool {
        return Input::IsMouseButtonPressed(btn);
    });
    input.set_function("GetMousePosition", []() -> std::tuple<float, float> {
        glm::vec2 p = Input::GetMousePosition();
        return {p.x, p.y};
    });
    s_Lua["Input"] = input;

    // Scene.Find(name) -> table with methods
    sol::table scene_tbl = s_Lua.create_table();
    scene_tbl.set_function("Find", [](const std::string& name) -> sol::object {
        if (!s_Scene) return sol::nil;
        auto go = s_Scene->FindGameObject(name);
        if (!go) return sol::nil;
        sol::table t = GetLuaState().create_table();
        t.set_function("GetName",   [go]() -> std::string { return go->GetName(); });
        t.set_function("SetActive", [go](bool a) { go->SetActive(a); });
        t.set_function("IsActive",  [go]() -> bool { return go->IsActive(); });
        t.set_function("GetPosition", [go]() -> std::tuple<float,float,float> {
            auto& p = go->GetTransform().position;
            return {p.x, p.y, p.z};
        });
        t.set_function("SetPosition", [go](float x, float y, float z) {
            go->GetTransform().position = {x, y, z};
        });
        return t;
    });
    s_Lua["Scene"] = scene_tbl;

    // math.lerp, math.clamp extensions
    sol::table m = s_Lua["math"];
    m.set_function("lerp",  [](float a, float b, float t) { return a + (b - a) * t; });
    m.set_function("clamp", [](float v, float lo, float hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    });
#endif
}

void LuaManager::Shutdown() {
#ifdef LUA_SCRIPTING
    s_Lua = sol::state{};
#endif
}

void LuaManager::SetScene(Scene* scene) { s_Scene = scene; }

void LuaManager::SetTime(float dt) {
    if (s_Playing) s_TotalTime += dt;
#ifdef LUA_SCRIPTING
    s_Lua["Time"]["dt"] = dt;
    s_Lua["Time"]["t"]  = s_TotalTime;
#endif
}

void LuaManager::SetPlayMode(bool playing) {
    s_Playing = playing;
    if (!playing) s_TotalTime = 0.0f;
#ifdef LUA_SCRIPTING
    s_Lua["Time"]["playMode"] = playing;
#endif
}

bool LuaManager::IsPlaying() { return s_Playing; }
