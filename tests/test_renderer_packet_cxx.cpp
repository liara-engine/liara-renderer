// NOLINTBEGIN(readability-identifier-naming)
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "packet/Packet.h"

#include <liara/extent.h>
#include <liara/mesh.h>
#include <liara/renderer/packet.h>
#include <liara/result.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

#include <doctest/doctest.h>

// The packet checks and the viewport conversion, tested without a GPU: Packet.cpp is compiled into this executable on
// its own.

using Liara::Renderer::PixelRect;
using Liara::Renderer::ToPixelRect;
using Liara::Renderer::ValidatePacket;

namespace
{
    constexpr liara_render_viewport_t FULL {.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F};

    constexpr liara_render_view_t View(const liara_render_target_id_t target,
                                       const liara_render_viewport_t viewport,
                                       const liara_render_drawable_t* drawables = nullptr,
                                       const size_t count = 0) {
        return {
            .target = target,
            .viewport = viewport,
            .clear_color = 0xFF1A2440U,
            .reserved = 0,
            .drawables = drawables,
            .drawable_count = count,
        };
    }

    constexpr liara_render_packet_t Packet(const liara_render_view_t* views, const size_t count) {
        return {.struct_version = LIARA_RENDER_PACKET_VERSION, .reserved = 0, .views = views, .view_count = count};
    }

    /** The code of a packet holding this one view. */
    liara_result_t Validate(const liara_render_view_t& view) { return ValidatePacket(Packet(&view, 1)); }
}  // namespace

TEST_CASE("ValidatePacket - a version it does not understand is refused before anything else is read") {
    for (const uint32_t version : {1U, 3U}) {
        const liara_render_packet_t packet {.struct_version = version,
                                            .reserved = 0,
                                            .views = nullptr,
                                            .view_count = 5};
        CHECK(ValidatePacket(packet) == LIARA_RESULT_VERSION_MISMATCH);
    }
}

TEST_CASE("ValidatePacket - a packet with no view, and one full-frame view with a drawable") {
    CHECK(ValidatePacket(Packet(nullptr, 0)) == LIARA_RESULT_SUCCESS);
    const liara_render_drawable_t drawable {.mesh = 1};
    CHECK(Validate(View(LIARA_RENDER_TARGET_WINDOW, FULL, &drawable, 1)) == LIARA_RESULT_SUCCESS);
}

TEST_CASE("ValidatePacket - views null with a count") {
    CHECK(ValidatePacket(Packet(nullptr, 1)) == LIARA_RESULT_NULL_POINTER);
}

TEST_CASE("ValidatePacket - a view targeting NONE is refused, an unknown target is not") {
    CHECK(Validate(View(LIARA_RENDER_TARGET_NONE, FULL)) == LIARA_RESULT_INVALID_ARGUMENT);
    CHECK(Validate(View(42, FULL)) == LIARA_RESULT_SUCCESS);
}

TEST_CASE("ValidatePacket - viewports outside the target") {
    constexpr float NOT_A_NUMBER = std::numeric_limits<float>::quiet_NaN();
    const std::array<liara_render_viewport_t, 6> refused {
        {{.x = NOT_A_NUMBER, .y = 0.0F, .width = 1.0F, .height = 1.0F},
         {.x = -0.1F, .y = 0.0F, .width = 1.0F, .height = 1.0F},
         {.x = 0.0F, .y = 0.0F, .width = 1.1F, .height = 1.0F},
         {.x = 0.6F, .y = 0.0F, .width = 0.5F, .height = 1.0F},
         {.x = 0.0F, .y = 0.6F, .width = 1.0F, .height = 0.5F},
         {.x = 0.0F, .y = 0.0F, .width = std::numeric_limits<float>::infinity(), .height = 1.0F}}
    };
    for (const liara_render_viewport_t& viewport : refused) {
        CHECK(Validate(View(LIARA_RENDER_TARGET_WINDOW, viewport)) == LIARA_RESULT_INVALID_ARGUMENT);
    }
}

