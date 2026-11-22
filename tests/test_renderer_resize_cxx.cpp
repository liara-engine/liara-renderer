// NOLINTBEGIN(readability-identifier-naming)
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "SinkCapture.h"

#include "device/Loader.h"
#include "renderer/CreateInfo.h"
#include "renderer/Renderer.h"

#include <liara/extent.h>
#include <liara/mesh.h>
#include <liara/native_window.h>
#include <liara/renderer/packet.h>
#include <liara/result.h>

#include <vulkan/vulkan_raii.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

// <X11/Xlib.h>, which Vulkan-Hpp includes for the Xlib surface (VK_USE_PLATFORM_XLIB_KHR, defined for every file that
// sees Vulkan-Hpp in this module), defines None as a macro, and doctest names an enumerator None. Nothing below uses
// Xlib's.
#ifdef None
    #undef None
#endif

#include <doctest/doctest.h>

// The swapchain's recreation, on a VK_EXT_headless_surface surface. Such a surface has no size of its own, as a Wayland
// one has none, so the swapchain's extent comes from the size pushed through Renderer::WindowResized, and the whole
// path a real window runs (the device's presentation criterion, the swapchain, the pipeline, the frames) runs here with
// no display. It needs a GPU, so `gpu`; it is skipped (exit code 77) where the loader does not offer the extension.
//
// Every case captures the sink to count the swapchain lines, so it checks the capture for warnings and errors itself:
// what is captured never reaches CTest's failure expression.

using Liara::Renderer::Loader;
using Liara::Renderer::Renderer;
using Liara::Renderer::RendererConfig;

namespace
{
    constexpr int FRAMES = 10;

    constexpr liara_mesh_id_t TRIANGLE = 1;
    constexpr std::array<float, 9> TRIANGLE_POSITIONS {0.0F, 0.5F, 0.0F, 0.5F, -0.5F, 0.0F, -0.5F, -0.5F, 0.0F};
    constexpr std::array<uint32_t, 3> TRIANGLE_COLORS {0xFFFF0000U, 0xFF00FF00U, 0xFF0000FFU};

    constexpr liara_render_drawable_t TRIANGLE_DRAWABLE {.mesh = TRIANGLE};
    constexpr liara_render_view_t TRIANGLE_VIEW {
        .target = LIARA_RENDER_TARGET_WINDOW,
        .viewport = {.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F},
        .clear_color = 0xFF1A2440U,
        .reserved = 0,
        .drawables = &TRIANGLE_DRAWABLE,
        .drawable_count = 1,
    };

    /** A headless surface of `width` by `height` pixels, as the create-info would describe a window of that size. */
    RendererConfig HeadlessConfig(const uint32_t width, const uint32_t height) {
        RendererConfig config;
        config.m_Window.kind = LIARA_NATIVE_WINDOW_KIND_NONE;
        config.m_PixelSize = {.width = width, .height = height};
        config.m_HeadlessSurface = true;
        return config;
    }

    /** The core's packet once it draws the triangle: one full-frame view on the window, one drawable. */
    liara_render_packet_t TrianglePacket() {
        return {
            .struct_version = LIARA_RENDER_PACKET_VERSION,
            .reserved = 0,
            .views = &TRIANGLE_VIEW,
            .view_count = 1,
        };
    }

    void UploadTriangle(Renderer& renderer) {
        const liara_mesh_data_t data {
            .struct_version = LIARA_MESH_DATA_VERSION,
            .reserved = 0,
            .positions = TRIANGLE_POSITIONS.data(),
            .colors = TRIANGLE_COLORS.data(),
            .vertex_count = 3,
            .indices = nullptr,
            .index_count = 0,
        };
        // Thrown rather than CHECKed: doctest reports the exception as the case's failure, and the static analyzer does
        // not follow doctest's expression templates out of a helper (as in test_renderer_draw_cxx.cpp).
        if (renderer.UploadMesh(TRIANGLE, data) != LIARA_RESULT_SUCCESS) {
            throw std::runtime_error("the triangle could not be uploaded");
        }
    }

