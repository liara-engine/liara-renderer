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
#include <string_view>
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
        bool m_SupportsSwapchain = false;         /**< VK_KHR_swapchain among the device's extensions. */
        /** First family with VK_QUEUE_GRAPHICS_BIT that can also present to the surface, if any. */
        std::optional<uint32_t> m_GraphicsPresentFamily;
    };

    /** @brief What the caller asks of the selection besides the fixed requirements. */
    struct SelectionCriteria
    {
        bool m_RequirePresentation = false; /**< True when the renderer has a surface to present to. */
        std::string_view m_PreferredName;   /**< Empty: no preference. */
    };

    /** @brief What became of the preferred name. */
    enum class PreferenceOutcome : std::uint8_t
    {
        NotGiven,           /**< No name was given. (Not `None`: <X11/Xlib.h> defines it as a macro.) */
        Honoured,           /**< The chosen candidate's name contains it. */
        NoMatch,            /**< No candidate's name contains it. */
        MatchedOnlyRejected /**< Some names contain it, and every one of those candidates was rejected. */
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
        PreferenceOutcome m_Preference = PreferenceOutcome::NotGiven;
    };

    /**
     * @brief Choose the device the renderer creates.
     *
     * A candidate is rejected by the first requirement it fails, in this order: an API version below Vulkan 1.3, then
     * no queue family supporting graphics, then, only when presentation is required, no VK_KHR_swapchain, then no
     * family doing both graphics and presentation. Among the others, those whose name contains the preferred name,
     * compared case-insensitively over ASCII, come first; within each group a discrete GPU beats an integrated one,
     * which beats a virtual one, which beats a CPU implementation, which beats anything else; ties go to the lowest
     * index, which is the enumeration order. A candidate that is merely outranked is not a rejection.
     *
     * Writes nothing anywhere: the outcome of the preference is reported in the result, for the caller to log.
     */
    [[nodiscard]] DeviceSelection SelectDevice(std::span<const DeviceCandidate> candidates,
                                               const SelectionCriteria& request = {});
}  // namespace Liara::Renderer
