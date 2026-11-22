#include "renderer/Renderer.h"

#include "Errors.h"

#include "mesh/MeshData.h"
#include "renderer/CreateInfo.h"

#include <liara/abi_version.h>
#include <liara/extent.h>
#include <liara/mesh.h>
#include <liara/modules.h>
#include <liara/renderer/config.h>
#include <liara/renderer/packet.h>
#include <liara/renderer/renderer.h>
#include <liara/result.h>

#include <cstdint>
#include <memory>

struct liara_renderer_t
{
    explicit liara_renderer_t(const Liara::Renderer::RendererConfig& config)
        : m_Impl(config) {}

    Liara::Renderer::Renderer m_Impl;
};

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
liara_result_t liara_renderer_create(const liara_renderer_create_info_t* create_info,
                                     liara_renderer_handle_t** out_renderer) {
    // NOLINTEND(readability-identifier-naming)
    if (create_info == nullptr || out_renderer == nullptr) { return LIARA_RESULT_NULL_POINTER; }

    // Every argument is checked before anything opens the Vulkan loader, and a failure here logs nothing.
    if (const liara_result_t checked = Liara::Renderer::CheckCreateInfo(*create_info);
        checked != LIARA_RESULT_SUCCESS) {
        return checked;
    }

    // Constructing the renderer creates its Vulkan objects and throws on failure. Nothing unwinds through the ABI:
    // the exception becomes a result code, and *out_renderer is written only on success.
    try {
        auto renderer = std::make_unique<liara_renderer_handle_t>(Liara::Renderer::ToConfig(*create_info));
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

    // Recording and presenting call Vulkan, whose errors arrive as exceptions; none unwinds through the ABI.
    try {
        return renderer->m_Impl.SubmitFrame(*packet);
    }
    catch (...) {
        return Liara::Renderer::ResultFromCurrentException();
    }
}

// NOLINTBEGIN(readability-identifier-naming)
liara_result_t liara_renderer_mesh_upload(liara_renderer_handle_t* renderer,
                                          liara_mesh_id_t mesh,
                                          const liara_mesh_data_t* data) {
    // NOLINTEND(readability-identifier-naming)
    if (renderer == nullptr || data == nullptr) { return LIARA_RESULT_NULL_POINTER; }
    if (const liara_result_t checked = Liara::Renderer::ValidateMeshData(mesh, *data);
        checked != LIARA_RESULT_SUCCESS) {
        return checked;
    }
    // The upload calls Vulkan, whose errors arrive as exceptions; none unwinds through the ABI.
    try {
        return renderer->m_Impl.UploadMesh(mesh, *data);
    }
    catch (...) {
        return Liara::Renderer::ResultFromCurrentException();
    }
}

// NOLINTBEGIN(readability-identifier-naming)
liara_result_t liara_renderer_mesh_release(liara_renderer_handle_t* renderer, liara_mesh_id_t mesh) {
    // NOLINTEND(readability-identifier-naming)
    if (renderer == nullptr) { return LIARA_RESULT_NULL_POINTER; }
    if (mesh == LIARA_MESH_ID_NONE) { return LIARA_RESULT_INVALID_ARGUMENT; }
    try {
        return renderer->m_Impl.ReleaseMesh(mesh);
    }
    catch (...) {
        return Liara::Renderer::ResultFromCurrentException();
    }
}

// NOLINTBEGIN(readability-identifier-naming)
liara_result_t liara_renderer_window_resized(liara_renderer_handle_t* renderer, liara_extent2d_t pixel_size) {
    // NOLINTEND(readability-identifier-naming)
    if (renderer == nullptr) { return LIARA_RESULT_NULL_POINTER; }
    // Only kept until the next frame, which is where the swapchain follows it: nothing here can fail or throw.
    renderer->m_Impl.WindowResized(pixel_size);
    return LIARA_RESULT_SUCCESS;
}
