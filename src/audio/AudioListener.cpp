#include "audio/AudioListener.h"
#include "audio/AudioManager.h"
#include "core/GameObject.h"
#include "core/Transform.h"
#include <glm/gtx/quaternion.hpp>

void AudioListener::Update(float dt) {
    auto* owner = GetGameObject();
    if (!owner) return;

    const Transform& t = owner->GetTransform();
    glm::vec3 forward = t.rotation * glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up      = t.rotation * glm::vec3(0.0f, 1.0f,  0.0f);
    AudioManager::SetListenerPosition(t.position, forward, up);
}
