// NOLINTBEGIN(readability-identifier-naming)
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "liara/renderer/packet.h"

#include <liara/renderer/renderer.h>
#include <liara/result.h>

#include <cstdint>

#include <doctest/doctest.h>

TEST_CASE("liara_renderer_abi_version") {
    const uint32_t version = liara_renderer_abi_version();
    CHECK(version > 0);
}

TEST_CASE("liara_renderer_create - success") {
    liara_renderer_handle_t* renderer = nullptr;
    const liara_result_t result = liara_renderer_create(&renderer);

    CHECK(result == LIARA_RESULT_SUCCESS);
    CHECK(renderer != nullptr);

    liara_renderer_destroy(renderer);
}

TEST_CASE("liara_renderer_create - null pointer") {
    const liara_result_t result = liara_renderer_create(nullptr);
    CHECK(result == LIARA_RESULT_NULL_POINTER);
}

TEST_CASE("liara_renderer_destroy - success") {
    liara_renderer_handle_t* renderer = nullptr;
    liara_renderer_create(&renderer);

    const liara_result_t result = liara_renderer_destroy(renderer);
    CHECK(result == LIARA_RESULT_SUCCESS);
}

TEST_CASE("liara_renderer_destroy - null pointer") {
    const liara_result_t result = liara_renderer_destroy(nullptr);
    CHECK(result == LIARA_RESULT_NULL_POINTER);
}

TEST_CASE("liara_renderer_destroy - invalid state") {
    liara_renderer_handle_t* renderer = nullptr;
    liara_renderer_create(&renderer);
    liara_renderer_destroy(renderer);

    const liara_result_t result = liara_renderer_destroy(renderer);
    CHECK(result == LIARA_RESULT_INVALID_STATE);
}

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

TEST_CASE("liara_renderer_submit_frame - null packet") {
    liara_renderer_handle_t* renderer = nullptr;
    liara_renderer_create(&renderer);

    const liara_result_t result = liara_renderer_submit_frame(renderer, nullptr);
    CHECK(result == LIARA_RESULT_NULL_POINTER);

    liara_renderer_destroy(renderer);
}

TEST_CASE("liara_renderer_submit_frame - null drawables with nonzero count") {
    liara_renderer_handle_t* renderer = nullptr;
    liara_renderer_create(&renderer);

    const liara_render_packet_t packet {
        .struct_version = LIARA_RENDER_PACKET_VERSION,
        .grid_width = 4,
        .grid_height = 4,
        .background_color = 0xFF000000U,
        .drawables = nullptr,
        .drawable_count = 1,
    };

    const liara_result_t result = liara_renderer_submit_frame(renderer, &packet);
    CHECK(result == LIARA_RESULT_NULL_POINTER);

    liara_renderer_destroy(renderer);
}

TEST_CASE("liara_renderer_submit_frame - zero grid dimensions") {
    liara_renderer_handle_t* renderer = nullptr;
    liara_renderer_create(&renderer);

    const liara_render_packet_t zeroWidth {
        .struct_version = LIARA_RENDER_PACKET_VERSION,
        .grid_width = 0,
        .grid_height = 4,
        .background_color = 0xFF000000U,
        .drawables = nullptr,
        .drawable_count = 0,
    };
    CHECK(liara_renderer_submit_frame(renderer, &zeroWidth) == LIARA_RESULT_INVALID_ARGUMENT);

    const liara_render_packet_t zeroHeight {
        .struct_version = LIARA_RENDER_PACKET_VERSION,
        .grid_width = 4,
        .grid_height = 0,
        .background_color = 0xFF000000U,
        .drawables = nullptr,
        .drawable_count = 0,
    };
    CHECK(liara_renderer_submit_frame(renderer, &zeroHeight) == LIARA_RESULT_INVALID_ARGUMENT);

    liara_renderer_destroy(renderer);
}

TEST_CASE("liara_renderer_submit_frame - empty drawable list") {
    liara_renderer_handle_t* renderer = nullptr;
    liara_renderer_create(&renderer);

    const liara_render_packet_t packet {
        .struct_version = LIARA_RENDER_PACKET_VERSION,
        .grid_width = 4,
        .grid_height = 4,
        .background_color = 0xFF000000U,
        .drawables = nullptr,
        .drawable_count = 0,
    };

    CHECK(liara_renderer_submit_frame(renderer, &packet) == LIARA_RESULT_SUCCESS);

    liara_renderer_destroy(renderer);
}

TEST_CASE("liara_renderer_submit_frame - valid single drawable, submitted twice") {
    liara_renderer_handle_t* renderer = nullptr;
    liara_renderer_create(&renderer);

    liara_render_drawable_t drawable {.x = 1.0F, .y = 1.0F, .color = 0xFFFF0000U};
    const liara_render_packet_t packet {
        .struct_version = LIARA_RENDER_PACKET_VERSION,
        .grid_width = 4,
        .grid_height = 4,
        .background_color = 0xFF000000U,
        .drawables = &drawable,
        .drawable_count = 1,
    };

    CHECK(liara_renderer_submit_frame(renderer, &packet) == LIARA_RESULT_SUCCESS);

    // Move the drawable and resubmit, exercising the frame-diffing path (old cell cleared, new cell painted).
    drawable.x = 2.0F;
    drawable.y = 2.0F;
    CHECK(liara_renderer_submit_frame(renderer, &packet) == LIARA_RESULT_SUCCESS);

    liara_renderer_destroy(renderer);
}

TEST_CASE("liara_renderer_submit_frame - out-of-bounds drawable is clipped, not a crash") {
    liara_renderer_handle_t* renderer = nullptr;
    liara_renderer_create(&renderer);

    const liara_render_drawable_t drawable {.x = 100.0F, .y = -5.0F, .color = 0xFFFFFFFFU};
    const liara_render_packet_t packet {
        .struct_version = LIARA_RENDER_PACKET_VERSION,
        .grid_width = 4,
        .grid_height = 4,
        .background_color = 0xFF000000U,
        .drawables = &drawable,
        .drawable_count = 1,
    };

    CHECK(liara_renderer_submit_frame(renderer, &packet) == LIARA_RESULT_SUCCESS);

    liara_renderer_destroy(renderer);
}

TEST_CASE("liara_renderer_submit_frame - invalid state after destroy") {
    liara_renderer_handle_t* renderer = nullptr;
    liara_renderer_create(&renderer);
    liara_renderer_destroy(renderer);

    const liara_render_packet_t packet {
        .struct_version = LIARA_RENDER_PACKET_VERSION,
        .grid_width = 4,
        .grid_height = 4,
        .background_color = 0xFF000000U,
        .drawables = nullptr,
        .drawable_count = 0,
    };

    CHECK(liara_renderer_submit_frame(renderer, &packet) == LIARA_RESULT_INVALID_STATE);
}

// NOLINTEND(readability-identifier-naming)
