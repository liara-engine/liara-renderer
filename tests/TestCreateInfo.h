#pragma once

/**
 * @file TestCreateInfo.h
 * @brief The create-info most renderer tests pass: version 1, no window, no GPU preference.
 */

#include <liara/native_window.h>
#include <liara/renderer/renderer.h>

/** A renderer without a surface: it creates its device and presents nothing, so a test needs no window. */
constexpr liara_renderer_create_info_t NoWindowCreateInfo() noexcept {
    liara_renderer_create_info_t info {};
    info.struct_version = LIARA_RENDERER_CREATE_INFO_VERSION;
    info.native_window.kind = LIARA_NATIVE_WINDOW_KIND_NONE;
    return info;
}

/** `NoWindowCreateInfo()`, for tests that only need to pass one. */
inline constexpr liara_renderer_create_info_t NO_WINDOW = NoWindowCreateInfo();
