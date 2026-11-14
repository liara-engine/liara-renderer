#pragma once

/**
 * @file Surface.h
 */

#include <liara/native_window.h>

#include <vulkan/vulkan_raii.hpp>

#include <string_view>

namespace Liara::Renderer
{
    /** @brief The instance extension a surface on this kind of window needs, or null for a kind without one. */
    [[nodiscard]] const char* SurfaceExtensionName(liara_native_window_kind_t kind) noexcept;

    /** @brief The window system's name, for messages: `Xlib`, `Wayland`, `Win32`, or `unknown`. */
    [[nodiscard]] std::string_view WindowKindName(liara_native_window_kind_t kind) noexcept;

    /** @brief The article that goes before `WindowKindName(kind)` in a message: `an` for Xlib, `a` otherwise. */
    [[nodiscard]] std::string_view WindowKindArticle(liara_native_window_kind_t kind) noexcept;

    /**
     * @brief The Vulkan surface on the host's window.
     *
     * Created from the window the create-info described, whose kind `CheckCreateInfo` already accepted for this build.
     * The window's pointers are borrowed for the surface's whole life, which the contract makes the host guarantee.
     */
    class Surface
    {
    public:
        Surface(const vk::raii::Instance& instance, const liara_native_window_t& window);
        ~Surface() = default;

        Surface(const Surface&) = delete;
        Surface& operator=(const Surface&) = delete;
        Surface(Surface&&) = delete;
        Surface& operator=(Surface&&) = delete;

        [[nodiscard]] const vk::raii::SurfaceKHR& Get() const noexcept;

    private:
        vk::raii::SurfaceKHR m_Surface;
    };
}  // namespace Liara::Renderer
