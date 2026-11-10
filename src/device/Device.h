#pragma once

/**
 * @file Device.h
 */

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>

namespace Liara::Renderer
{
    /**
     * @brief The selected physical device, the logical device created on it, its one graphics queue and one command
     * pool.
     *
     * Construction builds a `DeviceCandidate` for every physical device, lets `SelectDevice` choose, logs the rejected
     * candidates and the selected one, and throws `Unsupported` when nothing qualifies. The logical device enables no
     * extension and no optional feature yet: the swapchain work adds what it uses.
     */
    class Device
    {
    public:
        explicit Device(const vk::raii::Instance& instance);
        ~Device() = default;

        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;
        Device(Device&&) = delete;
        Device& operator=(Device&&) = delete;

        [[nodiscard]] const vk::raii::PhysicalDevice& GetPhysical() const noexcept;
        [[nodiscard]] const vk::raii::Device& Get() const noexcept;
        [[nodiscard]] uint32_t GetGraphicsFamily() const noexcept;

    private:
        struct Chosen
        {
            vk::raii::PhysicalDevice m_Physical;
            uint32_t m_GraphicsFamily;
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
