#pragma once

/**
 * @file Instance.h
 */

#include <liara/native_window.h>

#include <vulkan/vulkan_raii.hpp>

namespace Liara::Renderer
{
    class Loader;

    /**
     * @brief The Vulkan 1.3 instance, and in debug builds the validation layer and its messenger.
     *
     * A debug build (`LIARA_DEBUG`) refuses to run without `VK_LAYER_KHRONOS_validation` and `VK_EXT_debug_utils`, and
     * throws `Unsupported` naming what to install: a clean validation log has to mean the layer ran. It also turns on
     * synchronization validation. Validation warnings and errors go to the log sink, including those emitted while the
     * instance itself is created and destroyed.
     *
     * With a window kind other than `LIARA_NATIVE_WINDOW_KIND_NONE`, the instance also enables `VK_KHR_surface` and
     * that kind's surface extension, and throws `Unsupported` when the loader lacks either.
     */
    class Instance
    {
    public:
        Instance(const Loader& loader, liara_native_window_kind_t windowKind);
        ~Instance() = default;

        Instance(const Instance&) = delete;
        Instance& operator=(const Instance&) = delete;
        Instance(Instance&&) = delete;
        Instance& operator=(Instance&&) = delete;

        [[nodiscard]] const vk::raii::Instance& Get() const noexcept;

    private:
        vk::raii::Instance m_Instance;
        vk::raii::DebugUtilsMessengerEXT m_Messenger; /**< Null outside debug builds. */
    };
}  // namespace Liara::Renderer
