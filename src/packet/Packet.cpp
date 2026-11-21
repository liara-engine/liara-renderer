#include "Packet.h"

#include <liara/extent.h>
#include <liara/mesh.h>
#include <liara/renderer/packet.h>
#include <liara/result.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>

namespace Liara::Renderer
{
    namespace
    {
        bool IsWithinTarget(const liara_render_viewport_t& viewport) noexcept {
            const auto inUnit = [](const float value) {
                return std::isfinite(value) && value >= 0.0F && value <= 1.0F;
            };
            return inUnit(viewport.x) && inUnit(viewport.y) && inUnit(viewport.width) && inUnit(viewport.height)
                   && viewport.x + viewport.width <= 1.0F + VIEWPORT_TOLERANCE
                   && viewport.y + viewport.height <= 1.0F + VIEWPORT_TOLERANCE;
        }

        liara_result_t ValidateView(const liara_render_view_t& view) noexcept {
            if (view.target == LIARA_RENDER_TARGET_NONE) { return LIARA_RESULT_INVALID_ARGUMENT; }
            if (!IsWithinTarget(view.viewport)) { return LIARA_RESULT_INVALID_ARGUMENT; }
            if (view.drawables == nullptr && view.drawable_count > 0) { return LIARA_RESULT_NULL_POINTER; }
            if (view.drawable_count == 0) { return LIARA_RESULT_SUCCESS; }
            for (const liara_render_drawable_t& drawable : std::span(view.drawables, view.drawable_count)) {
                if (drawable.mesh == LIARA_MESH_ID_NONE) { return LIARA_RESULT_INVALID_ARGUMENT; }
            }
            return LIARA_RESULT_SUCCESS;
        }

        /** The pixel boundary nearest to `fraction` of `size`, clamped to [0, size]. */
        uint32_t Edge(const float fraction, const uint32_t size) noexcept {
            const double pixels = std::round(static_cast<double>(fraction) * static_cast<double>(size));
            return static_cast<uint32_t>(std::clamp(pixels, 0.0, static_cast<double>(size)));
        }
    }  // namespace

    liara_result_t ValidatePacket(const liara_render_packet_t& packet) noexcept {
        if (packet.struct_version != LIARA_RENDER_PACKET_VERSION) { return LIARA_RESULT_VERSION_MISMATCH; }
        if (packet.views == nullptr && packet.view_count > 0) { return LIARA_RESULT_NULL_POINTER; }
        if (packet.view_count == 0) { return LIARA_RESULT_SUCCESS; }
        for (const liara_render_view_t& view : std::span(packet.views, packet.view_count)) {
            if (const liara_result_t checked = ValidateView(view); checked != LIARA_RESULT_SUCCESS) { return checked; }
        }
        return LIARA_RESULT_SUCCESS;
    }

    PixelRect ToPixelRect(const liara_render_viewport_t& viewport, const liara_extent2d_t target) noexcept {
        const uint32_t left = Edge(viewport.x, target.width);
        const uint32_t top = Edge(viewport.y, target.height);
        const uint32_t right = std::max(left, Edge(viewport.x + viewport.width, target.width));
        const uint32_t bottom = std::max(top, Edge(viewport.y + viewport.height, target.height));
        return {.m_X = left, .m_Y = top, .m_Width = right - left, .m_Height = bottom - top};
    }
}  // namespace Liara::Renderer
