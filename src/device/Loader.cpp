#include "Loader.h"

#include "Log.h"
#include "Unsupported.h"

#include <cstdint>
#include <format>
#include <stdexcept>

namespace Liara::Renderer
{
    namespace
    {
        vk::raii::Context OpenLoader() {
            try {
                return {};
            }
            catch (const std::runtime_error&) {
                // Vulkan-Hpp's DynamicLoader reports a library it cannot open with a plain std::runtime_error.
                throw Unsupported("no Vulkan loader found (libvulkan.so.1 on Linux, vulkan-1.dll on Windows); install "
                                  "the Vulkan loader and a driver for the GPU");
            }
        }

        uint32_t LoaderVersion(const vk::raii::Context& context) {
            // vkEnumerateInstanceVersion appeared in Vulkan 1.1, so a loader without it is a 1.0 loader.
            if (context.getDispatcher()->vkEnumerateInstanceVersion == nullptr) { return VK_API_VERSION_1_0; }
            return context.enumerateInstanceVersion();
        }
    }  // namespace

    Loader::Loader()
        : m_Context(OpenLoader()) {
        const uint32_t version = LoaderVersion(m_Context);
        if (version < VK_API_VERSION_1_3) {
            throw Unsupported(std::format("the Vulkan loader implements {}.{}, and the renderer requires 1.3",
                                          VK_API_VERSION_MAJOR(version),
                                          VK_API_VERSION_MINOR(version)));
        }
        Log::Info(std::format("Vulkan loader {}.{}.{}",
                              VK_API_VERSION_MAJOR(version),
                              VK_API_VERSION_MINOR(version),
                              VK_API_VERSION_PATCH(version)));
    }

    const vk::raii::Context& Loader::GetContext() const noexcept { return m_Context; }
}  // namespace Liara::Renderer
