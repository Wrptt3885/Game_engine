#include "rhi/RHI.h"

GraphicsAPI RHI::s_API = GraphicsAPI::OpenGL;

void RHI::Init(GraphicsAPI api) {
    s_API = api;
}
