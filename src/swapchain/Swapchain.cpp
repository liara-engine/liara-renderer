#include "Swapchain.h"

#include "Log.h"
#include "PresentStatus.h"
#include "SwapchainChoices.h"

#include "device/Device.h"

#include <liara/extent.h>

#include <cstdint>
#include <format>
#include <stdexcept>
#include <vector>

namespace Liara::Renderer
{
    Swapchain::Swapchain(const Device& device, const vk::raii::SurfaceKHR& surface, const liara_extent2d_t pixelSize) {
        const vk::raii::PhysicalDevice& physical = device.GetPhysical();
        const vk::SurfaceCapabilitiesKHR capabilities = physical.getSurfaceCapabilitiesKHR(*surface);
        const std::vector<vk::SurfaceFormatKHR> formats = physical.getSurfaceFormatsKHR(*surface);

        const vk::SurfaceFormatKHR format = ChooseSurfaceFormat(formats);
        m_Format = format.format;
        m_Extent = ChooseExtent(capabilities, pixelSize);
        if (m_Extent.width == 0 || m_Extent.height == 0) {
            throw std::runtime_error("the window has no area, so no swapchain can be created on it");
        }

        // FIFO is the only mode every implementation offers; choosing another is a create-info field for later.
        constexpr vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
        const vk::SwapchainCreateInfoKHR createInfo({},
                                                    *surface,
                                                    ChooseImageCount(capabilities),
                                                    format.format,
                                                    format.colorSpace,
                                                    m_Extent,
                                                    1,
                                                    vk::ImageUsageFlagBits::eColorAttachment,
                                                    vk::SharingMode::eExclusive,
                                                    {},
                                                    capabilities.currentTransform,
                                                    ChooseCompositeAlpha(capabilities.supportedCompositeAlpha),
                                                    presentMode,
                                                    vk::True);
        m_Swapchain = device.Get().createSwapchainKHR(createInfo);
        m_Images = m_Swapchain.getImages();

        m_Views.reserve(m_Images.size());
        m_RenderFinished.reserve(m_Images.size());
        for (const vk::Image image : m_Images) {
            const vk::ImageViewCreateInfo viewInfo(
                {},
                image,
                vk::ImageViewType::e2D,
                m_Format,
                {},
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
            m_Views.push_back(device.Get().createImageView(viewInfo));
            m_RenderFinished.push_back(device.Get().createSemaphore({}));
        }

        Log::Info(std::format("swapchain {}x{}, {} images, {} {}, {}",
                              m_Extent.width,
                              m_Extent.height,
                              m_Images.size(),
                              vk::to_string(format.format),
                              vk::to_string(format.colorSpace),
                              vk::to_string(presentMode)));
    }

    vk::Format Swapchain::GetFormat() const noexcept { return m_Format; }

    vk::Extent2D Swapchain::GetExtent() const noexcept { return m_Extent; }

    vk::Image Swapchain::GetImage(const uint32_t index) const { return m_Images.at(index); }

    const vk::raii::ImageView& Swapchain::GetView(const uint32_t index) const { return m_Views.at(index); }

    const vk::raii::Semaphore& Swapchain::GetRenderFinished(const uint32_t index) const {
        return m_RenderFinished.at(index);
    }

    PresentStatus Swapchain::Acquire(const vk::raii::Semaphore& imageAcquired, uint32_t& outIndex) {
        try {
            // No timeout: FIFO hands an image back within a refresh or two. VK_SUBOPTIMAL_KHR counts as acquired.
            const auto [result, index] = m_Swapchain.acquireNextImage(UINT64_MAX, *imageAcquired);
            outIndex = index;
            return PresentStatus::Ok;
        }
        catch (const vk::OutOfDateKHRError&) {
            return PresentStatus::OutOfDate;
        }
        catch (const vk::SurfaceLostKHRError&) {
            return PresentStatus::SurfaceLost;
        }
    }

    PresentStatus Swapchain::Present(const vk::raii::Queue& queue, const uint32_t index) {
        const vk::Semaphore renderFinished = *m_RenderFinished.at(index);
        const vk::SwapchainKHR swapchain = *m_Swapchain;
        try {
            // VK_SUBOPTIMAL_KHR counts as presented.
            (void)queue.presentKHR(vk::PresentInfoKHR(renderFinished, swapchain, index));
            return PresentStatus::Ok;
        }
        catch (const vk::OutOfDateKHRError&) {
            return PresentStatus::OutOfDate;
        }
        catch (const vk::SurfaceLostKHRError&) {
            return PresentStatus::SurfaceLost;
        }
    }
}  // namespace Liara::Renderer