    std::vector<liara_result_t> SubmitFrames(Renderer& renderer, const int count) {
        const liara_render_packet_t packet = TrianglePacket();
        std::vector<liara_result_t> results;
        results.reserve(static_cast<size_t>(count));
        for (int frame = 0; frame < count; ++frame) { results.push_back(renderer.SubmitFrame(packet)); }
        return results;
    }

    bool AllAre(const std::vector<liara_result_t>& results, const liara_result_t expected) {
        return std::ranges::all_of(results, [expected](const liara_result_t result) { return result == expected; });
    }

    size_t CountOf(const std::string_view text, const std::string_view needle) {
        size_t count = 0;
        for (size_t at = text.find(needle); at != std::string_view::npos; at = text.find(needle, at + needle.size())) {
            ++count;
        }
        return count;
    }

    void CheckNoWarningOrError(const std::string_view captured) {
        const bool noWarning = CountOf(captured, "] warning: ") == 0;
        const bool noError = CountOf(captured, "] error: ") == 0;
        CHECK(noWarning);
        CHECK(noError);
    }

    /** @brief Whether the loader offers VK_EXT_headless_surface; says so on stdout when it does not. */
    bool HeadlessSurfaceOffered() {
        const Loader loader;
        const std::vector<vk::ExtensionProperties> extensions =
            loader.GetContext().enumerateInstanceExtensionProperties();
        const bool offered = std::ranges::any_of(extensions, [](const vk::ExtensionProperties& extension) {
            return std::string_view(extension.extensionName.data()) == VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME;
        });
        if (!offered) {
            std::cout << "The Vulkan loader does not offer " VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME ": skipped.\n";
        }
        return offered;
    }

    /**
     * @brief Everything the sink wrote while a renderer on a 64 by 64 headless surface, with the triangle uploaded,
     * went through `steps`, then released the triangle and was destroyed.
     */
    template<typename Steps>
    std::string RunAndCapture(Steps steps) {
        std::string written;
        {
            const SinkCapture capture;
            {
                Renderer renderer(HeadlessConfig(64, 64));
                UploadTriangle(renderer);
                steps(renderer);
                if (renderer.ReleaseMesh(TRIANGLE) != LIARA_RESULT_SUCCESS) {
                    throw std::runtime_error("the triangle could not be released");
                }
            }
            written = capture.Written();
        }
        return written;
    }

    constexpr std::string_view RECREATED = "info: swapchain recreated ";
    constexpr std::string_view NO_AREA = "info: window has no area; presentation paused";
}  // namespace

TEST_CASE("Resize - created at 64x64, frames present without recreating") {
    if (!HeadlessSurfaceOffered()) { return; }
    bool presented = false;
    const std::string written = RunAndCapture(
        [&](Renderer& renderer) { presented = AllAre(SubmitFrames(renderer, FRAMES), LIARA_RESULT_SUCCESS); });
    const bool createdOnce = CountOf(written, "info: swapchain 64x64, ") == 1;
    const bool neverRecreated = CountOf(written, RECREATED) == 0;
    CHECK(presented);
    CHECK(createdOnce);
    CHECK(neverRecreated);
    CheckNoWarningOrError(written);
}

TEST_CASE("Resize - a pushed size recreates once, at that size") {
    if (!HeadlessSurfaceOffered()) { return; }
    bool presented = false;
    const std::string written = RunAndCapture([&](Renderer& renderer) {
        renderer.WindowResized({.width = 128, .height = 96});
        presented = AllAre(SubmitFrames(renderer, FRAMES), LIARA_RESULT_SUCCESS);
    });
    const bool recreatedOnce = CountOf(written, RECREATED) == 1;
    const bool atThatSize = CountOf(written, "swapchain recreated 128x96, ") == 1;
    CHECK(presented);
    CHECK(recreatedOnce);
    CHECK(atThatSize);
    CheckNoWarningOrError(written);
}

