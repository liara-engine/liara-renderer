#pragma once

/**
 * @file MeshData.h
 */

#include <liara/mesh.h>
#include <liara/result.h>

namespace Liara::Renderer
{
    /**
     * @brief The checks an upload makes on its arguments before anything reaches the GPU, in the contract's order.
     *
     * On non-null data: the version, and nothing past it is read before it passes; the identifier (not
     * `LIARA_MESH_ID_NONE`); the pointers (positions, and indices when there is an index count); the counts (at least
     * one vertex, and a list of triangles); the ranges (counts a draw can take, indices below the vertex count). The
     * shim checks the null arguments before this, and the mesh table whether the identifier is taken after it. Nothing
     * is logged.
     */
    [[nodiscard]] liara_result_t ValidateMeshData(liara_mesh_id_t mesh, const liara_mesh_data_t& data) noexcept;
}  // namespace Liara::Renderer
