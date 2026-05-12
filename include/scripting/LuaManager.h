#pragma once
#include <string>

class Scene;

class LuaManager {
public:
    static void Init();
    static void Shutdown();

    static void SetScene(Scene* scene);
    static void SetTime(float dt);        // accumulates total; call every frame
    static void SetPlayMode(bool playing); // resets total time when false
    static bool IsPlaying();
};
