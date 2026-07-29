#include <liara/abi_version.h>
#include <liara/modules.h>
#include <liara/renderer/LiaraRenderer.h>
#include <liara/renderer/packet.h>
#include <liara/renderer/renderer.h>
#include <liara/result.h>

#include <cstdint>

#include <config.h>

struct liara_renderer_t
{
    mutable uint8_t m_Valid = 0;
    Liara::Renderer::LiaraRenderer m_Impl;
};

static constexpr liara_module_info_t LIARA_RENDERER_MODULE_INFO = {
    .struct_version = LIARA_MODULE_INFO_VERSION,
    .abi_version = LIARA_ABI_VERSION,
    .module_version = LIARA_RENDERER_VERSION,
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

    auto* renderer = new liara_renderer_handle_t();
    renderer->m_Valid = 1;

    *out_renderer = renderer;
    return LIARA_RESULT_SUCCESS;
}  // NOLINTEND(cppcoreguidelines-owning-memory)

// NOLINTBEGIN(cppcoreguidelines-owning-memory)
// NOLINTBEGIN(readability-identifier-naming)
liara_result_t liara_renderer_destroy(const liara_renderer_handle_t* renderer) {
    // NOLINTEND(readability-identifier-naming)
    if (renderer == nullptr) { return LIARA_RESULT_NULL_POINTER; }
    if (renderer->m_Valid != 1) { return LIARA_RESULT_INVALID_STATE; }
    renderer->m_Valid = 0;
    delete renderer;
    return LIARA_RESULT_SUCCESS;
}  // NOLINTEND(cppcoreguidelines-owning-memory)

// NOLINTBEGIN(readability-identifier-naming)
liara_result_t liara_renderer_submit_frame(liara_renderer_handle_t* renderer, const liara_render_packet_t* packet) {
    // NOLINTEND(readability-identifier-naming)
    if (renderer == nullptr || packet == nullptr) { return LIARA_RESULT_NULL_POINTER; }
    if (renderer->m_Valid != 1) { return LIARA_RESULT_INVALID_STATE; }
    return renderer->m_Impl.SubmitFrame(*packet);
}
