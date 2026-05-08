#include "renderer/OBJLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

// Key for de-duplicating OBJ vertex combinations (pos/tex/norm indices)
struct OBJIndex {
    int posIdx = 0, texIdx = 0, normIdx = 0;
    bool operator==(const OBJIndex& o) const {
        return posIdx == o.posIdx && texIdx == o.texIdx && normIdx == o.normIdx;
    }
};

struct OBJIndexHash {
    size_t operator()(const OBJIndex& i) const {
        size_t h = std::hash<int>()(i.posIdx);
        h ^= std::hash<int>()(i.texIdx) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>()(i.normIdx) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

// Parse one face token: "v", "v/vt", "v//vn", "v/vt/vn"
static OBJIndex ParseFaceToken(const std::string& token) {
    OBJIndex idx;
    std::stringstream ss(token);
    std::string part;
    int field = 0;
    while (std::getline(ss, part, '/')) {
        if (!part.empty()) {
            int val = std::stoi(part);
            if (field == 0) idx.posIdx  = val;
            if (field == 1) idx.texIdx  = val;
            if (field == 2) idx.normIdx = val;
        }
        field++;
    }
    return idx;
}

std::shared_ptr<Mesh> OBJLoader::Load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[OBJLoader] Cannot open: " << filepath << std::endl;
        return nullptr;
    }

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::unordered_map<OBJIndex, unsigned int, OBJIndexHash> cache;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "v") {
            glm::vec3 p;
            ss >> p.x >> p.y >> p.z;
            positions.push_back(p);
        } else if (type == "vn") {
            glm::vec3 n;
            ss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        } else if (type == "vt") {
            glm::vec2 uv;
            ss >> uv.x >> uv.y;
#ifdef USE_DX11_BACKEND
            uv.y = 1.0f - uv.y;  // OBJ V=0=bottom, DX11 V=0=top
#endif
            texCoords.push_back(uv);
        } else if (type == "f") {
            std::vector<OBJIndex> face;
            std::string tok;
            while (ss >> tok)
                face.push_back(ParseFaceToken(tok));

            // Fan triangulation — works for convex polygons and quads
            for (size_t i = 1; i + 1 < face.size(); i++) {
                OBJIndex tri[3] = { face[0], face[i], face[i + 1] };
                for (auto& fi : tri) {
                    auto it = cache.find(fi);
                    if (it != cache.end()) {
                        indices.push_back(it->second);
                    } else {
                        Vertex v{};
                        if (fi.posIdx  > 0 && (size_t)(fi.posIdx  - 1) < positions.size()) v.position = positions [fi.posIdx  - 1];
                        if (fi.normIdx > 0 && (size_t)(fi.normIdx - 1) < normals.size()) v.normal   = normals   [fi.normIdx - 1];
                        if (fi.texIdx  > 0 && (size_t)(fi.texIdx  - 1) < texCoords.size()) v.texCoord = texCoords [fi.texIdx  - 1];
                        auto newIdx = (unsigned int)vertices.size();
                        vertices.push_back(v);
                        cache[fi] = newIdx;
                        indices.push_back(newIdx);
                    }
                }
            }
        }
    }

    if (vertices.empty()) {
        std::cerr << "[OBJLoader] No geometry found in: " << filepath << std::endl;
        return nullptr;
    }

    // Compute tangent/bitangent per triangle then accumulate
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        Vertex& v0 = vertices[indices[i]];
        Vertex& v1 = vertices[indices[i + 1]];
        Vertex& v2 = vertices[indices[i + 2]];

        glm::vec3 edge1  = v1.position - v0.position;
        glm::vec3 edge2  = v2.position - v0.position;
        glm::vec2 dUV1   = v1.texCoord - v0.texCoord;
        glm::vec2 dUV2   = v2.texCoord - v0.texCoord;

        float denom = dUV1.x * dUV2.y - dUV2.x * dUV1.y;
        if (std::abs(denom) < 1e-6f) continue;

        float f = 1.0f / denom;
        glm::vec3 tangent(
            f * (dUV2.y * edge1.x - dUV1.y * edge2.x),
            f * (dUV2.y * edge1.y - dUV1.y * edge2.y),
            f * (dUV2.y * edge1.z - dUV1.y * edge2.z)
        );

        v0.tangent += tangent;
        v1.tangent += tangent;
        v2.tangent += tangent;
    }

    // Gram-Schmidt orthogonalize and compute bitangent
    for (auto& v : vertices) {
        glm::vec3 n = glm::normalize(v.normal);
        glm::vec3 t = v.tangent - glm::dot(v.tangent, n) * n;
        if (glm::length(t) > 1e-6f) t = glm::normalize(t);
        v.tangent   = t;
        v.bitangent = glm::cross(n, t);
    }

    std::cout << "[OBJLoader] Loaded " << filepath
              << " — " << vertices.size() << " vertices, "
              << indices.size() / 3 << " triangles\n";

    auto mesh = Mesh::Create(vertices, indices);
    mesh->SetPath(filepath);
    return mesh;
}
