#include "audio/AudioSource.h"
#include "audio/AudioManager.h"
#include "core/GameObject.h"
#include "core/Transform.h"
#include "miniaudio.h"
#include <iostream>

AudioSource::~AudioSource() {
    UnloadSound();
}

void AudioSource::LoadSound() {
    if (!AudioManager::IsReady() || clipPath.empty()) return;
    if (clipPath == m_LoadedPath && m_Sound) return;  // already loaded

    UnloadSound();

    m_Sound = new ma_sound();
    uint32_t flags = MA_SOUND_FLAG_ASYNC;
    if (!spatial) flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;

    ma_result res = ma_sound_init_from_file(
        AudioManager::GetEngine(), clipPath.c_str(), flags, nullptr, nullptr, m_Sound);

    if (res != MA_SUCCESS) {
        std::cerr << "[Audio] Failed to load: " << clipPath << " (" << res << ")\n";
        delete m_Sound;
        m_Sound = nullptr;
        return;
    }
    m_LoadedPath = clipPath;
    ApplyProperties();
}

void AudioSource::UnloadSound() {
    if (!m_Sound) return;
    ma_sound_uninit(m_Sound);
    delete m_Sound;
    m_Sound     = nullptr;
    m_LoadedPath.clear();
}

void AudioSource::ApplyProperties() {
    if (!m_Sound) return;
    ma_sound_set_volume(m_Sound, volume);
    ma_sound_set_pitch(m_Sound, pitch);
    ma_sound_set_looping(m_Sound, loop ? MA_TRUE : MA_FALSE);
    ma_sound_set_spatialization_enabled(m_Sound, spatial ? MA_TRUE : MA_FALSE);
    if (spatial) {
        ma_sound_set_min_distance(m_Sound, minDistance);
        ma_sound_set_max_distance(m_Sound, maxDistance);
        // rolloff: linear
        ma_sound_set_rolloff(m_Sound, 1.0f);
    }
}

void AudioSource::Play() {
    LoadSound();
    if (!m_Sound) return;
    ApplyProperties();
    ma_sound_seek_to_pcm_frame(m_Sound, 0);
    ma_sound_start(m_Sound);
}

void AudioSource::Stop() {
    if (!m_Sound) return;
    ma_sound_stop(m_Sound);
    ma_sound_seek_to_pcm_frame(m_Sound, 0);
}

void AudioSource::Pause() {
    if (!m_Sound) return;
    ma_sound_stop(m_Sound);
}

bool AudioSource::IsPlaying() const {
    if (!m_Sound) return false;
    return ma_sound_is_playing(m_Sound) == MA_TRUE;
}

void AudioSource::OnPlayStart() {
    LoadSound();
    if (playOnStart) Play();
}

void AudioSource::Update(float dt) {
    if (!m_Sound || !spatial) return;

    // Sync sound position with GameObject transform every frame
    auto* owner = GetGameObject();
    if (!owner) return;
    const glm::vec3& pos = owner->GetTransform().position;
    ma_sound_set_position(m_Sound, pos.x, pos.y, pos.z);
}
