#include "Buffer.h"

#include "Allocator.h"

#include <vulkan/vulkan.hpp>

#include <cstddef>
#include <cstring>
#include <span>
#include <stdexcept>
#include <utility>

#include <vk_mem_alloc.h>

namespace Liara::Renderer
{
    namespace
    {
        VmaAllocationCreateInfo AllocationInfo(const BufferAccess access) noexcept {
            VmaAllocationCreateInfo info {};
            switch (access) {
                case BufferAccess::DeviceOnly: info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE; break;
                case BufferAccess::HostWrite:
                    info.usage = VMA_MEMORY_USAGE_AUTO;
                    info.flags =
                        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
                    break;
                case BufferAccess::HostRead:
                    info.usage = VMA_MEMORY_USAGE_AUTO;
                    info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
                    break;
            }
            return info;
        }
    }  // namespace

    Buffer::Buffer(const Allocator& allocator,
                   const vk::DeviceSize size,
                   const vk::BufferUsageFlags usage,
                   const BufferAccess access)
        : m_Allocator(&allocator)
        , m_Size(size) {
        const VkBufferCreateInfo bufferInfo = vk::BufferCreateInfo({}, size, usage, vk::SharingMode::eExclusive);
        const VmaAllocationCreateInfo allocationInfo = AllocationInfo(access);
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = nullptr;
        VmaAllocationInfo allocated {};
        vk::detail::resultCheck(
            static_cast<vk::Result>(
                vmaCreateBuffer(allocator.Get(), &bufferInfo, &allocationInfo, &buffer, &allocation, &allocated)),
            "vmaCreateBuffer");
        m_Buffer = buffer;
        m_Allocation = allocation;
        m_Mapped = allocated.pMappedData;
    }

    Buffer::~Buffer() { Destroy(); }

    Buffer::Buffer(Buffer&& other) noexcept
        : m_Allocator(other.m_Allocator)
        , m_Buffer(std::exchange(other.m_Buffer, nullptr))
        , m_Allocation(std::exchange(other.m_Allocation, nullptr))
        , m_Mapped(std::exchange(other.m_Mapped, nullptr))
        , m_Size(std::exchange(other.m_Size, 0)) {}

    Buffer& Buffer::operator=(Buffer&& other) noexcept {
        if (this != &other) {
            Destroy();
            m_Allocator = other.m_Allocator;
            m_Buffer = std::exchange(other.m_Buffer, nullptr);
            m_Allocation = std::exchange(other.m_Allocation, nullptr);
            m_Mapped = std::exchange(other.m_Mapped, nullptr);
            m_Size = std::exchange(other.m_Size, 0);
        }
        return *this;
    }

    vk::Buffer Buffer::Get() const noexcept { return m_Buffer; }

    vk::DeviceSize Buffer::GetSize() const noexcept { return m_Size; }

    void Buffer::Write(const vk::DeviceSize offset, const std::span<const std::byte> bytes) {
        if (m_Mapped == nullptr || offset + bytes.size() > m_Size) {
            throw std::logic_error("writing outside a mapped buffer");
        }
        std::memcpy(static_cast<std::byte*>(m_Mapped) + offset, bytes.data(), bytes.size());
        vk::detail::resultCheck(
            static_cast<vk::Result>(vmaFlushAllocation(m_Allocator->Get(), m_Allocation, offset, bytes.size())),
            "vmaFlushAllocation");
    }

    void Buffer::Read(const vk::DeviceSize offset, const std::span<std::byte> out) const {
        if (m_Mapped == nullptr || offset + out.size() > m_Size) {
            throw std::logic_error("reading outside a mapped buffer");
        }
        vk::detail::resultCheck(
            static_cast<vk::Result>(vmaInvalidateAllocation(m_Allocator->Get(), m_Allocation, offset, out.size())),
            "vmaInvalidateAllocation");
        std::memcpy(out.data(), static_cast<const std::byte*>(m_Mapped) + offset, out.size());
    }

    void Buffer::Destroy() noexcept {
        if (m_Allocation != nullptr) {
            vmaDestroyBuffer(m_Allocator->Get(), static_cast<VkBuffer>(m_Buffer), m_Allocation);
        }
        m_Buffer = nullptr;
        m_Allocation = nullptr;
        m_Mapped = nullptr;
    }
}  // namespace Liara::Renderer
