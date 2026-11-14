#pragma once

/**
 * @file Device.h
 */

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <string_view>

namespace Liara::Renderer
{
    /**
     * @brief The selected physical device, the logical device created on it, its one queue and one command pool.
     *
     * Construction builds a `DeviceCandidate` for every physical device, lets `SelectDevice` choose, logs the eligible,
     * rejected and selected candidates and what became of the preferred name, and throws `Unsupported` when nothing
     * qualifies. With a surface, a candidate must also support VK_KHR_swapchain and have a queue family that draws and
     * presents to it, and the device enables VK_KHR_swapchain. Dynamic rendering and synchronization2 are always on.
     */
    class Device
    {
    public:
        /** @param surface Null for a renderer without a surface. */
        Device(const vk::raii::Instance& instance, const vk::raii::SurfaceKHR* surface, std::string_view preferredName);
        ~Device() = default;

        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;
        Device(Device&&) = delete;
        Device& operator=(Device&&) = delete;

        [[nodiscard]] const vk::raii::PhysicalDevice& GetPhysical() const noexcept;
        [[nodiscard]] const vk::raii::Device& Get() const noexcept;
        /** @brief The family the queue comes from: graphics, and presentation when there is a surface. */
        [[nodiscard]] uint32_t GetGraphicsFamily() const noexcept;
        [[nodiscard]] const vk::raii::Queue& GetQueue() const noexcept;
        [[nodiscard]] const vk::raii::CommandPool& GetCommandPool() const noexcept;

    private:
        struct Chosen
        {
            vk::raii::PhysicalDevice m_Physical;
            uint32_t m_GraphicsFamily;
            bool m_Presenting;
        };

        explicit Device(Chosen chosen);

        // Declared in creation order, so destroyed pool first and physical device last.
        vk::raii::PhysicalDevice m_Physical;
        uint32_t m_GraphicsFamily;
        vk::raii::Device m_Device;
        vk::raii::Queue m_GraphicsQueue;
        vk::raii::CommandPool m_CommandPool;
    };
}  // namespace Liara::Renderer
