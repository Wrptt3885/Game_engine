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

class GLTFLoader {
public:
    // Import a GLTF/GLB file: adds GameObjects to scene, returns them.
    // Nodes with skins get SkinnedMeshRenderer; others get MeshRenderer.
    static std::vector<std::shared_ptr<GameObject>> Import(
        const std::string& filepath, Scene& scene);

    // Load a single static Mesh for SceneSerializer reload.
    // filepath may contain "::N" suffix identifying the primitive index.
    static std::shared_ptr<Mesh> LoadMesh(const std::string& filepath);

    // Skinned mesh data bundle returned by LoadSkinnedMesh
    struct SkinnedMeshData {
        std::shared_ptr<Mesh>      mesh;
        Material                   material;
        std::shared_ptr<Skeleton>  skeleton;
        std::vector<AnimationClip> clips;
    };

    // Reload a skinned primitive for SceneSerializer.
    // filepath may contain "::N" suffix identifying the primitive index.
    static SkinnedMeshData LoadSkinnedMesh(const std::string& filepath);

    // Load animation clips + source skeleton from a GLB/GLTF file (skin 0).
    // Caller uses the skeleton to remap joint indices to match a different target skeleton.
    struct ClipBundle {
        std::shared_ptr<Skeleton>  skeleton;
        std::vector<AnimationClip> clips;
    };
    static ClipBundle LoadClipsFromFile(const std::string& filepath);
};
