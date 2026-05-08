#pragma once

#include <string>
#include <memory>

class Scene;

class SceneSerializer {
public:
    static void Save(Scene& scene, const std::string& filepath);
    static std::shared_ptr<Scene> Load(const std::string& filepath);

    // In-memory snapshot (no file I/O) — used by Play Mode
    static std::string             SaveToString(Scene& scene);
    static std::shared_ptr<Scene>  LoadFromString(const std::string& data);
};
