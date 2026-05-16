#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

struct JointTrack {
    int jointIndex = -1;

    std::vector<float>     translationTimes;
    std::vector<glm::vec3> translations;

    std::vector<float>     rotationTimes;
    std::vector<glm::quat> rotations;

    std::vector<float>     scaleTimes;
    std::vector<glm::vec3> scales;
};

struct AnimationClip {
    std::string             name;
    float                   duration = 0.0f;
    std::vector<JointTrack> tracks;
};
