#include "Renderer.h"

#include "CreateInfo.h"
#include "Log.h"

#include "console/Terminal.h"
#include "device/Allocator.h"
#include "device/Device.h"
#include "device/Instance.h"
#include "device/Loader.h"
#include "frame/Frames.h"
#include "swapchain/PresentStatus.h"
#include "swapchain/Surface.h"
#include "swapchain/Swapchain.h"
#include "swapchain/SwapchainChoices.h"

#include <liara/native_window.h>
#include <liara/renderer/packet.h>
#include <liara/result.h>

#include <exception>
#include <memory>
#include <string>

namespace Liara::Renderer
{
    Renderer::Renderer(const RendererConfig& config)
        : m_Loader(std::make_unique<Loader>())
        , m_Instance(std::make_unique<Instance>(*m_Loader, config.m_Window.kind))
        , m_Surface(config.m_Window.kind == LIARA_NATIVE_WINDOW_KIND_NONE
                        ? nullptr
                        : std::make_unique<Surface>(m_Instance->Get(), config.m_Window))
        , m_Device(std::make_unique<Device>(m_Instance->Get(),
                                            m_Surface ? &m_Surface->Get() : nullptr,
                                            config.m_PreferredDeviceName))
        , m_Allocator(std::make_unique<Allocator>(*m_Loader, *m_Instance, *m_Device))
        , m_Swapchain(m_Surface ? std::make_unique<Swapchain>(*m_Device, m_Surface->Get(), config.m_PixelSize)
                                : nullptr)
        , m_Frames(m_Swapchain ? std::make_unique<Frames>(*m_Device, *m_Swapchain) : nullptr)
        , m_Console(StandardStreamsAreTerminals()) {}

    Renderer::~Renderer() {
        // Frames may still be in flight; nothing they use can go before the GPU is done with it.
        try {
            m_Device->Get().waitIdle();
        }
        catch (const std::exception& error) {
            Log::Error(std::string("waiting for the GPU before destruction failed: ").append(error.what()));
        }
    }

    liara_result_t Renderer::SubmitFrame(const liara_render_packet_t& packet) {
        if (const liara_result_t drawn = m_Console.SubmitFrame(packet); drawn != LIARA_RESULT_SUCCESS) { return drawn; }
        if (!m_Frames) { return LIARA_RESULT_SUCCESS; }

        // Sticky until a later stage recreates the swapchain, which also keeps the log line below to one.
        if (m_PresentStatus == PresentStatus::Ok) {
            m_PresentStatus =
                m_Frames->Draw(ClearColorFromPacket(packet.background_color, IsSrgbFormat(m_Swapchain->GetFormat())));
            if (m_PresentStatus == PresentStatus::OutOfDate) {
                Log::Info("swapchain out of date; resizing is handled from stage 6");
            }
            else if (m_PresentStatus == PresentStatus::SurfaceLost) {
                Log::Warning("surface lost; presentation stops");
            }
        }

        switch (m_PresentStatus) {
            case PresentStatus::Ok: return LIARA_RESULT_SUCCESS;
            case PresentStatus::OutOfDate: return LIARA_RESULT_SURFACE_OUT_OF_DATE;
            case PresentStatus::SurfaceLost: return LIARA_RESULT_SURFACE_LOST;
        }
        return LIARA_RESULT_FAILED;
    }
}  // namespace Liara::Renderer
