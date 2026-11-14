#include "SwapchainChoices.h"

#include <liara/extent.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>

namespace Liara::Renderer
{
    namespace
    {
        constexpr uint32_t NO_SIZE_OF_ITS_OWN = 0xFFFFFFFFU;

        constexpr std::array PREFERRED_FORMATS {vk::Format::eB8G8R8A8Srgb, vk::Format::eR8G8B8A8Srgb};

        constexpr float MAX_CHANNEL = 255.0F;
        constexpr uint32_t RED_SHIFT = 16;
        constexpr uint32_t GREEN_SHIFT = 8;
        constexpr size_t COLOUR_CHANNELS = 3;

        // The sRGB transfer function's constants (IEC 61966-2-1).
        constexpr float SRGB_LINEAR_THRESHOLD = 0.04045F;
        constexpr float SRGB_LINEAR_SLOPE = 12.92F;
        constexpr float SRGB_OFFSET = 0.055F;
        constexpr float SRGB_GAMMA = 2.4F;

        /** @brief One 8-bit channel of a packet colour, as a value in [0, 1]. */
        float Channel(const uint32_t argb, const uint32_t shift) noexcept {
            return static_cast<float>((argb >> shift) & 0xFFU) / MAX_CHANNEL;
        }

        /** @brief The sRGB transfer function, inverted: an encoded value in [0, 1] to a linear one. */
        float DecodeSrgb(const float encoded) noexcept {
            if (encoded <= SRGB_LINEAR_THRESHOLD) { return encoded / SRGB_LINEAR_SLOPE; }
            return std::pow((encoded + SRGB_OFFSET) / (1.0F + SRGB_OFFSET), SRGB_GAMMA);
        }
    }  // namespace

    vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::span<const vk::SurfaceFormatKHR> formats) {
        for (const vk::Format preferred : PREFERRED_FORMATS) {
            const auto found = std::ranges::find_if(formats, [preferred](const vk::SurfaceFormatKHR& format) {
                return format.format == preferred && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
            });
            if (found != formats.end()) { return *found; }
        }
        return formats.front();
    }

    uint32_t ChooseImageCount(const vk::SurfaceCapabilitiesKHR& capabilities) {
        const uint32_t wanted = capabilities.minImageCount + 1;
        return capabilities.maxImageCount == 0 ? wanted : std::min(wanted, capabilities.maxImageCount);
    }

    vk::Extent2D ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities, const liara_extent2d_t pixelSize) {
        if (capabilities.currentExtent.width != NO_SIZE_OF_ITS_OWN) { return capabilities.currentExtent; }
        return {std::clamp(pixelSize.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
                std::clamp(pixelSize.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};
    }

    vk::CompositeAlphaFlagBitsKHR ChooseCompositeAlpha(const vk::CompositeAlphaFlagsKHR supported) {
        using Alpha = vk::CompositeAlphaFlagBitsKHR;
        for (const Alpha candidate : {Alpha::eOpaque, Alpha::ePreMultiplied, Alpha::ePostMultiplied, Alpha::eInherit}) {
            if (supported & candidate) { return candidate; }
        }
        return Alpha::eOpaque;
    }

    bool IsSrgbFormat(const vk::Format format) noexcept {
        return std::ranges::find(PREFERRED_FORMATS, format) != PREFERRED_FORMATS.end();
    }

    std::array<float, 4> ClearColorFromPacket(const uint32_t argb, const bool srgbFormat) noexcept {
        std::array<float, 4> color {Channel(argb, RED_SHIFT), Channel(argb, GREEN_SHIFT), Channel(argb, 0), 1.0F};
        if (srgbFormat) {
            for (size_t channel = 0; channel < COLOUR_CHANNELS; ++channel) {
                color.at(channel) = DecodeSrgb(color.at(channel));
            }
        }
        return color;
    }
}  // namespace Liara::Renderer
