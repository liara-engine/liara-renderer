#pragma once

/**
 * @file ViewPass.h
 */

#include <liara/renderer/packet.h>

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <span>
#include <unordered_set>

namespace Liara::Renderer
{
    /**
     * @brief Records a packet's views into one colour image: the part of a frame that does not depend on where the
     * image comes from, so that the frames and the tests share it.
     *
     * The views targeting `LIARA_RENDER_TARGET_WINDOW` are drawn into the image it is given, in the packet's order. A
     * view on any other target is skipped, with one warning per target for the life of the object.
     */
    class ViewPass
    {
    public:
        /**
         * @brief Record, into `target`, which is already in `COLOR_ATTACHMENT_OPTIMAL`: one rendering scope over the
         * whole extent, cleared to opaque black, and, for each window view with a non-empty pixel rectangle, a clear of
         * that rectangle to the view's colour, then its viewport, flipped so that y goes up, and its scissor.
         *
         * @param srgbTarget Whether the hardware encodes writes to the target to sRGB, which decides whether the
         *        packet's sRGB-encoded colours are decoded first.
         */
        void Record(const vk::raii::CommandBuffer& commands,
                    const vk::raii::ImageView& target,
                    vk::Extent2D extent,
                    bool srgbTarget,
                    std::span<const liara_render_view_t> views);

    private:
        std::unordered_set<uint64_t> m_WarnedTargets;
    };
}  // namespace Liara::Renderer
