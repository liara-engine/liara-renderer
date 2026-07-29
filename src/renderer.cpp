#include <liara/abi_version.h>
#include <liara/modules.h>
#include <liara/renderer/LiaraRenderer.h>
#include <liara/renderer/packet.h>
#include <liara/renderer/renderer.h>
#include <liara/result.h>
#include <liara/version.h>

#include <cstddef>
#include <cstdint>

#include <config.h>

struct liara_renderer_t
{
    mutable uint8_t m_Valid = 0;
    LiaraRenderer m_Impl;
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

uint32_t liara_renderer_version() {
    return LIARA_MAKE_VERSION_UNSAFE(LIARA_RENDERER_MAJOR_VERSION,
                                     LIARA_RENDERER_MINOR_VERSION,
                                     LIARA_RENDERER_PATCH_VERSION);
}

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

// TODO: Remove all the following functions when the renderer use ABI v0.2.0
//       All these functions are deprecated and will be removed in the future.
//       For now, they are kept, but they are now no-ops.

// NOLINTBEGIN(readability-identifier-naming)
liara_result_t liara_renderer_print(const liara_renderer_handle_t* renderer,
                                    const char* message,
                                    size_t message_length) {
    // NOLINTEND(readability-identifier-naming)
    if (renderer == nullptr || message == nullptr) { return LIARA_RESULT_NULL_POINTER; }
    if (message_length == 0) { return LIARA_RESULT_INVALID_ARGUMENT; }
    if (renderer->m_Valid != 1) { return LIARA_RESULT_INVALID_STATE; }
    return LIARA_RESULT_SUCCESS;
}

// NOLINTBEGIN(readability-identifier-naming)
liara_result_t liara_renderer_println(const liara_renderer_handle_t* renderer,
                                      const char* message,
                                      size_t message_length) {
    // NOLINTEND(readability-identifier-naming)
    if (renderer == nullptr || message == nullptr) { return LIARA_RESULT_NULL_POINTER; }
    if (message_length == 0) { return LIARA_RESULT_INVALID_ARGUMENT; }
    if (renderer->m_Valid != 1) { return LIARA_RESULT_INVALID_STATE; }
    return LIARA_RESULT_SUCCESS;
}

// NOLINTBEGIN(readability-identifier-naming)
void liara_renderer_set_text_color(const liara_renderer_handle_t* /*renderer*/, const uint32_t /*color*/) {
    // NOLINTEND(readability-identifier-naming)
}

// NOLINTBEGIN(readability-identifier-naming)
void liara_renderer_set_background_color(const liara_renderer_handle_t* /*renderer*/, const uint32_t /*color*/) {
    // NOLINTEND(readability-identifier-naming)
}
