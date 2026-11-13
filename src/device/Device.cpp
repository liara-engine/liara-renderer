#include "Device.h"

#include "DeviceSelection.h"
#include "Log.h"
#include "Unsupported.h"

#include <cstdint>
#include <format>
#include <optional>
#include <string>
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

        DeviceCandidate Describe(const vk::raii::PhysicalDevice& physical) {
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
            return candidate;
        }

        std::string VersionString(const uint32_t version) {
            return std::format("{}.{}.{}",
                               VK_API_VERSION_MAJOR(version),
                               VK_API_VERSION_MINOR(version),
                               VK_API_VERSION_PATCH(version));
        }

        vk::raii::Device CreateLogicalDevice(const vk::raii::PhysicalDevice& physical, const uint32_t graphicsFamily) {
            constexpr float priority = 1.0F;
            const vk::DeviceQueueCreateInfo queue({}, graphicsFamily, 1, &priority);
            const vk::DeviceCreateInfo createInfo({}, queue);
            return {physical, createInfo};
        }
    }  // namespace

    Device::Device(const vk::raii::Instance& instance)
        : Device([&instance] {
            std::vector<vk::raii::PhysicalDevice> physicals = instance.enumeratePhysicalDevices();
            std::vector<DeviceCandidate> candidates;
            candidates.reserve(physicals.size());
            for (const vk::raii::PhysicalDevice& physical : physicals) { candidates.push_back(Describe(physical)); }

            const DeviceSelection selection = SelectDevice(candidates);
            for (const DeviceRejection& rejection : selection.m_Rejections) {
                Log::Info(std::format("rejected {}: {}", candidates.at(rejection.m_Index).m_Name, rejection.m_Reason));
            }
            if (!selection.m_Chosen.has_value()) {
                throw Unsupported("no physical device supports Vulkan 1.3 with a graphics queue");
            }

            const DeviceCandidate& chosen = candidates.at(*selection.m_Chosen);
            Log::Info(std::format("selected {} ({}, {} {}, Vulkan {})",
                                  chosen.m_Name,
                                  vk::to_string(chosen.m_Type),
                                  chosen.m_DriverName,
                                  chosen.m_DriverInfo,
                                  VersionString(chosen.m_ApiVersion)));
            return Chosen {.m_Physical = std::move(physicals.at(*selection.m_Chosen)),
                           .m_GraphicsFamily = *chosen.m_GraphicsFamily};
        }()) {}

    Device::Device(Chosen chosen)
        : m_Physical(std::move(chosen.m_Physical))
        , m_GraphicsFamily(chosen.m_GraphicsFamily)
        , m_Device(CreateLogicalDevice(m_Physical, m_GraphicsFamily))
        , m_GraphicsQueue(m_Device, m_GraphicsFamily, 0)
        , m_CommandPool(
              m_Device,
              vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_GraphicsFamily)) {}

    const vk::raii::PhysicalDevice& Device::GetPhysical() const noexcept { return m_Physical; }

    const vk::raii::Device& Device::Get() const noexcept { return m_Device; }

    uint32_t Device::GetGraphicsFamily() const noexcept { return m_GraphicsFamily; }
}  // namespace Liara::Renderer
