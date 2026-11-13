#pragma once

/**
 * @file Loader.h
 */

#include <vulkan/vulkan_raii.hpp>

namespace Liara::Renderer
{
    /**
     * @brief The Vulkan loader, opened at run time rather than linked (ADR 0016).
     *
     * Constructing it opens the loader library through Vulkan-Hpp's `DynamicLoader`, which `vk::raii::Context` owns,
     * and checks that it implements Vulkan 1.3. Both failures throw `Unsupported`. On success it logs the loader's
     * version.
     */
    class Loader
    {
    public:
        Loader();
        ~Loader() = default;

        Loader(const Loader&) = delete;
        Loader& operator=(const Loader&) = delete;
        Loader(Loader&&) = delete;
        Loader& operator=(Loader&&) = delete;

        /** @brief The entry point to every global Vulkan function, and the source of `vkGetInstanceProcAddr`. */
        [[nodiscard]] const vk::raii::Context& GetContext() const noexcept;

    private:
        vk::raii::Context m_Context;
    };
}  // namespace Liara::Renderer
