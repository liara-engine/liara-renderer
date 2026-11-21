// NOLINTBEGIN(readability-identifier-naming)
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "TestCreateInfo.h"

#include <liara/renderer/packet.h>
#include <liara/renderer/renderer.h>
#include <liara/result.h>

#include <string_view>

#include <doctest/doctest.h>

// Every test here creates a renderer instance, which creates a Vulkan instance and device, so the file carries the
// `gpu` label and CI does not run it. CTest also fails it on any `[liara-renderer] warning:` or `error:` line in its
// output (FAIL_REGULAR_EXPRESSION in tests/CMakeLists.txt), which is how a validation message fails the run even when
// every CHECK passed. In a Release build the validation layer is off, and these tests only check that creation works.

TEST_CASE("liara_renderer_create - three times in a row") {
    for (int i = 0; i < 3; ++i) {
        liara_renderer_handle_t* renderer = nullptr;
        REQUIRE(liara_renderer_create(&NO_WINDOW, &renderer) == LIARA_RESULT_SUCCESS);
        REQUIRE_FALSE(renderer == nullptr);
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

TEST_CASE("liara_renderer_submit_frame - checks the packet, then draws nothing without a window") {
    liara_renderer_handle_t* renderer = nullptr;
    REQUIRE(liara_renderer_create(&NO_WINDOW, &renderer) == LIARA_RESULT_SUCCESS);

    const liara_render_drawable_t drawable {.mesh = 1};
    liara_render_view_t view {
        .target = LIARA_RENDER_TARGET_WINDOW,
        .viewport = {.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F},
        .clear_color = 0xFF1A2440U,
        .reserved = 0,
        .drawables = &drawable,
        .drawable_count = 1,
    };
    liara_render_packet_t packet {
        .struct_version = LIARA_RENDER_PACKET_VERSION,
        .reserved = 0,
        .views = &view,
        .view_count = 1,
    };

    CHECK(liara_renderer_submit_frame(renderer, nullptr) == LIARA_RESULT_NULL_POINTER);
    CHECK(liara_renderer_submit_frame(renderer, &packet) == LIARA_RESULT_SUCCESS);
    view.target = LIARA_RENDER_TARGET_NONE;
    CHECK(liara_renderer_submit_frame(renderer, &packet) == LIARA_RESULT_INVALID_ARGUMENT);
    packet.struct_version = 1;
    CHECK(liara_renderer_submit_frame(renderer, &packet) == LIARA_RESULT_VERSION_MISMATCH);

    liara_renderer_destroy(renderer);
}

// Registered on its own (tests/CMakeLists.txt), since it expects the warning the main registration fails on.
TEST_CASE("liara_renderer_create - a preferred GPU no device matches") {
    liara_renderer_create_info_t info = NoWindowCreateInfo();
    constexpr std::string_view NAME = "liara-no-such-gpu";
    info.preferred_device_name = NAME.data();
    info.preferred_device_name_length = NAME.size();
    liara_renderer_handle_t* renderer = nullptr;
    REQUIRE(liara_renderer_create(&info, &renderer) == LIARA_RESULT_SUCCESS);
    liara_renderer_destroy(renderer);
}

// NOLINTEND(readability-identifier-naming)
