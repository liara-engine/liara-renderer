#include "Device.h"

#include "DeviceSelection.h"
#include "Log.h"
#include "Unsupported.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Liara::Renderer
{
    namespace
    {
        std::optional<uint32_t> FirstGraphicsFamily(const vk::raii::PhysicalDevice& physical) {
            uint32_t index = 0;
            for (const vk::QueueFamilyProperties& family : physical.getQueueFamilyProperties()) {
                if (family.queueFlags & vk::QueueFlagBits::eGraphics) { return index; }
                ++index;
            }
            return std::nullopt;
        }

        /** @brief The first family that supports graphics and can present to the surface, if any. */
        std::optional<uint32_t> FirstGraphicsPresentFamily(const vk::raii::PhysicalDevice& physical,
                                                           const vk::raii::SurfaceKHR& surface) {
            uint32_t index = 0;
            for (const vk::QueueFamilyProperties& family : physical.getQueueFamilyProperties()) {
                if ((family.queueFlags & vk::QueueFlagBits::eGraphics)
                    && physical.getSurfaceSupportKHR(index, *surface) == vk::True) {
                    return index;
                }
                ++index;
            }
            return std::nullopt;
        }

        bool SupportsSwapchain(const vk::raii::PhysicalDevice& physical) {
            return std::ranges::any_of(physical.enumerateDeviceExtensionProperties(),
                                       [](const vk::ExtensionProperties& extension) {
                                           return std::string_view(extension.extensionName.data())
                                                  == VK_KHR_SWAPCHAIN_EXTENSION_NAME;
                                       });
        }

        DeviceCandidate Describe(const vk::raii::PhysicalDevice& physical, const vk::raii::SurfaceKHR* surface) {
            const vk::PhysicalDeviceProperties properties = physical.getProperties();
            DeviceCandidate candidate {
                .m_Name = properties.deviceName.data(),
                .m_Type = properties.deviceType,
                .m_ApiVersion = properties.apiVersion,
                .m_DriverName = {},
                .m_DriverInfo = {},
                .m_GraphicsFamily = FirstGraphicsFamily(physical),
                .m_SupportsSwapchain = false,
                .m_GraphicsPresentFamily = std::nullopt,
            };
            // The driver's name and version string are core in Vulkan 1.2; an older device is rejected anyway.
            if (properties.apiVersion >= VK_API_VERSION_1_2) {
                const auto chain =
                    physical.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceDriverProperties>();
                const auto& driver = chain.get<vk::PhysicalDeviceDriverProperties>();
                candidate.m_DriverName = driver.driverName.data();
                candidate.m_DriverInfo = driver.driverInfo.data();
            }
            if (surface != nullptr) {
                candidate.m_SupportsSwapchain = SupportsSwapchain(physical);
                candidate.m_GraphicsPresentFamily = FirstGraphicsPresentFamily(physical, *surface);
            }
            return candidate;
        }

        /** @brief Log what the selection saw and decided; it writes nothing itself. */
        void LogSelection(const std::vector<DeviceCandidate>& candidates,
                          const DeviceSelection& selection,
                          const std::string_view preferredName) {
            if (!preferredName.empty()) { Log::Info(std::format("preferred GPU: \"{}\"", preferredName)); }
            for (size_t index = 0; index < candidates.size(); ++index) {
                const auto rejection = std::ranges::find(selection.m_Rejections, index, &DeviceRejection::m_Index);
                if (rejection != selection.m_Rejections.end()) {
                    Log::Info(std::format("rejected {}: {}", candidates.at(index).m_Name, rejection->m_Reason));
                }
                else {
                    Log::Info(std::format("candidate {} ({})",
                                          candidates.at(index).m_Name,
                                          vk::to_string(candidates.at(index).m_Type)));
                }
            }
            switch (selection.m_Preference) {
                case PreferenceOutcome::NoMatch:
                    Log::Warning(
                        std::format("no GPU name contains \"{}\"; selecting as without a preference", preferredName));
                    break;
                case PreferenceOutcome::MatchedOnlyRejected:
                    Log::Warning(std::format("every GPU whose name contains \"{}\" was rejected; selecting as without "
                                             "a preference",
                                             preferredName));
                    break;
                case PreferenceOutcome::NotGiven:
                case PreferenceOutcome::Honoured: break;
            }
        }

        std::string VersionString(const uint32_t version) {
            return std::format("{}.{}.{}",
                               VK_API_VERSION_MAJOR(version),
                               VK_API_VERSION_MINOR(version),
                               VK_API_VERSION_PATCH(version));
        }

        vk::raii::Device CreateLogicalDevice(const vk::raii::PhysicalDevice& physical,
                                             const uint32_t family,
                                             const bool presenting) {
            constexpr float priority = 1.0F;
            const vk::DeviceQueueCreateInfo queue({}, family, 1, &priority);
            std::vector<const char*> extensions;
            if (presenting) { extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME); }
            // Both mandatory in Vulkan 1.3: frames are recorded with dynamic rendering and synchronization2 barriers.
            const vk::StructureChain<vk::DeviceCreateInfo, vk::PhysicalDeviceVulkan13Features> chain(
                vk::DeviceCreateInfo({}, queue, {}, extensions),
                vk::PhysicalDeviceVulkan13Features().setSynchronization2(vk::True).setDynamicRendering(vk::True));
            return {physical, chain.get<vk::DeviceCreateInfo>()};
        }
    }  // namespace

    Device::Device(const vk::raii::Instance& instance,
                   const vk::raii::SurfaceKHR* surface,
                   const std::string_view preferredName)
        : Device([&instance, surface, preferredName] {
            std::vector<vk::raii::PhysicalDevice> physicals = instance.enumeratePhysicalDevices();
            std::vector<DeviceCandidate> candidates;
            candidates.reserve(physicals.size());
            for (const vk::raii::PhysicalDevice& physical : physicals) {
                candidates.push_back(Describe(physical, surface));
            }

            const bool presenting = surface != nullptr;
            const DeviceSelection selection =
                SelectDevice(candidates, {.m_RequirePresentation = presenting, .m_PreferredName = preferredName});
            LogSelection(candidates, selection, preferredName);
            if (!selection.m_Chosen.has_value()) {
                throw Unsupported(presenting ? "no physical device supports Vulkan 1.3 with a queue that can draw and "
                                               "present to the window"
                                             : "no physical device supports Vulkan 1.3 with a graphics queue");
            }

            const DeviceCandidate& chosen = candidates.at(*selection.m_Chosen);
            Log::Info(std::format("selected {} ({}, {} {}, Vulkan {})",
                                  chosen.m_Name,
                                  vk::to_string(chosen.m_Type),
                                  chosen.m_DriverName,
                                  chosen.m_DriverInfo,
                                  VersionString(chosen.m_ApiVersion)));
            return Chosen {.m_Physical = std::move(physicals.at(*selection.m_Chosen)),
                           .m_GraphicsFamily = presenting ? *chosen.m_GraphicsPresentFamily : *chosen.m_GraphicsFamily,
                           .m_Presenting = presenting};
        }()) {}

    Device::Device(Chosen chosen)
        : m_Physical(std::move(chosen.m_Physical))
        , m_GraphicsFamily(chosen.m_GraphicsFamily)
        , m_Device(CreateLogicalDevice(m_Physical, m_GraphicsFamily, chosen.m_Presenting))
        , m_GraphicsQueue(m_Device, m_GraphicsFamily, 0)
        , m_CommandPool(
              m_Device,
              vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_GraphicsFamily)) {}

    const vk::raii::PhysicalDevice& Device::GetPhysical() const noexcept { return m_Physical; }

    const vk::raii::Device& Device::Get() const noexcept { return m_Device; }

    uint32_t Device::GetGraphicsFamily() const noexcept { return m_GraphicsFamily; }

    const vk::raii::Queue& Device::GetQueue() const noexcept { return m_GraphicsQueue; }

    const vk::raii::CommandPool& Device::GetCommandPool() const noexcept { return m_CommandPool; }
}  // namespace Liara::Renderer
