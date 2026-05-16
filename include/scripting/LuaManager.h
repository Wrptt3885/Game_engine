#pragma once
#include <string>
#include <functional>

class Scene;

class LuaManager {
public:
    static void Init();
    static void Shutdown();

    static void SetScene(Scene* scene);
    static void SetTime(float dt);        // accumulates total; call every frame
    static void SetPlayMode(bool playing); // resets total time when false
    static bool IsPlaying();

    // Scene transition callbacks — set by Application after Init()
    static void SetLoadSceneCallback(std::function<void(const std::string&)> cb);
    static void SetPushSceneCallback(std::function<void(const std::string&)> cb);
    static void SetPopSceneCallback(std::function<void()> cb);
};
