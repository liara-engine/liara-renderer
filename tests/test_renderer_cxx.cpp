// NOLINTBEGIN(readability-identifier-naming)
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "TestCreateInfo.h"

#include <liara/extent.h>
#include <liara/modules.h>
#include <liara/native_window.h>
#include <liara/renderer/packet.h>
#include <liara/renderer/renderer.h>
#include <liara/result.h>

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string_view>

#include <doctest/doctest.h>

// Everything here runs without creating a renderer instance, so without a GPU, and runs in CI. What needs an instance
// is in test_renderer_device_cxx.cpp, under the `gpu` label.
//
// The create-info cases also prove that an argument failure never reaches Vulkan: CTest fails this test on any line the
// renderer logs (FAIL_REGULAR_EXPRESSION in tests/CMakeLists.txt), and opening the loader always logs one.

namespace
{
    /** Any address the call has no reason to write: it must come back unchanged. */
    liara_renderer_handle_t* Sentinel() {
        static std::max_align_t storage {};
        return reinterpret_cast<liara_renderer_handle_t*>(&storage);
    }

    /** Stands in for a window-system object. The checks under test return before anything dereferences it. */
    void* Dummy() {
        static std::max_align_t storage {};
        return &storage;
    }

    constexpr liara_extent2d_t WINDOW_SIZE {.width = 1280, .height = 720};

    /** A window of a kind this build can present to, with every pointer it needs set. */
    liara_renderer_create_info_t PresentableWindowCreateInfo() {
        liara_renderer_create_info_t info = NoWindowCreateInfo();
        info.pixel_size = WINDOW_SIZE;
#ifdef _WIN32
        info.native_window.kind = LIARA_NATIVE_WINDOW_KIND_WIN32;
        info.native_window.handle.win32.hinstance = Dummy();
        info.native_window.handle.win32.hwnd = Dummy();
#else
        info.native_window.kind = LIARA_NATIVE_WINDOW_KIND_WAYLAND;
        info.native_window.handle.wayland.display = Dummy();
        info.native_window.handle.wayland.surface = Dummy();
#endif
        return info;
    }

    /** Creation must fail with `expected` and leave the output untouched. */
    void CheckRefused(const liara_renderer_create_info_t& info, const liara_result_t expected) {
        liara_renderer_handle_t* renderer = Sentinel();
        CHECK(liara_renderer_create(&info, &renderer) == expected);
        CHECK(renderer == Sentinel());
    }
}  // namespace

TEST_CASE("liara_renderer_abi_version") {
    const uint32_t version = liara_renderer_abi_version();
    CHECK(version > 0);
}

TEST_CASE("liara_renderer_info") {
    const liara_module_info_t* info = liara_renderer_info();
    REQUIRE(info != nullptr);
    CHECK(std::string_view(info->module_name) == "Renderer");
}

TEST_CASE("liara_renderer_create - null arguments") {
    liara_renderer_handle_t* renderer = Sentinel();
    CHECK(liara_renderer_create(nullptr, &renderer) == LIARA_RESULT_NULL_POINTER);
    CHECK(renderer == Sentinel());
    const liara_renderer_create_info_t info = NoWindowCreateInfo();
    CHECK(liara_renderer_create(&info, nullptr) == LIARA_RESULT_NULL_POINTER);
}

TEST_CASE("liara_renderer_create - struct versions it does not understand") {
    for (const uint32_t version : {0U, 2U}) {
        liara_renderer_create_info_t info = NoWindowCreateInfo();
        info.struct_version = version;
        CheckRefused(info, LIARA_RESULT_VERSION_MISMATCH);
    }
}

TEST_CASE("liara_renderer_create - a null device name with a length") {
    liara_renderer_create_info_t info = NoWindowCreateInfo();
    info.preferred_device_name = nullptr;
    info.preferred_device_name_length = 3;
    CheckRefused(info, LIARA_RESULT_NULL_POINTER);
}

TEST_CASE("liara_renderer_create - an unknown window kind") {
    liara_renderer_create_info_t info = PresentableWindowCreateInfo();
    info.native_window.kind = 99;
    CheckRefused(info, LIARA_RESULT_NOT_SUPPORTED);
}

TEST_CASE("liara_renderer_create - a window kind foreign to this platform") {
    liara_renderer_create_info_t info = PresentableWindowCreateInfo();
#ifdef _WIN32
    info.native_window.kind = LIARA_NATIVE_WINDOW_KIND_XLIB;
    info.native_window.handle.xlib.dpy = Dummy();
    info.native_window.handle.xlib.window = 1;
#else
    info.native_window.kind = LIARA_NATIVE_WINDOW_KIND_WIN32;
    info.native_window.handle.win32.hinstance = Dummy();
    info.native_window.handle.win32.hwnd = Dummy();
#endif
    CheckRefused(info, LIARA_RESULT_NOT_SUPPORTED);
}

TEST_CASE("liara_renderer_create - a window missing a pointer its kind needs") {
#ifdef _WIN32
    liara_renderer_create_info_t info = PresentableWindowCreateInfo();
    info.native_window.handle.win32.hwnd = nullptr;
    CheckRefused(info, LIARA_RESULT_NULL_POINTER);
#else
    liara_renderer_create_info_t wayland = PresentableWindowCreateInfo();
    wayland.native_window.handle.wayland.surface = nullptr;
    CheckRefused(wayland, LIARA_RESULT_NULL_POINTER);

    liara_renderer_create_info_t xlib = PresentableWindowCreateInfo();
    xlib.native_window.kind = LIARA_NATIVE_WINDOW_KIND_XLIB;
    xlib.native_window.handle.xlib.dpy = nullptr;
    xlib.native_window.handle.xlib.window = 1;
    CheckRefused(xlib, LIARA_RESULT_NULL_POINTER);
#endif
}

TEST_CASE("liara_renderer_create - a window with no area") {
    liara_renderer_create_info_t info = PresentableWindowCreateInfo();
    info.pixel_size = {.width = 0, .height = 720};
    CheckRefused(info, LIARA_RESULT_INVALID_ARGUMENT);
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
