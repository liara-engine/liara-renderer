#pragma once

/**
 * @file Buffer.h
 */

#include <vulkan/vulkan.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

// VmaAllocation is a pointer to this incomplete type; declaring it here keeps vk_mem_alloc.h out of the header.
struct VmaAllocation_T;

namespace Liara::Renderer
{
    class Allocator;

    /** @brief Who reads and writes a buffer's memory, which decides where VMA places it. */
    enum class BufferAccess : std::uint8_t
    {
        DeviceOnly,  ///< The GPU alone; filled by a copy. Device-local memory when there is some.
        HostWrite,   ///< Written by the CPU, front to back, then read by the GPU: a staging buffer. Mapped.
        HostRead,    ///< Written by the GPU, then read by the CPU: a readback buffer. Mapped.
    };

    /**
     * @brief A VkBuffer and its VMA allocation, destroyed together. Move-only.
     *
     * Creation failures are thrown through Vulkan-Hpp's result check, so that running out of memory reaches the shim's
     * translation table as `vk::OutOfDeviceMemoryError` or `vk::OutOfHostMemoryError`.
     */
    class Buffer
    {
    public:
        Buffer(const Allocator& allocator, vk::DeviceSize size, vk::BufferUsageFlags usage, BufferAccess access);
        ~Buffer();

        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;
        Buffer(Buffer&& other) noexcept;
        Buffer& operator=(Buffer&& other) noexcept;

        [[nodiscard]] vk::Buffer Get() const noexcept;
        [[nodiscard]] vk::DeviceSize GetSize() const noexcept;

        /** @brief `HostWrite` buffers only: copy `bytes` at `offset`, then flush, so the GPU sees them. */
        void Write(vk::DeviceSize offset, std::span<const std::byte> bytes);

        /** @brief `HostRead` buffers only: invalidate, so the GPU's writes are visible, then copy out from `offset`. */
        void Read(vk::DeviceSize offset, std::span<std::byte> out) const;

    private:
        void Destroy() noexcept;

        const Allocator* m_Allocator;
        vk::Buffer m_Buffer;
        VmaAllocation_T* m_Allocation = nullptr;
        void* m_Mapped = nullptr;
        vk::DeviceSize m_Size = 0;
    };
}  // namespace Liara::Renderer
