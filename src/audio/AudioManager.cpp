#include "audio/AudioManager.h"
#include "miniaudio.h"
#include <iostream>

static ma_engine s_Engine;
static bool      s_Ready = false;

void AudioManager::Init() {
    ma_result result = ma_engine_init(nullptr, &s_Engine);
    if (result != MA_SUCCESS) {
        std::cerr << "[Audio] Failed to initialize miniaudio engine: " << result << "\n";
        return;
    }
    s_Ready = true;
    std::cout << "[Audio] Initialized\n";
}

void AudioManager::Shutdown() {
    if (!s_Ready) return;
    ma_engine_uninit(&s_Engine);
    s_Ready = false;
    std::cout << "[Audio] Shutdown\n";
}

void AudioManager::SetListenerPosition(const glm::vec3& pos, const glm::vec3& forward, const glm::vec3& up) {
    if (!s_Ready) return;
    ma_engine_listener_set_position(&s_Engine, 0, pos.x, pos.y, pos.z);
    ma_engine_listener_set_direction(&s_Engine, 0, forward.x, forward.y, forward.z);
    ma_engine_listener_set_world_up(&s_Engine, 0, up.x, up.y, up.z);
}

ma_engine* AudioManager::GetEngine() {
    return s_Ready ? &s_Engine : nullptr;
}

bool AudioManager::IsReady() {
    return s_Ready;
}
