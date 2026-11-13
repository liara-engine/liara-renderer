#pragma once

/**
 * @file CreateInfo.h
 *
 * What the shim does with a `liara_renderer_create_info_t`: check it without touching Vulkan, then copy what the
 * renderer keeps into a `RendererConfig`, so that nothing past the shim reads the C struct.
 */

#include <liara/extent.h>
#include <liara/native_window.h>
#include <liara/renderer/renderer.h>
#include <liara/result.h>

#include <string>

namespace Liara::Renderer
{
    /** @brief What a renderer is created from, once the create-info has been checked. */
    struct RendererConfig
    {
        liara_native_window_t m_Window {}; /**< Kind `LIARA_NATIVE_WINDOW_KIND_NONE`: no surface. */
        liara_extent2d_t m_PixelSize {};   /**< Meaningful only with a window. */
        std::string m_PreferredDeviceName; /**< Empty: no preference. */
    };

    /** @brief True for the kinds this build can create a surface on: Xlib and Wayland on Linux, Win32 on Windows. */
    [[nodiscard]] bool IsPresentableKind(liara_native_window_kind_t kind) noexcept;

    /**
     * @brief Check a non-null create-info, in the contract's order, and touch nothing else.
     *
     * The version comes first and nothing after it is read before it passes; then the device name, the window's kind,
     * the pointers that kind needs, and the pixel size. Writes nothing anywhere: an argument failure is the caller's
     * mistake, and the code says which.
     */
    [[nodiscard]] liara_result_t CheckCreateInfo(const liara_renderer_create_info_t& createInfo) noexcept;

    /** @brief Copy what the renderer keeps. Call only once `CheckCreateInfo` returned `LIARA_RESULT_SUCCESS`. */
    [[nodiscard]] RendererConfig ToConfig(const liara_renderer_create_info_t& createInfo);
}  // namespace Liara::Renderer
