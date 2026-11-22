#include "Swapchain.h"

#include "Log.h"
#include "PresentStatus.h"
#include "SwapchainChoices.h"

#include "device/Device.h"

#include <liara/extent.h>

#include <cstdint>
#include <format>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace Liara::Renderer
{
    Swapchain::Swapchain(const Device& device, const vk::raii::SurfaceKHR& surface, const liara_extent2d_t pixelSize)
        : m_Device(&device)
        , m_Surface(&surface) {
        Create(pixelSize, false);
    }

    SwapchainPlan Swapchain::Plan(const liara_extent2d_t windowSize, const bool invalid) const {
        const vk::SurfaceCapabilitiesKHR capabilities = m_Device->GetPhysical().getSurfaceCapabilitiesKHR(**m_Surface);
        const std::optional<vk::Extent2D> current = HasImages() ? std::optional(m_Extent) : std::nullopt;
        return PlanSwapchain(ChooseExtent(capabilities, windowSize), current, invalid);
    }

    void Swapchain::Recreate(const liara_extent2d_t windowSize) {
        // Idle first: every fence is then signalled and no semaphore of the old images is still awaited.
        m_Device->Get().waitIdle();
        Create(windowSize, true);
    }

    void Swapchain::Release() {
        m_Device->Get().waitIdle();
        m_RenderFinished.clear();
        m_Views.clear();
        m_Images.clear();
        m_Swapchain = nullptr;
    }

    bool Swapchain::HasImages() const noexcept { return !m_Images.empty(); }

    void Swapchain::Create(const liara_extent2d_t windowSize, const bool recreating) {
        const vk::raii::PhysicalDevice& physical = m_Device->GetPhysical();
        const vk::raii::SurfaceKHR& surface = *m_Surface;
        const vk::SurfaceCapabilitiesKHR capabilities = physical.getSurfaceCapabilitiesKHR(*surface);
        const std::vector<vk::SurfaceFormatKHR> formats = physical.getSurfaceFormatsKHR(*surface);

        const vk::SurfaceFormatKHR format = ChooseSurfaceFormat(formats);
        const vk::Extent2D extent = ChooseExtent(capabilities, windowSize);
        if (extent.width == 0 || extent.height == 0) {
            throw std::runtime_error("the window has no area, so no swapchain can be created on it");
        }

        // FIFO is the only mode every implementation offers; choosing another is a create-info field for later.
        constexpr vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
        const vk::SwapchainCreateInfoKHR createInfo({},
                                                    *surface,
                                                    ChooseImageCount(capabilities),
                                                    format.format,
                                                    format.colorSpace,
                                                    extent,
                                                    1,
                                                    vk::ImageUsageFlagBits::eColorAttachment,
                                                    vk::SharingMode::eExclusive,
                                                    {},
                                                    capabilities.currentTransform,
                                                    ChooseCompositeAlpha(capabilities.supportedCompositeAlpha),
                                                    presentMode,
                                                    vk::True,
                                                    *m_Swapchain);
        vk::raii::SwapchainKHR created = m_Device->Get().createSwapchainKHR(createInfo);

        // The old swapchain, its views and its semaphores go only now, once the new one exists.
        m_RenderFinished.clear();
        m_Views.clear();
        m_Swapchain = std::move(created);
        m_Format = format.format;
        m_Extent = extent;
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
            m_Views.push_back(m_Device->Get().createImageView(viewInfo));
            m_RenderFinished.push_back(m_Device->Get().createSemaphore({}));
        }

        Log::Info(std::format("{} {}x{}, {} images, {} {}, {}",
                              recreating ? "swapchain recreated" : "swapchain",
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
            // No timeout: FIFO hands an image back within a refresh or two. VK_SUBOPTIMAL_KHR is an acquired image.
            const auto [result, index] = m_Swapchain.acquireNextImage(UINT64_MAX, *imageAcquired);
            outIndex = index;
            return result == vk::Result::eSuboptimalKHR ? PresentStatus::Suboptimal : PresentStatus::Ok;
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
            // VK_SUBOPTIMAL_KHR is a presented image.
            const vk::Result result = queue.presentKHR(vk::PresentInfoKHR(renderFinished, swapchain, index));
            return result == vk::Result::eSuboptimalKHR ? PresentStatus::Suboptimal : PresentStatus::Ok;
        }
        catch (const vk::OutOfDateKHRError&) {
            return PresentStatus::OutOfDate;
        }
        catch (const vk::SurfaceLostKHRError&) {
            return PresentStatus::SurfaceLost;
        }
    }
}  // namespace Liara::Renderer
