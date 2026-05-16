#pragma once
#include "core/Component.h"
#include <string>

struct ma_sound;

class AudioSource : public Component {
public:
    std::string clipPath;
    float volume      = 1.0f;
    float pitch       = 1.0f;
    bool  loop        = false;
    bool  playOnStart = false;
    bool  spatial     = true;   // 3D positional attenuation
    float minDistance = 1.0f;
    float maxDistance = 50.0f;

    ~AudioSource();

    void Play();
    void Stop();
    void Pause();
    bool IsPlaying() const;

    // Component hooks
    void Update(float dt) override;

    // Called by ApplicationPlayMode on StartPlay
    void OnPlayStart();

private:
    void LoadSound();
    void UnloadSound();
    void ApplyProperties();

    ma_sound*   m_Sound       = nullptr;
    std::string m_LoadedPath;
};
