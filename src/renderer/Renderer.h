#pragma once

/**
 * @file Renderer.h
 */

#include "CreateInfo.h"

#include "../console/ConsoleRenderer.h"
#include "../swapchain/PresentStatus.h"

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
    class Surface;
    class Swapchain;

    /**
     * @brief Everything one renderer instance owns, behind `struct liara_renderer_t`.
     *
     * The console renderer is declared last so that it is destroyed first: it leaves the terminal's alternate screen
     * before anything else is torn down, and messages emitted during the rest of the destruction land on the main
     * screen, where they stay visible.
     */
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
         * @brief Draw one packet: in the terminal, then, with a window, clear the window to the packet's background
         * colour and present it.
         *
         * Once the swapchain is out of date or the surface lost, every later call returns that code at once, the
         * console still drawing: nothing recreates the swapchain yet.
         */
        [[nodiscard]] liara_result_t SubmitFrame(const liara_render_packet_t& packet);

    private:
        // Destroyed in reverse order: the console first, then the Vulkan objects from the most derived to the loader.
        std::unique_ptr<Loader> m_Loader;
        std::unique_ptr<Instance> m_Instance;
        std::unique_ptr<Surface> m_Surface; /**< Null for a renderer without a window. */
        std::unique_ptr<Device> m_Device;
        std::unique_ptr<Allocator> m_Allocator;
        std::unique_ptr<Swapchain> m_Swapchain; /**< Null for a renderer without a window. */
        std::unique_ptr<Frames> m_Frames;       /**< Null for a renderer without a window. */
        PresentStatus m_PresentStatus = PresentStatus::Ok;
        ConsoleRenderer m_Console;
    };
}  // namespace Liara::Renderer
