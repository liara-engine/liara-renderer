#pragma once

/**
 * @file Allocator.h
 */

// VmaAllocator is a pointer to this incomplete type; declaring it here keeps vk_mem_alloc.h out of the header.
struct VmaAllocator_T;

namespace Liara::Renderer
{
    class Device;
    class Instance;
    class Loader;

    /**
     * @brief The VMA allocator for the logical device, owning nothing allocated yet.
     *
     * VMA is built with its static Vulkan functions off (the loader is not linked, ADR 0016) and fetches what it needs
     * through the `vkGetInstanceProcAddr` and `vkGetDeviceProcAddr` pointers it is given here.
     */
    class Allocator
    {
    public:
        Allocator(const Loader& loader, const Instance& instance, const Device& device);
        ~Allocator();

        Allocator(const Allocator&) = delete;
        Allocator& operator=(const Allocator&) = delete;
        Allocator(Allocator&&) = delete;
        Allocator& operator=(Allocator&&) = delete;

    private:
        VmaAllocator_T* m_Allocator = nullptr;
    };
}  // namespace Liara::Renderer
