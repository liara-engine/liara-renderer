#include "Renderer.h"

#include "device/Instance.h"
#include "device/Loader.h"

#include <liara/renderer/packet.h>
#include <liara/result.h>

#include <memory>

namespace Liara::Renderer
{
    Renderer::Renderer()
        : m_Loader(std::make_unique<Loader>())
        , m_Instance(std::make_unique<Instance>(*m_Loader)) {}

    Renderer::~Renderer() = default;

    liara_result_t Renderer::SubmitFrame(const liara_render_packet_t& packet) { return m_Console.SubmitFrame(packet); }
}  // namespace Liara::Renderer
