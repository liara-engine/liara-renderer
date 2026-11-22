#pragma once

/**
 * @file Frames.h
 */

#include "passes/ViewPass.h"
#include "swapchain/PresentStatus.h"

#include <liara/renderer/packet.h>

#include <vulkan/vulkan_raii.hpp>

#include <cstddef>
#include <vector>

namespace Liara::Renderer
{
    class Device;
    class Meshes;
    class MeshPipeline;
    class Swapchain;

    /** @brief How many frames the CPU records while the GPU still works on earlier ones. */
    inline constexpr std::size_t FRAMES_IN_FLIGHT = 2;

    /**
     * @brief The frames in flight, each with its command buffer, its fence and its "image acquired" semaphore, and
     * what a frame does: draw a packet's views into the next swapchain image and present it.
     *
     * What is drawn inside the image is the view pass's; this class owns the image's layout transitions around it, the
     * synchronization and the presentation.
     */
    class Frames
    {
    public:
        /** @param device, swapchain, meshes All outlive this object. */
        Frames(const Device& device, Swapchain& swapchain, const Meshes& meshes);
        ~Frames() = default;

        Frames(const Frames&) = delete;
        Frames& operator=(const Frames&) = delete;
        Frames(Frames&&) = delete;
        Frames& operator=(Frames&&) = delete;

        /**
         * @brief Draw a checked packet's views into the next image and present it, with a pipeline built for the
         * swapchain's format.
         *
         * @return `Suboptimal` when the acquisition or the presentation was, though the image was presented.
         */
        [[nodiscard]] PresentStatus Draw(const liara_render_packet_t& packet, const MeshPipeline& pipeline);

    private:
        struct Frame
        {
            vk::raii::CommandBuffer m_Commands;
            vk::raii::Fence m_InFlight;
            vk::raii::Semaphore m_ImageAcquired;
        };

        void Record(const Frame& frame,
                    uint32_t imageIndex,
                    const liara_render_packet_t& packet,
                    const MeshPipeline& pipeline);

        const Device* m_Device;
        Swapchain* m_Swapchain;
        std::vector<Frame> m_Frames;
        std::size_t m_Current = 0;
        ViewPass m_Pass;
    };
}  // namespace Liara::Renderer
