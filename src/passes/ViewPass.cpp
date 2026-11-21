#include "ViewPass.h"

#include "Log.h"

#include "packet/Packet.h"
#include "swapchain/SwapchainChoices.h"

#include <liara/extent.h>
#include <liara/renderer/packet.h>

#include <array>
#include <cstdint>
#include <format>
#include <span>

namespace Liara::Renderer
{
    namespace
    {
        constexpr std::array<float, 4> OPAQUE_BLACK {0.0F, 0.0F, 0.0F, 1.0F};

        /**
         * The viewport for a rectangle, with a negative height: the contract's y goes up, Vulkan's framebuffer y goes
         * down, and a viewport starting at the rectangle's bottom edge with a negative height maps one onto the other
         * without touching the shaders.
         */
        vk::Viewport FlippedViewport(const PixelRect& rect) {
            return {static_cast<float>(rect.m_X),
                    static_cast<float>(rect.m_Y + rect.m_Height),
                    static_cast<float>(rect.m_Width),
                    -static_cast<float>(rect.m_Height),
                    0.0F,
                    1.0F};
        }

        vk::Rect2D ToRect2D(const PixelRect& rect) {
            return {vk::Offset2D(static_cast<int32_t>(rect.m_X), static_cast<int32_t>(rect.m_Y)),
                    vk::Extent2D(rect.m_Width, rect.m_Height)};
        }
    }  // namespace

    void ViewPass::Record(const vk::raii::CommandBuffer& commands,
                          const vk::raii::ImageView& target,
                          const vk::Extent2D extent,
                          const bool srgbTarget,
                          const std::span<const liara_render_view_t> views) {
        // Cleared to black first: the image arrives UNDEFINED, and whatever lies outside every view would be too.
        const vk::RenderingAttachmentInfo attachment(*target,
                                                     vk::ImageLayout::eColorAttachmentOptimal,
                                                     vk::ResolveModeFlagBits::eNone,
                                                     {},
                                                     vk::ImageLayout::eUndefined,
                                                     vk::AttachmentLoadOp::eClear,
                                                     vk::AttachmentStoreOp::eStore,
                                                     vk::ClearValue(vk::ClearColorValue(OPAQUE_BLACK)));
        commands.beginRendering(vk::RenderingInfo({}, vk::Rect2D({0, 0}, extent), 1, 0, attachment));

        const liara_extent2d_t size {.width = extent.width, .height = extent.height};
        for (const liara_render_view_t& view : views) {
            if (view.target != LIARA_RENDER_TARGET_WINDOW) {
                if (m_WarnedTargets.insert(view.target).second) {
                    Log::Warning(
                        std::format("a view targets render target {}, which does not exist; skipped", view.target));
                }
                continue;
            }

            // Vulkan forbids a clear rectangle of zero area and a viewport of zero height: an empty view draws nothing.
            const PixelRect rect = ToPixelRect(view.viewport, size);
            if (rect.m_Width == 0 || rect.m_Height == 0) { continue; }

            // In framebuffer coordinates, which the viewport's flip below does not affect.
            const vk::ClearAttachment clear(
                vk::ImageAspectFlagBits::eColor,
                0,
                vk::ClearValue(vk::ClearColorValue(LinearColorFromArgb(view.clear_color, srgbTarget))));
            commands.clearAttachments(clear, vk::ClearRect(ToRect2D(rect), 0, 1));
            commands.setViewport(0, FlippedViewport(rect));
            commands.setScissor(0, ToRect2D(rect));
        }

        commands.endRendering();
    }
}  // namespace Liara::Renderer
