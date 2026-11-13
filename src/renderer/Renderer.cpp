#include "Renderer.h"

#include "CreateInfo.h"

#include "console/Terminal.h"
#include "device/Allocator.h"
#include "device/Device.h"
#include "device/Instance.h"
#include "device/Loader.h"

#include <liara/renderer/packet.h>
#include <liara/result.h>

#include <memory>

namespace Liara::Renderer
{
    // The configuration is read once the device layer presents to a window.
    Renderer::Renderer([[maybe_unused]] const RendererConfig& config)
        : m_Loader(std::make_unique<Loader>())
        , m_Instance(std::make_unique<Instance>(*m_Loader))
        , m_Device(std::make_unique<Device>(m_Instance->Get()))
        , m_Allocator(std::make_unique<Allocator>(*m_Loader, *m_Instance, *m_Device))
        , m_Console(StandardStreamsAreTerminals()) {}

    Renderer::~Renderer() = default;

    liara_result_t Renderer::SubmitFrame(const liara_render_packet_t& packet) { return m_Console.SubmitFrame(packet); }
}  // namespace Liara::Renderer
