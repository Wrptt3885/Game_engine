#pragma once

enum class GraphicsAPI { OpenGL, DirectX11 };

class RHI {
public:
    static void       Init(GraphicsAPI api);
    static GraphicsAPI GetAPI() { return s_API; }

private:
    static GraphicsAPI s_API;
};
