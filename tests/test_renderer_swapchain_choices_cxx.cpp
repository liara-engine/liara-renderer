// NOLINTBEGIN(readability-identifier-naming)
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "swapchain/SwapchainChoices.h"

#include <array>
#include <cstdint>

#include <doctest/doctest.h>

// The swapchain's choices, tested without a GPU: SwapchainChoices.cpp is compiled into this executable on its own, and
// the surface's capabilities are made up.

using Liara::Renderer::ChooseCompositeAlpha;
using Liara::Renderer::ChooseExtent;
using Liara::Renderer::ChooseImageCount;
using Liara::Renderer::ChooseSurfaceFormat;
using Liara::Renderer::ClearColorFromPacket;
using Liara::Renderer::IsSrgbFormat;

namespace
{
    constexpr uint32_t NO_SIZE_OF_ITS_OWN = 0xFFFFFFFFU;

    vk::SurfaceCapabilitiesKHR Capabilities(const vk::Extent2D current) {
        vk::SurfaceCapabilitiesKHR capabilities;
        capabilities.currentExtent = current;
        capabilities.minImageExtent = vk::Extent2D {1, 1};
        capabilities.maxImageExtent = vk::Extent2D {4096, 4096};
        capabilities.minImageCount = 2;
        capabilities.maxImageCount = 0;
        return capabilities;
    }

    constexpr vk::SurfaceFormatKHR Format(const vk::Format format,
                                          const vk::ColorSpaceKHR space = vk::ColorSpaceKHR::eSrgbNonlinear) {
        return {format, space};
    }
}  // namespace

TEST_CASE("ChooseExtent - a surface without a size of its own takes the window's, clamped") {
    const auto capabilities = Capabilities({NO_SIZE_OF_ITS_OWN, NO_SIZE_OF_ITS_OWN});
    CHECK(ChooseExtent(capabilities, {.width = 1280, .height = 720}) == vk::Extent2D {1280, 720});
    CHECK(ChooseExtent(capabilities, {.width = 8000, .height = 720}) == vk::Extent2D {4096, 720});
}

TEST_CASE("ChooseExtent - a surface with a size of its own keeps it, zero included") {
    CHECK(ChooseExtent(Capabilities({800, 600}), {.width = 1280, .height = 720}) == vk::Extent2D {800, 600});
    CHECK(ChooseExtent(Capabilities({0, 0}), {.width = 1280, .height = 720}) == vk::Extent2D {0, 0});
}

TEST_CASE("ChooseSurfaceFormat - BGRA sRGB, then RGBA sRGB, then the first reported") {
    const std::array all {Format(vk::Format::eB8G8R8A8Unorm),
                          Format(vk::Format::eR8G8B8A8Srgb),
                          Format(vk::Format::eB8G8R8A8Srgb)};
    CHECK(ChooseSurfaceFormat(all) == Format(vk::Format::eB8G8R8A8Srgb));

    const std::array noBgra {Format(vk::Format::eB8G8R8A8Unorm), Format(vk::Format::eR8G8B8A8Srgb)};
    CHECK(ChooseSurfaceFormat(noBgra) == Format(vk::Format::eR8G8B8A8Srgb));

    const std::array wrongSpace {Format(vk::Format::eA2B10G10R10UnormPack32),
                                 Format(vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eExtendedSrgbLinearEXT)};
    CHECK(ChooseSurfaceFormat(wrongSpace) == Format(vk::Format::eA2B10G10R10UnormPack32));
}

TEST_CASE("ChooseImageCount - one more than the minimum, capped by a non-zero maximum") {
    auto capabilities = Capabilities({800, 600});
    CHECK(ChooseImageCount(capabilities) == 3U);
    capabilities.maxImageCount = 2;
    CHECK(ChooseImageCount(capabilities) == 2U);
}

TEST_CASE("ChooseCompositeAlpha - opaque when supported, otherwise the lowest supported bit") {
    using Alpha = vk::CompositeAlphaFlagBitsKHR;
    CHECK(ChooseCompositeAlpha(Alpha::eOpaque | Alpha::eInherit) == Alpha::eOpaque);
    CHECK(ChooseCompositeAlpha(Alpha::eInherit | Alpha::ePreMultiplied) == Alpha::ePreMultiplied);
}

TEST_CASE("IsSrgbFormat") {
    CHECK(IsSrgbFormat(vk::Format::eB8G8R8A8Srgb));
    CHECK(IsSrgbFormat(vk::Format::eR8G8B8A8Srgb));
    CHECK_FALSE(IsSrgbFormat(vk::Format::eB8G8R8A8Unorm));
}

TEST_CASE("ClearColorFromPacket - decoded from sRGB for an sRGB format, as is otherwise, alpha ignored") {
    const auto srgb = ClearColorFromPacket(0xFF0080FFU, true);
    CHECK(srgb.at(0) == doctest::Approx(0.0F));
    CHECK(srgb.at(1) == doctest::Approx(0.2158605F).epsilon(1e-5));
    CHECK(srgb.at(2) == doctest::Approx(1.0F));
    CHECK(srgb.at(3) == doctest::Approx(1.0F));

    const auto unorm = ClearColorFromPacket(0x000080FFU, false);
    CHECK(unorm.at(1) == doctest::Approx(128.0F / 255.0F));
    CHECK(unorm.at(3) == doctest::Approx(1.0F));
}

// NOLINTEND(readability-identifier-naming)
