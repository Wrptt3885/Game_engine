#pragma once
#include "core/Component.h"

// Marks which GameObject acts as the audio listener (the "ear").
// Attach to the camera object or character. Only the first active
// AudioListener in the scene is used each frame.
class AudioListener : public Component {
public:
    void Update(float dt) override;
};
