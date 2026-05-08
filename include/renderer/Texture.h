#pragma once

#include <string>
#include <memory>

class Texture {
public:
    virtual ~Texture() = default;

    virtual void Bind(unsigned int slot = 0) const = 0;
    virtual int  GetWidth()  const = 0;
    virtual int  GetHeight() const = 0;
    virtual bool IsLoaded()  const = 0;
    virtual const std::string& GetPath() const = 0;

    static std::shared_ptr<Texture> Create(const std::string& path);
    // Create from raw RGBA pixel data (for embedded GLB textures)
    static std::shared_ptr<Texture> CreateFromMemory(
        int width, int height, const unsigned char* rgba,
        const std::string& debugName = "");
};
