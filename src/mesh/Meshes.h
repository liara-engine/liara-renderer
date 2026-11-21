#pragma once

/**
 * @file Meshes.h
 */

#include "device/Buffer.h"

#include <liara/mesh.h>
#include <liara/result.h>

#include <vulkan/vulkan.hpp>

#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace Liara::Renderer
{
    class Allocator;
    class Device;

    /** @brief A mesh on the GPU: one buffer holding its positions, its colours and its indices, at these offsets. */
    struct GpuMesh
    {
        Buffer m_Buffer;
        vk::DeviceSize m_PositionsOffset = 0;
        vk::DeviceSize m_ColorsOffset = 0;
        vk::DeviceSize m_IndicesOffset = 0;
        uint32_t m_VertexCount = 0;
        uint32_t m_IndexCount = 0;  ///< 0: not indexed.
    };

    /**
     * @brief The meshes the host uploaded, keyed by the identifier their source chose.
     *
     * An upload copies the data into a staging buffer, then, on the device's one queue, into one device-local buffer,
     * and waits for the copy to complete before returning: v0.x loads synchronously. A mesh without colours gets white
     * ones, so that one pipeline serves every mesh.
     */
    class Meshes
    {
    public:
        /** @param device, allocator Both outlive this object. */
        Meshes(const Device& device, const Allocator& allocator);
        ~Meshes() = default;

        Meshes(const Meshes&) = delete;
        Meshes& operator=(const Meshes&) = delete;
        Meshes(Meshes&&) = delete;
        Meshes& operator=(Meshes&&) = delete;

        /**
         * @brief Put a mesh on the GPU. Call only once `ValidateMeshData` returned `LIARA_RESULT_SUCCESS`.
         *
         * @return `LIARA_RESULT_SUCCESS`, or `LIARA_RESULT_RESOURCE_ALREADY_EXISTS` for a taken identifier, with
         * nothing logged. Throws on a failure of the graphics API.
         */
        [[nodiscard]] liara_result_t Upload(liara_mesh_id_t mesh, const liara_mesh_data_t& data);

        /**
         * @brief Wait for the GPU to be idle, then free the mesh.
         *
         * @return `LIARA_RESULT_SUCCESS`, or `LIARA_RESULT_RESOURCE_NOT_FOUND` for an identifier not uploaded. Throws
         * when waiting fails.
         */
        [[nodiscard]] liara_result_t Release(liara_mesh_id_t mesh);

        /** @brief The mesh uploaded under that identifier, or null. */
        [[nodiscard]] const GpuMesh* Find(liara_mesh_id_t mesh) const noexcept;

        /** @brief How many meshes are uploaded. */
        [[nodiscard]] std::size_t Count() const noexcept;

    private:
        const Device* m_Device;
        const Allocator* m_Allocator;
        std::unordered_map<liara_mesh_id_t, GpuMesh> m_Meshes;
    };
}  // namespace Liara::Renderer
