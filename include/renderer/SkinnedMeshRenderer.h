#pragma once
#include "core/Component.h"
#include "renderer/Mesh.h"
#include "renderer/Material.h"
#include "renderer/Skeleton.h"
#include "renderer/AnimationClip.h"
#include <vector>
#include <memory>
#include <string>

class Camera;

class SkinnedMeshRenderer : public Component {
public:
    void SetMesh(std::shared_ptr<Mesh> mesh)         { m_Mesh = std::move(mesh); }
    void SetMaterial(const Material& mat)             { m_Material = mat; }
    void SetSkeleton(std::shared_ptr<Skeleton> skel);
    void SetClips(std::vector<AnimationClip> clips);

    std::shared_ptr<Mesh>             GetMesh()     const { return m_Mesh; }
    Material&                         GetMaterial()       { return m_Material; }
    const Material&                   GetMaterial() const { return m_Material; }
    std::shared_ptr<Skeleton>         GetSkeleton() const { return m_Skeleton; }
    const std::vector<AnimationClip>& GetClips()    const { return m_Clips; }
    const std::vector<glm::mat4>&     GetBoneMatrices() const { return m_BoneMatrices; }

    void  PlayClip(int index);
    void  PlayClip(const std::string& name);
    void  AddClip(AnimationClip clip) { m_Clips.push_back(std::move(clip)); }
    // Loads clips from an external file; remaps joint indices to match this skeleton by name.
    void  AddClipsFromFile(const std::string& filepath,
                           std::shared_ptr<Skeleton> srcSkel,
                           std::vector<AnimationClip> clips);
    const std::vector<std::string>& GetExtraClipFiles() const { return m_ExtraClipFiles; }
    int   GetCurrentClip()   const { return m_CurrentClip; }
    float GetAnimTime()      const { return m_Time; }
    bool  GetLoop()          const { return m_Loop; }
    float GetSpeed()         const { return m_Speed; }
    bool  IsAnimPlaying()    const { return m_Playing; }
    void  SetLoop(bool l)          { m_Loop = l; }
    void  SetSpeed(float s)        { m_Speed = s; }
    void  SetAnimPlaying(bool p)   { m_Playing = p; }

    // Play-mode animation rules (evaluated top-to-bottom, first match wins)
    struct AnimRule {
        enum class Trigger { Idle, Moving, Sprinting, Jumping };
        int     clipIndex = -1;
        Trigger trigger   = Trigger::Idle;
        static const char* TriggerName(Trigger t);
        static const char* TriggerDesc(Trigger t);
    };

    std::vector<AnimRule>&       GetAnimRules()       { return m_AnimRules; }
    const std::vector<AnimRule>& GetAnimRules() const { return m_AnimRules; }
    void EvaluateRules(bool moving, bool sprinting, bool jumping);

    // Y distance from physics capsule centre to mesh root. Adjust until feet sit on ground.
    float GetPhysicsYOffset() const  { return m_PhysicsYOffset; }
    void  SetPhysicsYOffset(float v) { m_PhysicsYOffset = v; }

    void Update(float dt) override;
    void Render(const Camera& cam) override;

private:
    std::shared_ptr<Mesh>      m_Mesh;
    Material                   m_Material;
    std::shared_ptr<Skeleton>  m_Skeleton;
    std::vector<AnimationClip> m_Clips;
    std::vector<glm::mat4>     m_BoneMatrices;
    std::vector<glm::mat4>     m_PoseScratch;   // reused per EvaluatePose
    std::vector<const struct JointTrack*> m_TrackByJoint; // reused per EvaluatePose

    int   m_CurrentClip    = 0;
    float m_Time           = 0.0f;
    bool  m_Playing        = true;
    bool  m_Loop           = true;
    float m_Speed          = 1.0f;
    std::vector<AnimRule>    m_AnimRules;
    std::vector<std::string> m_ExtraClipFiles;
    float                    m_PhysicsYOffset = -1.1f;

    void      EvaluatePose();
    glm::vec3 SampleVec3(const std::vector<float>& times,
                         const std::vector<glm::vec3>& vals,
                         float t, const glm::vec3& def) const;
    glm::quat SampleQuat(const std::vector<float>& times,
                         const std::vector<glm::quat>& vals, float t) const;
};
