#include "Renderer.h"

#include "Errors.h"

#include <liara/abi_version.h>
#include <liara/modules.h>
#include <liara/renderer/config.h>
#include <liara/renderer/packet.h>
#include <liara/renderer/renderer.h>
#include <liara/result.h>

#include <cstdint>
#include <memory>

struct liara_renderer_t
{ Liara::Renderer::Renderer m_Impl; };

static constexpr liara_module_info_t LIARA_RENDERER_MODULE_INFO = {
    .struct_version = LIARA_MODULE_INFO_VERSION,
    .abi_version = LIARA_ABI_VERSION,
    .module_version = LIARA_RENDERER_VERSION,
    .reserved = 0,
    .module_name = "Renderer",
    .abi_version_str = LIARA_ABI_VERSION_STR,
    .module_version_str = LIARA_RENDERER_VERSION_STRING,
};

const liara_module_info_t* liara_renderer_info(void) { return &LIARA_RENDERER_MODULE_INFO; }

uint32_t liara_renderer_abi_version(void) { return LIARA_RENDERER_MODULE_INFO.abi_version; }

// NOLINTBEGIN(cppcoreguidelines-owning-memory)
// NOLINTBEGIN(readability-identifier-naming)
liara_result_t liara_renderer_create(liara_renderer_handle_t** out_renderer) {
    // NOLINTEND(readability-identifier-naming)
    if (out_renderer == nullptr) { return LIARA_RESULT_NULL_POINTER; }

    // Constructing the renderer creates its Vulkan objects and throws on failure. Nothing unwinds through the ABI:
    // the exception becomes a result code, and *out_renderer is written only on success.
    try {
        auto renderer = std::make_unique<liara_renderer_handle_t>();
        *out_renderer = renderer.release();
        return LIARA_RESULT_SUCCESS;
    }
    catch (...) {
        return Liara::Renderer::ResultFromCurrentException();
    }
}  // NOLINTEND(cppcoreguidelines-owning-memory)

// NOLINTBEGIN(cppcoreguidelines-owning-memory)
// NOLINTBEGIN(readability-identifier-naming, misc-const-correctness)
void liara_renderer_destroy(liara_renderer_handle_t* renderer) {
    // NOLINTEND(readability-identifier-naming, misc-const-correctness)
    delete renderer;
}  // NOLINTEND(cppcoreguidelines-owning-memory)

// NOLINTBEGIN(readability-identifier-naming)
liara_result_t liara_renderer_submit_frame(liara_renderer_handle_t* renderer, const liara_render_packet_t* packet) {
    // NOLINTEND(readability-identifier-naming)
    if (renderer == nullptr || packet == nullptr) { return LIARA_RESULT_NULL_POINTER; }

    // The console renderer allocates its frame, so this can throw too.
    try {
        return renderer->m_Impl.SubmitFrame(*packet);
    }
    catch (...) {
        return Liara::Renderer::ResultFromCurrentException();
    }
}
