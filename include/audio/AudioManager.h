#pragma once
#include <glm/glm.hpp>

struct ma_engine;

class AudioManager {
public:
    static void Init();
    static void Shutdown();

    // Called every frame — updates listener transform for 3D audio
    static void SetListenerPosition(const glm::vec3& pos, const glm::vec3& forward, const glm::vec3& up);

    static ma_engine* GetEngine();
    static bool IsReady();
};
