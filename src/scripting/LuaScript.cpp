#ifdef LUA_SCRIPTING
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
extern sol::state& GetLuaState();
#endif

#include "scripting/LuaScript.h"
#include "scripting/LuaManager.h"
#include "core/GameObject.h"
#include "core/Transform.h"
#include <glm/gtc/quaternion.hpp>
#include <iostream>

// Defined with or without LUA_SCRIPTING so unique_ptr destructor works
struct LuaScript::ScriptState {
#ifdef LUA_SCRIPTING
    sol::environment env;
    explicit ScriptState(sol::state& lua) : env(lua, sol::create, lua.globals()) {}
#endif
};

LuaScript::LuaScript()  = default;
LuaScript::~LuaScript() = default;

void LuaScript::SetPath(const std::string& path) {
    m_Path  = path;
    m_Valid = false;
    m_Error.clear();
    m_State.reset();
    if (!path.empty()) Load();
}

void LuaScript::Reload() {
    if (m_Path.empty()) return;
    m_Valid = false;
    m_Error.clear();
    m_State.reset();
    Load();
}

void LuaScript::Load() {
#ifdef LUA_SCRIPTING
    if (!GetGameObject()) { m_Error = "No owning GameObject"; return; }
    auto& lua = GetLuaState();
    m_State = std::make_unique<ScriptState>(lua);
    InjectBindings();

    auto result = lua.script_file(m_Path, m_State->env, sol::script_pass_on_error);
    if (!result.valid()) {
        sol::error err = result;
        m_Error = err.what();
        std::cerr << "[LuaScript] " << m_Path << ": " << m_Error << "\n";
        return;
    }
    m_Valid = true;
    m_Error.clear();

    sol::protected_function awake = m_State->env["Awake"];
    if (awake.valid()) {
        auto r = awake();
        if (!r.valid()) {
            sol::error err = r;
            std::cerr << "[LuaScript] Awake(): " << err.what() << "\n";
        }
    }
#else
    m_Error = "Lua not compiled (LUA_SCRIPTING not defined)";
#endif
}

void LuaScript::InjectBindings() {
#ifdef LUA_SCRIPTING
    auto& lua       = GetLuaState();
    auto& env       = m_State->env;
    GameObject* go  = GetGameObject();

    // Transform table — bound to this script's owner
    sol::table t = lua.create_table();
    t.set_function("GetPosition", [go]() -> std::tuple<float,float,float> {
        auto& p = go->GetTransform().position;
        return {p.x, p.y, p.z};
    });
    t.set_function("SetPosition", [go](float x, float y, float z) {
        go->GetTransform().position = {x, y, z};
    });
    t.set_function("GetEulerAngles", [go]() -> std::tuple<float,float,float> {
        glm::vec3 e = glm::degrees(glm::eulerAngles(go->GetTransform().rotation));
        return {e.x, e.y, e.z};
    });
    t.set_function("SetEulerAngles", [go](float x, float y, float z) {
        go->GetTransform().rotation = glm::quat(glm::radians(glm::vec3{x, y, z}));
    });
    t.set_function("GetScale", [go]() -> std::tuple<float,float,float> {
        auto& s = go->GetTransform().scale;
        return {s.x, s.y, s.z};
    });
    t.set_function("SetScale", [go](float x, float y, float z) {
        go->GetTransform().scale = {x, y, z};
    });
    t.set_function("GetForward", [go]() -> std::tuple<float,float,float> {
        glm::vec3 f = go->GetTransform().rotation * glm::vec3(0.0f, 0.0f, -1.0f);
        return {f.x, f.y, f.z};
    });
    t.set_function("GetRight", [go]() -> std::tuple<float,float,float> {
        glm::vec3 r = go->GetTransform().rotation * glm::vec3(1.0f, 0.0f, 0.0f);
        return {r.x, r.y, r.z};
    });
    env["Transform"] = t;

    // GameObject table — basic info about this script's owner
    sol::table goTbl = lua.create_table();
    goTbl.set_function("GetName",   [go]() -> std::string { return go->GetName(); });
    goTbl.set_function("SetActive", [go](bool a) { go->SetActive(a); });
    goTbl.set_function("IsActive",  [go]() -> bool { return go->IsActive(); });
    env["GameObject"] = goTbl;
#endif
}

// Helper — call a Lua function by name, log errors, stop on Update errors
template<typename... Args>
static void CallProtected(
#ifdef LUA_SCRIPTING
    sol::environment& env,
#endif
    const char* fn, bool& valid, std::string& errOut, Args&&... args)
{
#ifdef LUA_SCRIPTING
    sol::protected_function f = env[fn];
    if (!f.valid()) return;
    auto r = f(std::forward<Args>(args)...);
    if (!r.valid()) {
        sol::error err = r;
        errOut = err.what();
        std::cerr << "[LuaScript] " << fn << "(): " << errOut << "\n";
        valid = false;
    }
#endif
}

void LuaScript::Update(float dt) {
    if (!m_Valid || !m_State || !LuaManager::IsPlaying()) return;
#ifdef LUA_SCRIPTING
    CallProtected(m_State->env, "Update", m_Valid, m_Error, dt);
#endif
}

void LuaScript::OnCollisionEnter(GameObject* other) {
    if (!m_Valid || !m_State || !other) return;
#ifdef LUA_SCRIPTING
    CallProtected(m_State->env, "OnCollisionEnter", m_Valid, m_Error, other->GetName());
#endif
}

void LuaScript::OnCollisionStay(GameObject* other) {
    if (!m_Valid || !m_State || !other) return;
#ifdef LUA_SCRIPTING
    CallProtected(m_State->env, "OnCollisionStay", m_Valid, m_Error, other->GetName());
#endif
}

void LuaScript::OnCollisionExit(GameObject* other) {
    if (!m_Valid || !m_State || !other) return;
#ifdef LUA_SCRIPTING
    CallProtected(m_State->env, "OnCollisionExit", m_Valid, m_Error, other->GetName());
#endif
}
