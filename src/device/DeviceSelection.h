#pragma once

/**
 * @file DeviceSelection.h
 *
 * The policy that picks the physical device, as a pure function over plain data. Querying Vulkan to build the
 * candidates and logging the outcome both belong to the caller (Device.cpp), which is what lets the policy be tested
 * without a GPU (tests/test_renderer_device_selection_cxx.cpp compiles DeviceSelection.cpp on its own).
 */

#include <vulkan/vulkan.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace Liara::Renderer
{
    /** @brief One physical device, reduced to what the policy reads and what the log reports. */
    struct DeviceCandidate
    {
        std::string m_Name;
        vk::PhysicalDeviceType m_Type = vk::PhysicalDeviceType::eOther;
        uint32_t m_ApiVersion = 0;
        std::string m_DriverName;                 /**< VkPhysicalDeviceDriverProperties::driverName. */
        std::string m_DriverInfo;                 /**< VkPhysicalDeviceDriverProperties::driverInfo. */
        std::optional<uint32_t> m_GraphicsFamily; /**< First queue family with VK_QUEUE_GRAPHICS_BIT, if any. */
    };

    /** @brief A candidate that failed a requirement, and the first requirement it failed. */
    struct DeviceRejection
    {
        size_t m_Index = 0;
        std::string m_Reason;
    };

    /** @brief The outcome: the chosen candidate's index if any, and every rejected candidate in index order. */
    struct DeviceSelection
    {
        std::optional<size_t> m_Chosen;
        std::vector<DeviceRejection> m_Rejections;
    };

    /**
     * @brief Choose the device the renderer creates.
     *
     * A candidate is rejected by the first requirement it fails, in this order: an API version below Vulkan 1.3, then
     * no queue family supporting graphics. Among the others, a discrete GPU beats an integrated one, which beats a
     * virtual one, which beats a CPU implementation, which beats anything else; ties go to the lowest index, which is
     * the enumeration order. A candidate that is merely outranked is not a rejection.
     *
     * Writes nothing anywhere.
     */
    [[nodiscard]] DeviceSelection SelectDevice(std::span<const DeviceCandidate> candidates);
}  // namespace Liara::Renderer
