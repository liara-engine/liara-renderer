#pragma once

/**
 * @file Frames.h
 */

#include "swapchain/PresentStatus.h"

#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <cstddef>
#include <vector>

namespace Liara::Renderer
{
    class Device;
    class Swapchain;

    /** @brief How many frames the CPU records while the GPU still works on earlier ones. */
    inline constexpr std::size_t FRAMES_IN_FLIGHT = 2;

    /**
     * @brief The frames in flight, each with its command buffer, its fence and its "image acquired" semaphore, and
     * what a frame does: clear the next swapchain image and present it.
     *
     * The clear is the load operation of an empty dynamic rendering scope, the scope the triangle will be drawn in.
     */
    class Frames
    {
    public:
        /** @param device, swapchain Both outlive this object. */
        Frames(const Device& device, Swapchain& swapchain);
        ~Frames() = default;

        Frames(const Frames&) = delete;
        Frames& operator=(const Frames&) = delete;
        Frames(Frames&&) = delete;
        Frames& operator=(Frames&&) = delete;

        /** @brief Clear the next image to `clearColor` (linear RGBA, or encoded for a non-sRGB format) and present it.
         */
        [[nodiscard]] PresentStatus Draw(const std::array<float, 4>& clearColor);

    private:
        struct Frame
        {
            vk::raii::CommandBuffer m_Commands;
            vk::raii::Fence m_InFlight;
            vk::raii::Semaphore m_ImageAcquired;
        };

        void Record(const Frame& frame, uint32_t imageIndex, const std::array<float, 4>& clearColor) const;

        const Device* m_Device;
        Swapchain* m_Swapchain;
        std::vector<Frame> m_Frames;
        std::size_t m_Current = 0;
    };
}  // namespace Liara::Renderer