TEST_CASE("ValidatePacket - viewports within the tolerance, and of zero width, are accepted") {
    CHECK(Validate(View(LIARA_RENDER_TARGET_WINDOW, {.x = 0.5F, .y = 0.0F, .width = 0.5F + 5e-6F, .height = 1.0F}))
          == LIARA_RESULT_SUCCESS);
    CHECK(Validate(View(LIARA_RENDER_TARGET_WINDOW, {.x = 0.0F, .y = 0.0F, .width = 0.0F, .height = 1.0F}))
          == LIARA_RESULT_SUCCESS);
}

TEST_CASE("ValidatePacket - drawables null with a count") {
    CHECK(Validate(View(LIARA_RENDER_TARGET_WINDOW, FULL, nullptr, 1)) == LIARA_RESULT_NULL_POINTER);
}

TEST_CASE("ValidatePacket - a drawable referencing NONE is refused, an unknown mesh is not") {
    const liara_render_drawable_t none {.mesh = LIARA_MESH_ID_NONE};
    CHECK(Validate(View(LIARA_RENDER_TARGET_WINDOW, FULL, &none, 1)) == LIARA_RESULT_INVALID_ARGUMENT);
    const liara_render_drawable_t unknown {.mesh = 99};
    CHECK(Validate(View(LIARA_RENDER_TARGET_WINDOW, FULL, &unknown, 1)) == LIARA_RESULT_SUCCESS);
}

TEST_CASE("ValidatePacket - the order of the checks") {
    // Within a view, the target comes before its drawables.
    CHECK(Validate(View(LIARA_RENDER_TARGET_NONE, FULL, nullptr, 1)) == LIARA_RESULT_INVALID_ARGUMENT);
    // Across views, the first failing one decides.
    const std::array views {View(LIARA_RENDER_TARGET_WINDOW, FULL, nullptr, 1), View(LIARA_RENDER_TARGET_NONE, FULL)};
    CHECK(ValidatePacket(Packet(views.data(), views.size())) == LIARA_RESULT_NULL_POINTER);
}

TEST_CASE("ToPixelRect - the whole target, half of it, and an inner rectangle") {
    constexpr liara_extent2d_t TARGET {.width = 1280, .height = 720};
    CHECK(ToPixelRect(FULL, TARGET) == PixelRect {.m_X = 0, .m_Y = 0, .m_Width = 1280, .m_Height = 720});
    CHECK(ToPixelRect({.x = 0.0F, .y = 0.0F, .width = 0.5F, .height = 1.0F}, TARGET)
          == PixelRect {.m_X = 0, .m_Y = 0, .m_Width = 640, .m_Height = 720});
    CHECK(ToPixelRect({.x = 0.5F, .y = 0.25F, .width = 0.5F, .height = 0.5F}, TARGET)
          == PixelRect {.m_X = 640, .m_Y = 180, .m_Width = 640, .m_Height = 360});
}

TEST_CASE("ToPixelRect - thirds share their edges, with neither gap nor overlap") {
    constexpr liara_extent2d_t TARGET {.width = 100, .height = 10};
    const PixelRect first = ToPixelRect({.x = 0.0F, .y = 0.0F, .width = 1.0F / 3.0F, .height = 1.0F}, TARGET);
    const PixelRect second = ToPixelRect({.x = 1.0F / 3.0F, .y = 0.0F, .width = 1.0F / 3.0F, .height = 1.0F}, TARGET);
    const PixelRect third = ToPixelRect({.x = 2.0F / 3.0F, .y = 0.0F, .width = 1.0F / 3.0F, .height = 1.0F}, TARGET);
    CHECK(first.m_X == 0);
    CHECK(first.m_X + first.m_Width == second.m_X);
    CHECK(second.m_X + second.m_Width == third.m_X);
    CHECK(third.m_X + third.m_Width == 100);
}

TEST_CASE("ToPixelRect - a zero width gives an empty rectangle") {
    CHECK(ToPixelRect({.x = 0.25F, .y = 0.0F, .width = 0.0F, .height = 1.0F}, {.width = 1280, .height = 720}).m_Width
          == 0);
}

TEST_CASE("ToPixelRect - a viewport within the tolerance past the edge ends at the edge") {
    const PixelRect rect =
        ToPixelRect({.x = 0.5F, .y = 0.0F, .width = 0.5F + 5e-6F, .height = 1.0F}, {.width = 1280, .height = 720});
    CHECK(rect.m_X + rect.m_Width == 1280);
}

// NOLINTEND(readability-identifier-naming)
