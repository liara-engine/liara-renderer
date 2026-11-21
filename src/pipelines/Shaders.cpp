#include "Shaders.h"

#include <array>
#include <cstdint>
#include <span>

namespace Liara::Renderer::Shaders
{
    namespace
    {
        // Each .inc is glslc's `-mfmt=c` output: the SPIR-V words as one braced initializer list, generated into the
        // build directory by CMake.
        constexpr auto MESH_VERTEX = std::to_array<uint32_t>(
#include "shaders/mesh.vert.inc"
        );

        constexpr auto MESH_FRAGMENT = std::to_array<uint32_t>(
#include "shaders/mesh.frag.inc"
        );
    }  // namespace

    std::span<const uint32_t> MeshVertex() noexcept { return MESH_VERTEX; }

    std::span<const uint32_t> MeshFragment() noexcept { return MESH_FRAGMENT; }
}  // namespace Liara::Renderer::Shaders
