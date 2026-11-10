#include "Renderer.h"

#include <liara/renderer/packet.h>
#include <liara/result.h>

namespace Liara::Renderer
{
    Renderer::Renderer() = default;

    Renderer::~Renderer() = default;

    liara_result_t Renderer::SubmitFrame(const liara_render_packet_t& packet) { return m_Console.SubmitFrame(packet); }
}  // namespace Liara::Renderer
