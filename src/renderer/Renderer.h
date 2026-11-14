#pragma once

/**
 * @file Renderer.h
 */

#include "CreateInfo.h"

#include "../console/ConsoleRenderer.h"

#include <liara/renderer/packet.h>
#include <liara/result.h>

#include <memory>

namespace Liara::Renderer
{
    class Allocator;
    class Device;
    class Instance;
    class Loader;
    class Surface;

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

        /** @brief Draw one packet. Still the console renderer's job. */
        [[nodiscard]] liara_result_t SubmitFrame(const liara_render_packet_t& packet);

    private:
        // Destroyed in reverse order: the console first, then the Vulkan objects from the most derived to the loader.
        std::unique_ptr<Loader> m_Loader;
        std::unique_ptr<Instance> m_Instance;
        std::unique_ptr<Surface> m_Surface; /**< Null for a renderer without a window. */
        std::unique_ptr<Device> m_Device;
        std::unique_ptr<Allocator> m_Allocator;
        ConsoleRenderer m_Console;
    };
}  // namespace Liara::Renderer
