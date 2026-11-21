#pragma once

/**
 * @file Packet.h
 */

#include <liara/extent.h>
#include <liara/renderer/packet.h>
#include <liara/result.h>

#include <cstdint>

namespace Liara::Renderer
{
    /** @brief A rectangle in pixels, origin top-left. */
    struct PixelRect
    {
        uint32_t m_X = 0;
        uint32_t m_Y = 0;
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;

        friend bool operator==(const PixelRect&, const PixelRect&) = default;
    };

    /** @brief How far past its target's edge a viewport may reach and still be accepted, then clamped. */
    inline constexpr float VIEWPORT_TOLERANCE = 1e-5F;

    /**
     * @brief Check a whole packet before anything of it is drawn, and return the first failure.
     *
     * The version comes first, and nothing past it is read before it passes. Then `views` against its count, and each
     * view in order: its target (not `LIARA_RENDER_TARGET_NONE`), its viewport (finite, within [0, 1], not reaching
     * past the target by more than the tolerance), its drawables against their count, and each drawable's mesh (not
     * `LIARA_MESH_ID_NONE`). A target or a mesh the renderer does not know is not a failure: it is skipped when drawn.
     * Nothing is logged.
     */
    [[nodiscard]] liara_result_t ValidatePacket(const liara_render_packet_t& packet) noexcept;

    /**
     * @brief The pixels a viewport `ValidatePacket` accepted covers in a target of the given size.
     *
     * Each edge is rounded on its own (`x0 = round(x * width)`, `x1 = round((x + w) * width)`, and likewise for y) and
     * clamped to the target, so that two viewports sharing an edge share a pixel boundary, with neither gap nor
     * overlap.
     */
    [[nodiscard]] PixelRect ToPixelRect(const liara_render_viewport_t& viewport, liara_extent2d_t target) noexcept;
}  // namespace Liara::Renderer
