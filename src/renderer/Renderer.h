#pragma once

/**
 * @file Renderer.h
 */

#include "CreateInfo.h"

#include <liara/extent.h>
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
    class MeshPipeline;
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
         * @brief Check one packet, then, with a window, bring the swapchain in line with the window, draw the packet's
         * views into it and present it.
         *
         * A packet failing its checks draws nothing. The swapchain is looked at again only when something asked for it:
         * a size pushed through `WindowResized`, an earlier frame finding it out of date or suboptimal, or a window
         * that had no area. It is then kept, recreated, or, while the window has no area, released, and the call
         * returns `LIARA_RESULT_SURFACE_OUT_OF_DATE` without presenting. Once the surface is lost, every later call
         * returns `LIARA_RESULT_SURFACE_LOST`.
         */
        [[nodiscard]] liara_result_t SubmitFrame(const liara_render_packet_t& packet);

        /** @brief Put a mesh on the GPU; see `Meshes::Upload`. Call only once its data passed `ValidateMeshData`. */
        [[nodiscard]] liara_result_t UploadMesh(liara_mesh_id_t mesh, const liara_mesh_data_t& data);

        /** @brief Free a mesh once the GPU is idle; see `Meshes::Release`. */
        [[nodiscard]] liara_result_t ReleaseMesh(liara_mesh_id_t mesh);

        /**
         * @brief Keep the window's size, in pixels, as the host reported it, and have the next frame look at the
         * swapchain. A renderer without a window ignores it.
         *
         * It matters where the surface has no size of its own (Wayland): the swapchain's extent then comes from it.
         */
        void WindowResized(liara_extent2d_t pixelSize) noexcept;

    private:
        /**
         * @brief Bring the swapchain in line with the window if something asked for it.
         *
         * @return False while the window has no area, in which case there is no swapchain to draw on.
         */
        [[nodiscard]] bool SwapchainReady();

        // Destroyed in reverse order, from the most derived Vulkan object to the loader.
        std::unique_ptr<Loader> m_Loader;
        std::unique_ptr<Instance> m_Instance;
        std::unique_ptr<Surface> m_Surface; /**< Null for a renderer without a window. */
        std::unique_ptr<Device> m_Device;
        std::unique_ptr<Allocator> m_Allocator;
        std::unique_ptr<Meshes> m_Meshes;
        std::unique_ptr<Swapchain> m_Swapchain;   /**< Null for a renderer without a window. */
        std::unique_ptr<MeshPipeline> m_Pipeline; /**< Null for a renderer without a window. */
        std::unique_ptr<Frames> m_Frames;         /**< Null for a renderer without a window. */
        liara_extent2d_t m_WindowSize {};         /**< The window's size, as the host last reported it. */
        bool m_CheckSwapchain = false;            /**< The next frame looks at the swapchain before drawing. */
        bool m_SwapchainInvalid = false; /**< Found out of date or suboptimal: recreate even at the same extent. */
        bool m_SurfaceLost = false;      /**< Sticky: nothing is presented again. */
    };
}  // namespace Liara::Renderer
