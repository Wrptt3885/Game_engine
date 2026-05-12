#pragma once
#include "core/Component.h"
#include <string>
#include <memory>

class LuaScript : public Component {
public:
    LuaScript();
    ~LuaScript();

    void SetPath(const std::string& path);
    const std::string& GetPath()  const { return m_Path; }
    const std::string& GetError() const { return m_Error; }
    void Reload();
    bool IsValid() const { return m_Valid; }

    void Update(float dt) override;
    void OnCollisionEnter(GameObject* other) override;
    void OnCollisionStay (GameObject* other) override;
    void OnCollisionExit (GameObject* other) override;

private:
    std::string m_Path;
    std::string m_Error;
    bool        m_Valid = false;

    struct ScriptState;
    std::unique_ptr<ScriptState> m_State;

    void Load();
    void InjectBindings();
};
