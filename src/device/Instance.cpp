#include "Instance.h"

#include "Loader.h"
#include "Log.h"
#include "Unsupported.h"

#include <liara/renderer/config.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace Liara::Renderer
{
    namespace
    {
// Everything up to CreateInstance exists for validation alone, which only debug builds use. Compiling it out of release
// builds, rather than leaving it behind a discarded `if constexpr`, keeps unused-function warnings (clang's
// -Wunneeded-internal-declaration, MSVC's C4505) from failing the release legs that build with warnings as errors.
#ifdef LIARA_DEBUG
        constexpr const char* VALIDATION_LAYER = "VK_LAYER_KHRONOS_validation";

        VKAPI_ATTR vk::Bool32 VKAPI_CALL OnDebugMessage(const vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                        const vk::DebugUtilsMessageTypeFlagsEXT /*types*/,
                                                        const vk::DebugUtilsMessengerCallbackDataEXT* data,
                                                        void* /*userData*/) noexcept {
            const bool isError = severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
            const std::string_view text = (data != nullptr && data->pMessage != nullptr) ? data->pMessage : "";
            try {
                const std::string line = std::string("validation: ").append(text);
                if (isError) { Log::Error(line); }
                else { Log::Warning(line); }
            }
            catch (...) {
                if (isError) { Log::Error("validation: (message lost: out of memory)"); }
                else { Log::Warning("validation: (message lost: out of memory)"); }
            }
            return VK_FALSE;
        }

        vk::DebugUtilsMessengerCreateInfoEXT MessengerInfo() {
            using Severity = vk::DebugUtilsMessageSeverityFlagBitsEXT;
            using Type = vk::DebugUtilsMessageTypeFlagBitsEXT;
            return {{},
                    Severity::eWarning | Severity::eError,
                    Type::eGeneral | Type::eValidation | Type::ePerformance,
                    &OnDebugMessage};
        }

        bool HasExtension(const std::vector<vk::ExtensionProperties>& extensions, const std::string_view name) {
            return std::ranges::any_of(extensions, [name](const vk::ExtensionProperties& extension) {
                return std::string_view(extension.extensionName.data()) == name;
            });
        }

        /** @brief Throw Unsupported unless a debug build has everything validation needs. */
        void RequireValidation(const vk::raii::Context& context) {
            const auto layers = context.enumerateInstanceLayerProperties();
            const bool hasLayer = std::ranges::any_of(layers, [](const vk::LayerProperties& layer) {
                return std::string_view(layer.layerName.data()) == VALIDATION_LAYER;
            });
            if (!hasLayer) {
                throw Unsupported("debug builds require the Vulkan validation layer (VK_LAYER_KHRONOS_validation), "
                                  "which the loader cannot find: install vulkan-validation-layers on Arch Linux, or "
                                  "the Vulkan SDK on Windows");
            }
            // The loader implements VK_EXT_debug_utils and the validation layer provides it too; either will do.
            if (!HasExtension(context.enumerateInstanceExtensionProperties(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
                && !HasExtension(context.enumerateInstanceExtensionProperties(std::string(VALIDATION_LAYER)),
                                 VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
                throw Unsupported("debug builds require VK_EXT_debug_utils, which neither the Vulkan loader nor the "
                                  "validation layer provides");
            }
        }
#endif

        vk::raii::Instance CreateInstance(const Loader& loader) {
            std::vector<const char*> layers;
            std::vector<const char*> extensions;
#ifdef LIARA_DEBUG
            RequireValidation(loader.GetContext());
            layers.push_back(VALIDATION_LAYER);
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

            // No application name: nothing carries one across the ABI before the renderer has a create-info.
            const vk::ApplicationInfo application(nullptr,
                                                  0,
                                                  "Liara",
                                                  VK_MAKE_API_VERSION(0,
                                                                      LIARA_RENDERER_MAJOR_VERSION,
                                                                      LIARA_RENDERER_MINOR_VERSION,
                                                                      LIARA_RENDERER_PATCH_VERSION),
                                                  VK_API_VERSION_1_3);
            vk::InstanceCreateInfo createInfo({}, &application, layers, extensions);

            // Chained into the instance's own create-info, the messenger also reports what vkCreateInstance and
            // vkDestroyInstance emit, before and after the separate messenger exists.
#ifdef LIARA_DEBUG
            const vk::DebugUtilsMessengerCreateInfoEXT messengerInfo = MessengerInfo();
            createInfo.pNext = &messengerInfo;
#endif

            try {
                return {loader.GetContext(), createInfo};
            }
            catch (const vk::IncompatibleDriverError&) {
                // The loader is there but no driver is: the likeliest failure on a machine that has never run Vulkan,
                // and Vulkan-Hpp's own message ("ErrorIncompatibleDriver") names nothing to install.
                throw Unsupported("no Vulkan driver found for this machine's GPU: install its Vulkan driver "
                                  "(vulkan-radeon, vulkan-intel, nvidia-utils or vulkan-nouveau on Arch Linux, the "
                                  "GPU vendor's driver on Windows)");
            }
        }

        vk::raii::DebugUtilsMessengerEXT CreateMessenger([[maybe_unused]] const vk::raii::Instance& instance) {
#ifdef LIARA_DEBUG
            return {instance, MessengerInfo()};
#else
            return nullptr;
#endif
        }
    }  // namespace

    Instance::Instance(const Loader& loader)
        : m_Instance(CreateInstance(loader))
        , m_Messenger(CreateMessenger(m_Instance)) {}

    const vk::raii::Instance& Instance::Get() const noexcept { return m_Instance; }
}  // namespace Liara::Renderer
