#pragma once
#include "renderer/Material.h"
#include "renderer/Skeleton.h"
#include "renderer/AnimationClip.h"
#include <string>
#include <memory>
#include <vector>

class Scene;
class Mesh;
class GameObject;

class FBXLoader {
public:
    // Import an FBX file: adds GameObjects to scene, returns them.
    // Nodes with skin deformers get SkinnedMeshRenderer; others get MeshRenderer.
    static std::vector<std::shared_ptr<GameObject>> Import(
        const std::string& filepath, Scene& scene);

    // Load a single static Mesh for SceneSerializer reload.
    // filepath may contain "::N" suffix identifying the mesh node index.
    static std::shared_ptr<Mesh> LoadMesh(const std::string& filepath);

    struct SkinnedMeshData {
        std::shared_ptr<Mesh>      mesh;
        Material                   material;
        std::shared_ptr<Skeleton>  skeleton;
        std::vector<AnimationClip> clips;
    };

    // Reload a skinned mesh for SceneSerializer.
    static SkinnedMeshData LoadSkinnedMesh(const std::string& filepath);

    struct ClipBundle {
        std::shared_ptr<Skeleton>  skeleton;
        std::vector<AnimationClip> clips;
    };

    // Load animation clips from an FBX file (for "Add Clip" dialog).
    static ClipBundle LoadClipsFromFile(const std::string& filepath);
};
