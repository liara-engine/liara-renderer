#include "Allocator.h"

#include "Device.h"
#include "Instance.h"
#include "Loader.h"

#include <vulkan/vulkan.hpp>

// The one translation unit that compiles VMA itself. VMA_STATIC_VULKAN_FUNCTIONS=0 and VMA_DYNAMIC_VULKAN_FUNCTIONS=1
// come from the target's definitions, so every file including vk_mem_alloc.h sees the same configuration.
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace Liara::Renderer
{
    Allocator::Allocator(const Loader& loader, const Instance& instance, const Device& device) {
        VmaVulkanFunctions functions {};
        functions.vkGetInstanceProcAddr = loader.GetContext().getDispatcher()->vkGetInstanceProcAddr;
        functions.vkGetDeviceProcAddr = instance.Get().getDispatcher()->vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo createInfo {};
        createInfo.physicalDevice = static_cast<VkPhysicalDevice>(*device.GetPhysical());
        createInfo.device = static_cast<VkDevice>(*device.Get());
        createInfo.instance = static_cast<VkInstance>(*instance.Get());
        createInfo.pVulkanFunctions = &functions;
        createInfo.vulkanApiVersion = VK_API_VERSION_1_3;

        VmaAllocator allocator = nullptr;
        vk::detail::resultCheck(static_cast<vk::Result>(vmaCreateAllocator(&createInfo, &allocator)),
                                "vmaCreateAllocator");
        m_Allocator = allocator;
    }

    Allocator::~Allocator() { vmaDestroyAllocator(m_Allocator); }
}  // namespace Liara::Renderer
