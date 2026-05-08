cmake_minimum_required(VERSION 3.20)
project(MyEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- Options ----------------------------------------------------------------
option(USE_DX11        "Build with DirectX 11 backend instead of OpenGL" OFF)
option(DX11_DEBUG      "Enable D3D11 debug layer + validation (development only)" OFF)

# ---- Find packages ----------------------------------------------------------
if(NOT USE_DX11)
    if(NOT WIN32)
        set(OpenGL_GL_PREFERENCE GLVND)
    endif()
    find_package(OpenGL REQUIRED)
endif()

# MSYS2 paths for GLFW / GLM
set(MSYS2_ROOT "C:/msys64/mingw64")
find_package(glfw3 QUIET)
if(NOT glfw3_FOUND)
    # Fall back to MSYS2 manual path
    set(GLFW_INCLUDE "${MSYS2_ROOT}/include")
    set(GLFW_LIB     "${MSYS2_ROOT}/lib/libglfw3.a")
    add_library(glfw STATIC IMPORTED)
    set_target_properties(glfw PROPERTIES
        IMPORTED_LOCATION "${GLFW_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${GLFW_INCLUDE}")
endif()

find_package(glm QUIET)
if(NOT glm_FOUND)
    set(GLM_INCLUDE "${MSYS2_ROOT}/include")
    add_library(glm::glm INTERFACE IMPORTED)
    set_target_properties(glm::glm PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${GLM_INCLUDE}")
endif()

# ---- Jolt Physics -----------------------------------------------------------
set(TARGET_UNIT_TESTS        OFF CACHE BOOL "" FORCE)
set(TARGET_HELLO_WORLD       OFF CACHE BOOL "" FORCE)
set(TARGET_PERFORMANCE_TEST  OFF CACHE BOOL "" FORCE)
set(TARGET_SAMPLES           OFF CACHE BOOL "" FORCE)
set(TARGET_VIEWER            OFF CACHE BOOL "" FORCE)
set(ENABLE_ALL_WARNINGS      OFF CACHE BOOL "" FORCE)
set(OVERRIDE_CXX_FLAGS       OFF CACHE BOOL "" FORCE)
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
set(JPH_USE_DX12             OFF CACHE BOOL "" FORCE)
set(JPH_USE_VK               OFF CACHE BOOL "" FORCE)
set(JPH_USE_MTL              OFF CACHE BOOL "" FORCE)
set(JPH_USE_CPU_COMPUTE      OFF CACHE BOOL "" FORCE)
set(CPP_RTTI_ENABLED         ON  CACHE BOOL "" FORCE)
add_subdirectory(vendor/JoltPhysics/Build JoltBuild)

# ---- Sources ----------------------------------------------------------------
file(GLOB_RECURSE SOURCES src/*.cpp src/*.c)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/vendor/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
)
if(USE_DX11)
    list(APPEND IMGUI_SOURCES ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp)
else()
    list(APPEND IMGUI_SOURCES ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)
endif()

set(IMGUIZMO_DIR ${CMAKE_SOURCE_DIR}/vendor/imguizmo)
set(IMGUIZMO_SOURCES ${IMGUIZMO_DIR}/ImGuizmo.cpp)

add_executable(MyEngine ${SOURCES} ${IMGUI_SOURCES} ${IMGUIZMO_SOURCES})

# ---- Includes ---------------------------------------------------------------
target_include_directories(MyEngine PRIVATE
    include
    vendor
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMGUIZMO_DIR}
)

# ---- Definitions ------------------------------------------------------------
target_compile_definitions(MyEngine PRIVATE
    SHADER_DIR="${CMAKE_SOURCE_DIR}/shaders"
    ASSET_DIR="${CMAKE_SOURCE_DIR}/assets"
    GLM_ENABLE_EXPERIMENTAL
    $<$<PLATFORM_ID:Windows>:NOMINMAX WIN32_LEAN_AND_MEAN>
)

if(USE_DX11)
    target_compile_definitions(MyEngine PRIVATE
        USE_DX11_BACKEND
        GLM_FORCE_DEPTH_ZERO_TO_ONE
    )
    if(DX11_DEBUG)
        target_compile_definitions(MyEngine PRIVATE DX11_DEBUG_LAYER)
        message(STATUS "[DX11] Debug layer ENABLED — expect verbose output")
    endif()
endif()

# ---- Link libraries ---------------------------------------------------------
target_link_libraries(MyEngine PRIVATE glfw glm::glm Jolt)

if(USE_DX11)
    target_link_libraries(MyEngine PRIVATE d3d11 dxgi d3dcompiler dxguid)
else()
    target_link_libraries(MyEngine PRIVATE OpenGL::GL)
endif()

if(WIN32)
    target_link_libraries(MyEngine PRIVATE dwmapi)
endif()
