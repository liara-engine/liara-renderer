#pragma once

/**
 * @file PresentStatus.h
 */

#include <cstdint>

namespace Liara::Renderer
{
    /**
     * @brief What became of an acquisition or a presentation. `Ok` includes VK_SUBOPTIMAL_KHR: the image was acquired
     * or presented, and only a later stage decides whether a suboptimal swapchain is worth recreating.
     */
    enum class PresentStatus : std::uint8_t
    {
        Ok,
        OutOfDate,   /**< VK_ERROR_OUT_OF_DATE_KHR: the swapchain no longer matches the window. */
        SurfaceLost, /**< VK_ERROR_SURFACE_LOST_KHR: the window system took the surface away. */
    };
}  // namespace Liara::Renderer
