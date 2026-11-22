#pragma once

/**
 * @file PresentStatus.h
 */

#include <cstdint>

namespace Liara::Renderer
{
    /** @brief What became of an acquisition or a presentation. */
    enum class PresentStatus : std::uint8_t
    {
        Ok,
        Suboptimal,  /**< VK_SUBOPTIMAL_KHR: acquired or presented, but the swapchain no longer matches the surface. */
        OutOfDate,   /**< VK_ERROR_OUT_OF_DATE_KHR: the swapchain no longer matches the window. */
        SurfaceLost, /**< VK_ERROR_SURFACE_LOST_KHR: the window system took the surface away. */
    };
}  // namespace Liara::Renderer
