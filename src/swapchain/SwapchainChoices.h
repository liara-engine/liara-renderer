#pragma once

/**
 * @file SwapchainChoices.h
 *
 * What the swapchain is made of, chosen from what the surface reports, as pure functions over plain data. Querying the
 * surface and creating the swapchain belong to Swapchain.cpp, which is what lets these be tested without a GPU
 * (tests/test_renderer_swapchain_choices_cxx.cpp compiles SwapchainChoices.cpp on its own).
 */

#include <liara/extent.h>

#include <vulkan/vulkan.hpp>

#include <array>
#include <cstdint>
#include <span>

namespace Liara::Renderer
{
    /**
     * @brief `B8G8R8A8_SRGB` then `R8G8B8A8_SRGB`, each with `SRGB_NONLINEAR`, then the first format reported.
     *
     * Precondition: `formats` is not empty, which Vulkan guarantees for a surface the device can present to.
     */
    [[nodiscard]] vk::SurfaceFormatKHR ChooseSurfaceFormat(std::span<const vk::SurfaceFormatKHR> formats);

    /** @brief One more image than the minimum, capped by the maximum unless that is 0 (no maximum). */
    [[nodiscard]] uint32_t ChooseImageCount(const vk::SurfaceCapabilitiesKHR& capabilities);

    /**
     * @brief The surface's own extent, or, when it has none (`currentExtent` of `0xFFFFFFFF`, as under Wayland), the
     * window's pixel size clamped per axis to what the surface accepts. A zero extent is returned as is: the caller
     * refuses it.
     */
    [[nodiscard]] vk::Extent2D ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities, liara_extent2d_t pixelSize);

    /** @brief Opaque when supported, otherwise the lowest supported bit. */
    [[nodiscard]] vk::CompositeAlphaFlagBitsKHR ChooseCompositeAlpha(vk::CompositeAlphaFlagsKHR supported);

    /** @brief Whether writes to an image of this format are encoded to sRGB by the hardware. */
    [[nodiscard]] bool IsSrgbFormat(vk::Format format) noexcept;

    /**
     * @brief The clear value for a packet colour, `0xAARRGGBB` with the alpha ignored (the result's alpha is 1).
     *
     * The packet's colours are sRGB-encoded. An sRGB image expects linear values and encodes them itself, so each
     * channel is decoded first; any other format takes the encoded value as is.
     */
    [[nodiscard]] std::array<float, 4> ClearColorFromPacket(uint32_t argb, bool srgbFormat) noexcept;
}  // namespace Liara::Renderer
