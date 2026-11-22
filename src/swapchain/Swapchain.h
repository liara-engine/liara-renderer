#pragma once

/**
 * @file Swapchain.h
 */

#include "PresentStatus.h"
#include "SwapchainChoices.h"

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
     * It is recreated in place, so that whatever points at it keeps doing so, and can be released while the window
     * has no area, which leaves it without images until the next recreation.
     */
    class Swapchain
    {
    public:
        /**
         * @brief Throws `std::runtime_error` when the surface has no area, as for a minimised window.
         *
         * @param device, surface Both outlive this object, which recreates itself on them.
         */
        Swapchain(const Device& device, const vk::raii::SurfaceKHR& surface, liara_extent2d_t pixelSize);
        ~Swapchain() = default;

        Swapchain(const Swapchain&) = delete;
        Swapchain& operator=(const Swapchain&) = delete;
        Swapchain(Swapchain&&) = delete;
        Swapchain& operator=(Swapchain&&) = delete;

        /**
         * @brief Whether this swapchain can stay as it is for a window of `windowSize` pixels: queries the surface's
         * capabilities, then `PlanSwapchain`.
         *
         * @param invalid Whether an acquisition or a presentation reported it out of date or suboptimal.
         */
        [[nodiscard]] SwapchainPlan Plan(liara_extent2d_t windowSize, bool invalid) const;

        /**
         * @brief Replace the swapchain with one chosen anew for a window of `windowSize` pixels, once the GPU is idle.
         * The current one, if any, is handed over as `oldSwapchain`. Throws `std::runtime_error` when the surface has
         * no area, which `Plan` reports first.
         */
        void Recreate(liara_extent2d_t windowSize);

        /** @brief Drop the swapchain and everything per image, once the GPU is idle. */
        void Release();

        /** @brief False once released, until the next recreation. */
        [[nodiscard]] bool HasImages() const noexcept;

        [[nodiscard]] vk::Format GetFormat() const noexcept;
        [[nodiscard]] vk::Extent2D GetExtent() const noexcept;
        [[nodiscard]] vk::Image GetImage(uint32_t index) const;
        [[nodiscard]] const vk::raii::ImageView& GetView(uint32_t index) const;
        [[nodiscard]] const vk::raii::Semaphore& GetRenderFinished(uint32_t index) const;

        /**
         * @brief Acquire the next image, signalling `imageAcquired`. On `Ok` or `Suboptimal`, `outIndex` is that
         * image.
         */
        [[nodiscard]] PresentStatus Acquire(const vk::raii::Semaphore& imageAcquired, uint32_t& outIndex);

        /** @brief Present an image once its "render finished" semaphore is signalled. */
        [[nodiscard]] PresentStatus Present(const vk::raii::Queue& queue, uint32_t index);

    private:
        /** @brief Create the swapchain on the surface, handing over the current one, and log it. */
        void Create(liara_extent2d_t windowSize, bool recreating);

        const Device* m_Device;
        const vk::raii::SurfaceKHR* m_Surface;
        vk::Format m_Format = vk::Format::eUndefined;
        vk::Extent2D m_Extent;
        vk::raii::SwapchainKHR m_Swapchain = nullptr;
        std::vector<vk::Image> m_Images;
        std::vector<vk::raii::ImageView> m_Views;
        std::vector<vk::raii::Semaphore> m_RenderFinished;
    };
}  // namespace Liara::Renderer
