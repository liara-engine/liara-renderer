// NOLINTBEGIN(readability-identifier-naming)
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <liara/modules.h>
#include <liara/renderer/packet.h>
#include <liara/renderer/renderer.h>
#include <liara/result.h>

#include <cstdint>
#include <string_view>

#include <doctest/doctest.h>

// Everything here runs without creating a renderer instance, so without a GPU, and runs in CI. What needs an instance
// is in test_renderer_device_cxx.cpp, under the `gpu` label.

TEST_CASE("liara_renderer_abi_version") {
    const uint32_t version = liara_renderer_abi_version();
    CHECK(version > 0);
}

TEST_CASE("liara_renderer_info") {
    const liara_module_info_t* info = liara_renderer_info();
    REQUIRE(info != nullptr);
    CHECK(std::string_view(info->module_name) == "Renderer");
}

TEST_CASE("liara_renderer_create - null pointer") {
    const liara_result_t result = liara_renderer_create(nullptr);
    CHECK(result == LIARA_RESULT_NULL_POINTER);
}

TEST_CASE("liara_renderer_destroy - a null handle is a no-op") { liara_renderer_destroy(nullptr); }

TEST_CASE("liara_renderer_submit_frame - null renderer") {
    const liara_render_drawable_t drawable {.x = 0.0F, .y = 0.0F, .color = 0xFFFFFFFFU};
    const liara_render_packet_t packet {
        .struct_version = LIARA_RENDER_PACKET_VERSION,
        .grid_width = 4,
        .grid_height = 4,
        .background_color = 0xFF000000U,
        .drawables = &drawable,
        .drawable_count = 1,
    };

    const liara_result_t result = liara_renderer_submit_frame(nullptr, &packet);
    CHECK(result == LIARA_RESULT_NULL_POINTER);
}

// NOLINTEND(readability-identifier-naming)
