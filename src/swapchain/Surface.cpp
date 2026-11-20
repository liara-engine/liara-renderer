#include "Surface.h"

#include "Log.h"
#include "Unsupported.h"

#include <liara/native_window.h>

#include <format>
#include <string_view>

#ifdef VK_USE_PLATFORM_XLIB_KHR
    #include <X11/X.h>
    #include <X11/Xlib.h>
#endif

namespace Liara::Renderer
{
    namespace
    {
        vk::raii::SurfaceKHR CreateSurface(const vk::raii::Instance& instance, const liara_native_window_t& window) {
            switch (window.kind) {
#ifdef VK_USE_PLATFORM_XLIB_KHR
                case LIARA_NATIVE_WINDOW_KIND_XLIB:
                    return instance.createXlibSurfaceKHR(
                        vk::XlibSurfaceCreateInfoKHR({},
                                                     static_cast<Display*>(window.handle.xlib.dpy),
                                                     static_cast<::Window>(window.handle.xlib.window)));
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
                case LIARA_NATIVE_WINDOW_KIND_WAYLAND:
                    return instance.createWaylandSurfaceKHR(
                        vk::WaylandSurfaceCreateInfoKHR({},
                                                        static_cast<wl_display*>(window.handle.wayland.display),
                                                        static_cast<wl_surface*>(window.handle.wayland.surface)));
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
                case LIARA_NATIVE_WINDOW_KIND_WIN32:
                    return instance.createWin32SurfaceKHR(
                        vk::Win32SurfaceCreateInfoKHR({},
                                                      static_cast<HINSTANCE>(window.handle.win32.hinstance),
                                                      static_cast<HWND>(window.handle.win32.hwnd)));
#endif
                default:
                    throw Unsupported(std::format("this build cannot present to {} {} window",
                                                  WindowKindArticle(window.kind),
                                                  WindowKindName(window.kind)));
            }
        }
    }  // namespace

    const char* SurfaceExtensionName(const liara_native_window_kind_t kind) noexcept {
        switch (kind) {
            case LIARA_NATIVE_WINDOW_KIND_XLIB: return "VK_KHR_xlib_surface";
            case LIARA_NATIVE_WINDOW_KIND_WAYLAND: return "VK_KHR_wayland_surface";
            case LIARA_NATIVE_WINDOW_KIND_WIN32: return "VK_KHR_win32_surface";
            default: return nullptr;
        }
    }

    std::string_view WindowKindName(const liara_native_window_kind_t kind) noexcept {
        switch (kind) {
            case LIARA_NATIVE_WINDOW_KIND_XLIB: return "Xlib";
            case LIARA_NATIVE_WINDOW_KIND_WAYLAND: return "Wayland";
            case LIARA_NATIVE_WINDOW_KIND_WIN32: return "Win32";
            default: return "unknown";
        }
    }

    std::string_view WindowKindArticle(const liara_native_window_kind_t kind) noexcept {
        return (kind == LIARA_NATIVE_WINDOW_KIND_XLIB || WindowKindName(kind) == "unknown") ? "an" : "a";
    }

    Surface::Surface(const vk::raii::Instance& instance, const liara_native_window_t& window)
        : m_Surface(CreateSurface(instance, window)) {
        Log::Info(std::format("surface created on {} {} window",
                              WindowKindArticle(window.kind),
                              WindowKindName(window.kind)));
    }

    const vk::raii::SurfaceKHR& Surface::Get() const noexcept { return m_Surface; }
}  // namespace Liara::Renderer
