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
    class Meshes;
    class MeshPipeline;

    /**
     * @brief Records a packet's views into one colour image: the part of a frame that does not depend on where the
     * image comes from, so that the frames and the tests share it.
     *
     * The views targeting `LIARA_RENDER_TARGET_WINDOW` are drawn into the image it is given, in the packet's order. A
     * view on any other target is skipped, with one warning per target for the life of the object. A drawable whose
     * mesh is not uploaded is skipped, with one warning per identifier; an identifier is forgotten once its mesh is
     * found again, so that a mesh released later warns once more.
     */
    class ViewPass
    {
    public:
        /** @param meshes Outlives this object. */
        explicit ViewPass(const Meshes& meshes);

        /**
         * @brief Record, into `target`, which is already in `COLOR_ATTACHMENT_OPTIMAL`: one rendering scope over the
         * whole extent, cleared to opaque black, and, for each window view with a non-empty pixel rectangle, a clear of
         * that rectangle to the view's colour, then its viewport, flipped so that y goes up, its scissor, and one draw
         * per drawable.
         *
         * @param srgbTarget Whether the hardware encodes writes to the target to sRGB, which decides whether the
         *        packet's sRGB-encoded colours are decoded first.
         * @param pipeline Built for the target's format. Passed on each call rather than kept, since the swapchain's
         *        recreation may replace it.
         */
        void Record(const vk::raii::CommandBuffer& commands,
                    const vk::raii::ImageView& target,
                    vk::Extent2D extent,
                    bool srgbTarget,
                    std::span<const liara_render_view_t> views,
                    const MeshPipeline& pipeline);

    private:
        void Draw(const vk::raii::CommandBuffer& commands, const liara_render_drawable_t& drawable);

        const Meshes* m_Meshes;
        std::unordered_set<uint64_t> m_WarnedTargets;
        std::unordered_set<uint64_t> m_WarnedMeshes;
    };
}  // namespace Liara::Renderer
