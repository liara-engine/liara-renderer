#include "CreateInfo.h"

#include <liara/extent.h>
#include <liara/native_window.h>
#include <liara/renderer/renderer.h>
#include <liara/result.h>

#include <string>

namespace Liara::Renderer
{
    namespace
    {
        /** @brief Whether every pointer the window's kind needs is set. The kind is one this build presents to. */
        bool HasWindowPointers(const liara_native_window_t& window) noexcept {
            switch (window.kind) {
                case LIARA_NATIVE_WINDOW_KIND_WIN32:
                    return window.handle.win32.hinstance != nullptr && window.handle.win32.hwnd != nullptr;
                case LIARA_NATIVE_WINDOW_KIND_XLIB: return window.handle.xlib.dpy != nullptr;
                case LIARA_NATIVE_WINDOW_KIND_WAYLAND:
                    return window.handle.wayland.display != nullptr && window.handle.wayland.surface != nullptr;
                default: return true;
            }
        }
    }  // namespace

    bool IsPresentableKind(const liara_native_window_kind_t kind) noexcept {
#ifdef _WIN32
        return kind == LIARA_NATIVE_WINDOW_KIND_WIN32;
#elif defined(__linux__)
        return kind == LIARA_NATIVE_WINDOW_KIND_XLIB || kind == LIARA_NATIVE_WINDOW_KIND_WAYLAND;
#else
        (void)kind;
        return false;
#endif
    }

    liara_result_t CheckCreateInfo(const liara_renderer_create_info_t& createInfo) noexcept {
        if (createInfo.struct_version != LIARA_RENDERER_CREATE_INFO_VERSION) { return LIARA_RESULT_VERSION_MISMATCH; }
        if (createInfo.preferred_device_name == nullptr && createInfo.preferred_device_name_length != 0) {
            return LIARA_RESULT_NULL_POINTER;
        }

        const liara_native_window_t& window = createInfo.native_window;
        if (window.kind == LIARA_NATIVE_WINDOW_KIND_NONE) { return LIARA_RESULT_SUCCESS; }
        if (!IsPresentableKind(window.kind)) { return LIARA_RESULT_NOT_SUPPORTED; }
        if (!HasWindowPointers(window)) { return LIARA_RESULT_NULL_POINTER; }
        if (createInfo.pixel_size.width == 0 || createInfo.pixel_size.height == 0) {
            return LIARA_RESULT_INVALID_ARGUMENT;
        }
        return LIARA_RESULT_SUCCESS;
    }

    RendererConfig ToConfig(const liara_renderer_create_info_t& createInfo) {
        RendererConfig config;
        config.m_Window = createInfo.native_window;
        config.m_PixelSize = createInfo.pixel_size;
        if (createInfo.preferred_device_name_length != 0) {
            config.m_PreferredDeviceName.assign(createInfo.preferred_device_name,
                                                createInfo.preferred_device_name_length);
        }
        return config;
    }
}  // namespace Liara::Renderer
