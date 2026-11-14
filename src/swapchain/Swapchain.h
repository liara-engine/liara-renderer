#pragma once

/**
 * @file Swapchain.h
 */

#include "PresentStatus.h"

#include <liara/extent.h>

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <vector>

namespace Liara::Renderer
{
    class Device;

    /**
     * @brief The swapchain presenting to the surface, its images and their views, and one "render finished" semaphore
     * per image.
     *
     * The semaphores are per image rather than per frame in flight: a present operation may still be waiting on the one
     * it was given when the next frame starts, and the validation layer reports reusing it. An image is acquired again
     * only once its previous presentation is done with its semaphore.
     *
     * Nothing recreates it yet: once out of date it stays so, and the renderer stops presenting.
     */
    class Swapchain
    {
    public:
        /** @brief Throws `std::runtime_error` when the surface has no area, as for a minimised window. */
        Swapchain(const Device& device, const vk::raii::SurfaceKHR& surface, liara_extent2d_t pixelSize);
        ~Swapchain() = default;

        Swapchain(const Swapchain&) = delete;
        Swapchain& operator=(const Swapchain&) = delete;
        Swapchain(Swapchain&&) = delete;
        Swapchain& operator=(Swapchain&&) = delete;

        [[nodiscard]] vk::Format GetFormat() const noexcept;
        [[nodiscard]] vk::Extent2D GetExtent() const noexcept;
        [[nodiscard]] vk::Image GetImage(uint32_t index) const;
        [[nodiscard]] const vk::raii::ImageView& GetView(uint32_t index) const;
        [[nodiscard]] const vk::raii::Semaphore& GetRenderFinished(uint32_t index) const;

        /** @brief Acquire the next image, signalling `imageAcquired`. On `Ok`, `outIndex` is that image. */
        [[nodiscard]] PresentStatus Acquire(const vk::raii::Semaphore& imageAcquired, uint32_t& outIndex);

        /** @brief Present an image once its "render finished" semaphore is signalled. */
        [[nodiscard]] PresentStatus Present(const vk::raii::Queue& queue, uint32_t index);

    private:
        vk::Format m_Format = vk::Format::eUndefined;
        vk::Extent2D m_Extent;
        vk::raii::SwapchainKHR m_Swapchain = nullptr;
        std::vector<vk::Image> m_Images;
        std::vector<vk::raii::ImageView> m_Views;
        std::vector<vk::raii::Semaphore> m_RenderFinished;
    };
}  // namespace Liara::Renderer
