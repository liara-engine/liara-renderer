#pragma once

/**
 * @file Shaders.h
 */

#include <cstdint>
#include <span>

/**
 * @brief The renderer's built-in shaders, as SPIR-V.
 *
 * The GLSL sources live in `shaders/`. The build compiles them with vcpkg's glslc (ADR 0017) and embeds the result, so
 * the renderer reads no file at run time and ships as one binary.
 */
namespace Liara::Renderer::Shaders
{
    /** @brief `shaders/mesh.vert`. */
    [[nodiscard]] std::span<const uint32_t> MeshVertex() noexcept;

    /** @brief `shaders/mesh.frag`. */
    [[nodiscard]] std::span<const uint32_t> MeshFragment() noexcept;
}  // namespace Liara::Renderer::Shaders
