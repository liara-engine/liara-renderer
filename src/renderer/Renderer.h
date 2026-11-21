#pragma once

/**
 * @file Renderer.h
 */

#include "CreateInfo.h"

#include "../swapchain/PresentStatus.h"

#include <liara/mesh.h>
#include <liara/renderer/packet.h>
#include <liara/result.h>

#include <memory>

namespace Liara::Renderer
{
    class Allocator;
    class Device;
    class Frames;
    class Instance;
    class Loader;
    class Meshes;
    class Surface;
    class Swapchain;

    /** @brief Everything one renderer instance owns, behind `struct liara_renderer_t`. */
    class Renderer
    {
    public:
        /** @brief Create everything; throws on failure, and the shim translates. */
        explicit Renderer(const RendererConfig& config);
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(Renderer&&) = delete;

        /**
         * @brief Check one packet, then, with a window, draw its views into the window and present it.
         *
         * A packet failing its checks draws nothing. Once the swapchain is out of date or the surface lost, every later
         * call returns that code at once: nothing recreates the swapchain yet.
         */
        [[nodiscard]] liara_result_t SubmitFrame(const liara_render_packet_t& packet);

        /** @brief Put a mesh on the GPU; see `Meshes::Upload`. Call only once its data passed `ValidateMeshData`. */
        [[nodiscard]] liara_result_t UploadMesh(liara_mesh_id_t mesh, const liara_mesh_data_t& data);

        /** @brief Free a mesh once the GPU is idle; see `Meshes::Release`. */
        [[nodiscard]] liara_result_t ReleaseMesh(liara_mesh_id_t mesh);

    private:
        // Destroyed in reverse order, from the most derived Vulkan object to the loader.
        std::unique_ptr<Loader> m_Loader;
        std::unique_ptr<Instance> m_Instance;
        std::unique_ptr<Surface> m_Surface; /**< Null for a renderer without a window. */
        std::unique_ptr<Device> m_Device;
        std::unique_ptr<Allocator> m_Allocator;
        std::unique_ptr<Meshes> m_Meshes;
        std::unique_ptr<Swapchain> m_Swapchain; /**< Null for a renderer without a window. */
        std::unique_ptr<Frames> m_Frames;       /**< Null for a renderer without a window. */
        PresentStatus m_PresentStatus = PresentStatus::Ok;
    };
}  // namespace Liara::Renderer
