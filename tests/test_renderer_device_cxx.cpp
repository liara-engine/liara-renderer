// NOLINTBEGIN(readability-identifier-naming)
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "TestCreateInfo.h"

#include <liara/renderer/packet.h>
#include <liara/renderer/renderer.h>
#include <liara/result.h>

#include <cstddef>
#include <cstdint>

#include <doctest/doctest.h>

// Every test here creates a renderer instance, which creates a Vulkan instance and device, so the file carries the
// `gpu` label and CI does not run it. CTest also fails it on any `[liara-renderer] warning:` or `error:` line in its
// output (FAIL_REGULAR_EXPRESSION in tests/CMakeLists.txt), which is how a validation message fails the run even when
// every CHECK passed. In a Release build the validation layer is off, and these tests only check that creation works.

namespace
{
    constexpr liara_render_packet_t Packet(const uint16_t width,
                                           const uint16_t height,
                                           const liara_render_drawable_t* drawables,
                                           const size_t count) {
        return {
            .struct_version = LIARA_RENDER_PACKET_VERSION,
            .grid_width = width,
            .grid_height = height,
            .background_color = 0xFF000000U,
            .drawables = drawables,
            .drawable_count = count,
        };
    }
}  // namespace

TEST_CASE("liara_renderer_create - three times in a row") {
    for (int i = 0; i < 3; ++i) {
        liara_renderer_handle_t* renderer = nullptr;
        REQUIRE(liara_renderer_create(&NO_WINDOW, &renderer) == LIARA_RESULT_SUCCESS);
        REQUIRE(renderer != nullptr);
        liara_renderer_destroy(renderer);
    }
}

TEST_CASE("liara_renderer_create - two renderers alive at once") {
    liara_renderer_handle_t* first = nullptr;
    liara_renderer_handle_t* second = nullptr;
    REQUIRE(liara_renderer_create(&NO_WINDOW, &first) == LIARA_RESULT_SUCCESS);
    REQUIRE(liara_renderer_create(&NO_WINDOW, &second) == LIARA_RESULT_SUCCESS);
    CHECK(first != second);
    liara_renderer_destroy(first);
    liara_renderer_destroy(second);
}

TEST_CASE("liara_renderer_submit_frame - forwards the console renderer's verdicts") {
    liara_renderer_handle_t* renderer = nullptr;
    REQUIRE(liara_renderer_create(&NO_WINDOW, &renderer) == LIARA_RESULT_SUCCESS);

    CHECK(liara_renderer_submit_frame(renderer, nullptr) == LIARA_RESULT_NULL_POINTER);

    const liara_render_packet_t zeroWidth = Packet(0, 4, nullptr, 0);
    CHECK(liara_renderer_submit_frame(renderer, &zeroWidth) == LIARA_RESULT_INVALID_ARGUMENT);

    const liara_render_packet_t nullDrawables = Packet(4, 4, nullptr, 1);
    CHECK(liara_renderer_submit_frame(renderer, &nullDrawables) == LIARA_RESULT_NULL_POINTER);

    liara_render_drawable_t drawable {.x = 1.0F, .y = 1.0F, .color = 0xFFFF0000U};
    const liara_render_packet_t valid = Packet(4, 4, &drawable, 1);
    CHECK(liara_renderer_submit_frame(renderer, &valid) == LIARA_RESULT_SUCCESS);
    drawable.x = 2.0F;
    CHECK(liara_renderer_submit_frame(renderer, &valid) == LIARA_RESULT_SUCCESS);

    liara_renderer_destroy(renderer);
}

// NOLINTEND(readability-identifier-naming)
