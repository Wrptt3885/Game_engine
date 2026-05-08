#pragma once

#include "renderer/Mesh.h"
#include <memory>
#include <string>

class OBJLoader {
public:
    static std::shared_ptr<Mesh> Load(const std::string& filepath);
};
