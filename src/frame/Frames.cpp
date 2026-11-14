#include "Frames.h"

#include "device/Device.h"
#include "swapchain/PresentStatus.h"
#include "swapchain/Swapchain.h"

#include <array>
#include <cstdint>
#include <utility>

namespace Liara::Renderer
{
    namespace
    {
        constexpr vk::ImageSubresourceRange COLOUR_RANGE(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

        vk::ImageMemoryBarrier2 Transition(const vk::Image image,
                                           const vk::PipelineStageFlags2 sourceStage,
                                           const vk::AccessFlags2 sourceAccess,
                                           const vk::PipelineStageFlags2 destinationStage,
                                           const vk::AccessFlags2 destinationAccess,
                                           const vk::ImageLayout from,
                                           const vk::ImageLayout to) {
            return {sourceStage,
                    sourceAccess,
                    destinationStage,
                    destinationAccess,
                    from,
                    to,
                    VK_QUEUE_FAMILY_IGNORED,
                    VK_QUEUE_FAMILY_IGNORED,
                    image,
                    COLOUR_RANGE};
        }
    }  // namespace

    Frames::Frames(const Device& device, Swapchain& swapchain)
        : m_Device(&device)
        , m_Swapchain(&swapchain) {
        vk::raii::CommandBuffers commands(device.Get(),
                                          vk::CommandBufferAllocateInfo(*device.GetCommandPool(),
                                                                        vk::CommandBufferLevel::ePrimary,
                                                                        FRAMES_IN_FLIGHT));
        m_Frames.reserve(FRAMES_IN_FLIGHT);
        for (vk::raii::CommandBuffer& buffer : commands) {
            // Created signalled, so that the first wait on each frame returns at once.
            m_Frames.push_back(Frame {
                .m_Commands = std::move(buffer),
                .m_InFlight = device.Get().createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled)),
                .m_ImageAcquired = device.Get().createSemaphore({}),
            });
        }
    }

    PresentStatus Frames::Draw(const std::array<float, 4>& clearColor) {
        const Frame& frame = m_Frames.at(m_Current);
        const vk::raii::Device& device = m_Device->Get();

        (void)device.waitForFences(*frame.m_InFlight, vk::True, UINT64_MAX);

        uint32_t imageIndex = 0;
        if (const PresentStatus acquired = m_Swapchain->Acquire(frame.m_ImageAcquired, imageIndex);
            acquired != PresentStatus::Ok) {
            return acquired;
        }
        // Reset only now: a fence reset before an acquisition that then failed would never be signalled again, and the
        // next wait on this frame would block forever.
        device.resetFences(*frame.m_InFlight);

        Record(frame, imageIndex, clearColor);

        const vk::SemaphoreSubmitInfo wait(*frame.m_ImageAcquired,
                                           0,
                                           vk::PipelineStageFlagBits2::eColorAttachmentOutput);
        const vk::CommandBufferSubmitInfo commands(*frame.m_Commands);
        // All commands, not only the colour output: the last barrier's layout transition to PRESENT_SRC_KHR is a write
        // too, and the presentation that waits on this semaphore has to see it finished.
        const vk::SemaphoreSubmitInfo signal(*m_Swapchain->GetRenderFinished(imageIndex),
                                             0,
                                             vk::PipelineStageFlagBits2::eAllCommands);
        m_Device->GetQueue().submit2(vk::SubmitInfo2({}, wait, commands, signal), *frame.m_InFlight);

        m_Current = (m_Current + 1) % FRAMES_IN_FLIGHT;
        return m_Swapchain->Present(m_Device->GetQueue(), imageIndex);
    }

    void Frames::Record(const Frame& frame, const uint32_t imageIndex, const std::array<float, 4>& clearColor) const {
        const vk::raii::CommandBuffer& commands = frame.m_Commands;
        const vk::Image image = m_Swapchain->GetImage(imageIndex);
        using Stage = vk::PipelineStageFlagBits2;
        using Access = vk::AccessFlagBits2;

        commands.reset();
        commands.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        // The previous content is discarded (UNDEFINED), and the transition waits for the acquisition, which the
        // submission's semaphore wait places at the same stage.
        const vk::ImageMemoryBarrier2 toAttachment = Transition(image,
                                                                Stage::eColorAttachmentOutput,
                                                                Access::eNone,
                                                                Stage::eColorAttachmentOutput,
                                                                Access::eColorAttachmentWrite,
                                                                vk::ImageLayout::eUndefined,
                                                                vk::ImageLayout::eColorAttachmentOptimal);
        commands.pipelineBarrier2(vk::DependencyInfo({}, {}, {}, toAttachment));

        const vk::RenderingAttachmentInfo attachment(*m_Swapchain->GetView(imageIndex),
                                                     vk::ImageLayout::eColorAttachmentOptimal,
                                                     vk::ResolveModeFlagBits::eNone,
                                                     {},
                                                     vk::ImageLayout::eUndefined,
                                                     vk::AttachmentLoadOp::eClear,
                                                     vk::AttachmentStoreOp::eStore,
                                                     vk::ClearValue(vk::ClearColorValue(clearColor)));
        commands.beginRendering(vk::RenderingInfo({}, vk::Rect2D({0, 0}, m_Swapchain->GetExtent()), 1, 0, attachment));
        commands.endRendering();

        const vk::ImageMemoryBarrier2 toPresent = Transition(image,
                                                             Stage::eColorAttachmentOutput,
                                                             Access::eColorAttachmentWrite,
                                                             Stage::eNone,
                                                             Access::eNone,
                                                             vk::ImageLayout::eColorAttachmentOptimal,
                                                             vk::ImageLayout::ePresentSrcKHR);
        commands.pipelineBarrier2(vk::DependencyInfo({}, {}, {}, toPresent));

        commands.end();
    }
}  // namespace Liara::Renderer
