#include "renderer/SkinnedMeshRenderer.h"
#include "renderer/Renderer.h"
#include "core/GameObject.h"
#include "core/Transform.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <cmath>
#include <unordered_map>

// Truncate each track's value array to its matching times array (and vice versa)
// so EvaluatePose can index safely without a per-sample bounds check.
static void NormalizeTracks(std::vector<AnimationClip>& clips) {
    for (auto& c : clips) {
        for (auto& t : c.tracks) {
            size_t nT = std::min(t.translationTimes.size(), t.translations.size());
            t.translationTimes.resize(nT); t.translations.resize(nT);
            size_t nR = std::min(t.rotationTimes.size(),    t.rotations.size());
            t.rotationTimes.resize(nR);    t.rotations.resize(nR);
            size_t nS = std::min(t.scaleTimes.size(),       t.scales.size());
            t.scaleTimes.resize(nS);       t.scales.resize(nS);
        }
    }
}

void SkinnedMeshRenderer::AddClipsFromFile(const std::string& filepath,
                                            std::shared_ptr<Skeleton> srcSkel,
                                            std::vector<AnimationClip> clips)
{
    NormalizeTracks(clips);
    // Build source-joint-index → target-joint-index map using joint names.
    // If skeletons match exactly this is an identity map; handles mis-ordered exports.
    std::unordered_map<int, int> remap;
    if (srcSkel && m_Skeleton) {
        for (int si = 0; si < (int)srcSkel->joints.size(); si++) {
            const std::string& name = srcSkel->joints[si].name;
            for (int ti = 0; ti < (int)m_Skeleton->joints.size(); ti++) {
                if (m_Skeleton->joints[ti].name == name) { remap[si] = ti; break; }
            }
        }
    }

    for (auto& clip : clips) {
        for (auto& track : clip.tracks) {
            auto it = remap.find(track.jointIndex);
            track.jointIndex = (it != remap.end()) ? it->second : -1;
        }
        m_Clips.push_back(std::move(clip));
    }
    m_ExtraClipFiles.push_back(filepath);
}

void SkinnedMeshRenderer::SetSkeleton(std::shared_ptr<Skeleton> skel) {
    m_Skeleton = std::move(skel);
    if (m_Skeleton)
        m_BoneMatrices.assign(m_Skeleton->joints.size(), glm::mat4(1.0f));
}

void SkinnedMeshRenderer::SetClips(std::vector<AnimationClip> clips) {
    NormalizeTracks(clips);
    m_Clips       = std::move(clips);
    m_CurrentClip = 0;
    m_Time        = 0.0f;
}

void SkinnedMeshRenderer::PlayClip(int index) {
    if (index < 0 || index >= (int)m_Clips.size()) return;
    m_CurrentClip = index;
    m_Time        = 0.0f;
}

void SkinnedMeshRenderer::PlayClip(const std::string& name) {
    for (int i = 0; i < (int)m_Clips.size(); i++) {
        if (m_Clips[i].name == name) { PlayClip(i); return; }
    }
}

const char* SkinnedMeshRenderer::AnimRule::TriggerName(Trigger t) {
    switch (t) {
        case Trigger::Idle:      return "Standing still";
        case Trigger::Moving:    return "Moving (WASD)";
        case Trigger::Sprinting: return "Sprinting (Shift+WASD)";
        case Trigger::Jumping:   return "Jumping (Space)";
    }
    return "";
}

const char* SkinnedMeshRenderer::AnimRule::TriggerDesc(Trigger t) {
    switch (t) {
        case Trigger::Idle:      return "No movement input";
        case Trigger::Moving:    return "WASD pressed";
        case Trigger::Sprinting: return "Shift + WASD";
        case Trigger::Jumping:   return "Space pressed";
    }
    return "";
}

void SkinnedMeshRenderer::EvaluateRules(bool moving, bool sprinting, bool jumping) {
    int fallback = -1;
    for (const auto& rule : m_AnimRules) {
        if (rule.clipIndex < 0 || rule.clipIndex >= (int)m_Clips.size()) continue;
        // Last Idle rule acts as fallback if nothing else matches
        if (rule.trigger == AnimRule::Trigger::Idle) fallback = rule.clipIndex;
        bool match = false;
        switch (rule.trigger) {
            case AnimRule::Trigger::Idle:      match = !moving && !jumping;   break;
            case AnimRule::Trigger::Moving:    match = moving  && !sprinting; break;
            case AnimRule::Trigger::Sprinting: match = moving  &&  sprinting; break;
            case AnimRule::Trigger::Jumping:   match = jumping;               break;
        }
        if (match) {
            if (rule.clipIndex != m_CurrentClip) PlayClip(rule.clipIndex);
            return;
        }
    }
    // No rule matched — play idle fallback if available, else clip 0
    if (fallback >= 0) {
        if (fallback != m_CurrentClip) PlayClip(fallback);
    } else if (!m_Clips.empty() && m_CurrentClip != 0) {
        PlayClip(0);
    }
}

