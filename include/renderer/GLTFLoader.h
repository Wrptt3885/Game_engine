#pragma once
#include <string>
#include <memory>
#include <vector>

class Scene;
class Mesh;
class GameObject;

class GLTFLoader {
public:
    // Import a GLTF/GLB file: adds GameObjects to scene, returns them.
    // Each mesh's path is set to "file.gltf::N" (N = primitive index) for serialization.
    static std::vector<std::shared_ptr<GameObject>> Import(
        const std::string& filepath, Scene& scene);

    // Load a single Mesh for SceneSerializer reload.
    // filepath may contain "::N" suffix identifying the primitive index.
    static std::shared_ptr<Mesh> LoadMesh(const std::string& filepath);
};