TEST_CASE("Resize - several pushes cost one recreation") {
    if (!HeadlessSurfaceOffered()) { return; }
    bool presented = false;
    const std::string written = RunAndCapture([&](Renderer& renderer) {
        renderer.WindowResized({.width = 100, .height = 100});
        renderer.WindowResized({.width = 50, .height = 50});
        renderer.WindowResized({.width = 80, .height = 60});
        presented = AllAre(SubmitFrames(renderer, FRAMES), LIARA_RESULT_SUCCESS);
    });
    const bool recreatedOnce = CountOf(written, RECREATED) == 1;
    const bool atTheLast = CountOf(written, "swapchain recreated 80x60, ") == 1;
    const bool notAtTheOthers =
        CountOf(written, "swapchain recreated 100x100") == 0 && CountOf(written, "swapchain recreated 50x50") == 0;
    CHECK(presented);
    CHECK(recreatedOnce);
    CHECK(atTheLast);
    CHECK(notAtTheOthers);
    CheckNoWarningOrError(written);
}

TEST_CASE("Resize - the current size pushed again recreates nothing") {
    if (!HeadlessSurfaceOffered()) { return; }
    bool presented = false;
    const std::string written = RunAndCapture([&](Renderer& renderer) {
        renderer.WindowResized({.width = 64, .height = 64});
        presented = AllAre(SubmitFrames(renderer, FRAMES), LIARA_RESULT_SUCCESS);
    });
    const bool neverRecreated = CountOf(written, RECREATED) == 0;
    CHECK(presented);
    CHECK(neverRecreated);
    CheckNoWarningOrError(written);
}

TEST_CASE("Resize - no area pauses, and a later size resumes") {
    if (!HeadlessSurfaceOffered()) { return; }
    bool paused = false;
    bool resumed = false;
    const std::string written = RunAndCapture([&](Renderer& renderer) {
        renderer.WindowResized({.width = 0, .height = 0});
        paused = AllAre(SubmitFrames(renderer, FRAMES), LIARA_RESULT_SURFACE_OUT_OF_DATE);
        renderer.WindowResized({.width = 96, .height = 64});
        resumed = AllAre(SubmitFrames(renderer, FRAMES), LIARA_RESULT_SUCCESS);
    });
    const bool saidOnce = CountOf(written, NO_AREA) == 1;
    const bool recreatedOnceAtTheNewSize =
        CountOf(written, RECREATED) == 1 && CountOf(written, "swapchain recreated 96x64, ") == 1;
    CHECK(paused);
    CHECK(resumed);
    CHECK(saidOnce);
    CHECK(recreatedOnceAtTheNewSize);
    CheckNoWarningOrError(written);
}

TEST_CASE("Resize - one zero component is no area too") {
    if (!HeadlessSurfaceOffered()) { return; }
    bool paused = false;
    const std::string written = RunAndCapture([&](Renderer& renderer) {
        renderer.WindowResized({.width = 0, .height = 64});
        paused = AllAre(SubmitFrames(renderer, 1), LIARA_RESULT_SURFACE_OUT_OF_DATE);
    });
    const bool saidOnce = CountOf(written, NO_AREA) == 1;
    CHECK(paused);
    CHECK(saidOnce);
    CheckNoWarningOrError(written);
}

TEST_CASE("Resize - a renderer without a swapchain releases and shuts down cleanly") {
    if (!HeadlessSurfaceOffered()) { return; }
    bool paused = false;
    // RunAndCapture releases the triangle and destroys the renderer after the steps, here with no swapchain left.
    const std::string written = RunAndCapture([&](Renderer& renderer) {
        renderer.WindowResized({.width = 0, .height = 0});
        paused = AllAre(SubmitFrames(renderer, 1), LIARA_RESULT_SURFACE_OUT_OF_DATE);
    });
    CHECK(paused);
    CheckNoWarningOrError(written);
}

// NOLINTEND(readability-identifier-naming)