void SkinnedMeshRenderer::Update(float dt) {
    if (!m_Skeleton || m_Clips.empty()) return;

    if (m_Playing && m_CurrentClip >= 0 && m_CurrentClip < (int)m_Clips.size()) {
        const float dur = m_Clips[m_CurrentClip].duration;
        m_Time += dt * m_Speed;
        if (dur > 0.0f) {
            if (m_Loop)
                m_Time = std::fmod(m_Time, dur);
            else
                m_Time = std::min(m_Time, dur);
        }
    }

    EvaluatePose();
}

void SkinnedMeshRenderer::Render(const Camera& cam) {
    if (!m_Mesh) return;
    auto owner = GetGameObject();
    if (!owner) return;

    Renderer::DrawMesh(
        *m_Mesh,
        owner->GetTransform().GetMatrix(),
        m_Material,
        m_BoneMatrices.empty() ? nullptr : m_BoneMatrices.data(),
        (int)m_BoneMatrices.size());
}

void SkinnedMeshRenderer::EvaluatePose() {
    if (!m_Skeleton || m_Skeleton->joints.empty()) return;

    int n = (int)m_Skeleton->joints.size();
    if ((int)m_BoneMatrices.size() != n) m_BoneMatrices.assign(n, glm::mat4(1.0f));
    if ((int)m_PoseScratch.size()  != n) m_PoseScratch.assign(n, glm::mat4(1.0f));

    // Track-by-joint table reused across frames; sized to joint count.
    m_TrackByJoint.assign(n, nullptr);
    if (m_CurrentClip >= 0 && m_CurrentClip < (int)m_Clips.size()) {
        for (const auto& t : m_Clips[m_CurrentClip].tracks) {
            if (t.jointIndex >= 0 && t.jointIndex < n)
                m_TrackByJoint[t.jointIndex] = &t;
        }
    }

    // Traverse joints in order (GLTF guarantees parents come before children)
    for (int i = 0; i < n; i++) {
        const Joint& joint = m_Skeleton->joints[i];

        glm::mat4 local(1.0f);
        if (const JointTrack* track = m_TrackByJoint[i]) {
            glm::vec3 pos   = SampleVec3(track->translationTimes, track->translations, m_Time, glm::vec3(0.0f));
            glm::quat rot   = SampleQuat(track->rotationTimes,    track->rotations,    m_Time);
            glm::vec3 scale = SampleVec3(track->scaleTimes,       track->scales,       m_Time, glm::vec3(1.0f));
            local = glm::translate(glm::mat4(1.0f), pos)
                  * glm::mat4_cast(rot)
                  * glm::scale(glm::mat4(1.0f), scale);
        }

        m_PoseScratch[i] = (joint.parentIndex < 0)
            ? local
            : m_PoseScratch[joint.parentIndex] * local;

        m_BoneMatrices[i] = m_PoseScratch[i] * joint.inverseBindMatrix;
    }
}

glm::vec3 SkinnedMeshRenderer::SampleVec3(
    const std::vector<float>& times, const std::vector<glm::vec3>& vals,
    float t, const glm::vec3& def) const
{
    // sizes are guaranteed equal at clip load (see SetClips/AddClipsFromFile).
    if (times.empty()) return def;
    if (t <= times.front()) return vals.front();
    if (t >= times.back())  return vals.back();
    for (size_t i = 0; i + 1 < times.size(); i++) {
        if (t >= times[i] && t < times[i + 1]) {
            float f = (t - times[i]) / (times[i + 1] - times[i]);
            return glm::mix(vals[i], vals[i + 1], f);
        }
    }
    return vals.back();
}

glm::quat SkinnedMeshRenderer::SampleQuat(
    const std::vector<float>& times, const std::vector<glm::quat>& vals, float t) const
{
    if (times.empty()) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    if (t <= times.front()) return vals.front();
    if (t >= times.back())  return vals.back();
    for (size_t i = 0; i + 1 < times.size(); i++) {
        if (t >= times[i] && t < times[i + 1]) {
            float f = (t - times[i]) / (times[i + 1] - times[i]);
            return glm::slerp(vals[i], vals[i + 1], f);
        }
    }
    return vals.back();
}
