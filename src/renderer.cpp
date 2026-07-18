#include "liara/abi_version.h"

#include <liara/renderer/renderer.h>
#include <liara/result.h>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

struct liara_renderer_t
{
    mutable uint8_t m_Valid = 0;
    mutable uint32_t m_TextColor = 0xFFFFFFFF;
    mutable uint32_t m_BackgroundColor = 0xFF000000;
    mutable std::string m_AnsiCode;
};

uint32_t liara_renderer_abi_version(void) { return LIARA_ABI_VERSION; }

// NOLINTBEGIN(cppcoreguidelines-owning-memory)
liara_result liara_renderer_create(liara_renderer_t** out_renderer) {
    if (out_renderer == nullptr) { return LIARA_RESULT_NULL_POINTER; }

    auto* renderer = new liara_renderer_t();
    renderer->m_Valid = 1;                     // Mark the renderer as valid
    renderer->m_TextColor = 0xFFFFFFFF;        // Default text color: white
    renderer->m_BackgroundColor = 0xFF000000;  // Default background color: black

    *out_renderer = renderer;
    return LIARA_RESULT_SUCCESS;
}  // NOLINTEND(cppcoreguidelines-owning-memory)

// NOLINTBEGIN(cppcoreguidelines-owning-memory)
liara_result liara_renderer_destroy(const liara_renderer_t* renderer) {
    if (renderer == nullptr) { return LIARA_RESULT_NULL_POINTER; }
    if (renderer->m_Valid != 1) { return LIARA_RESULT_INVALID_STATE; }
    renderer->m_Valid = 0;
    delete renderer;
    return LIARA_RESULT_SUCCESS;
}  // NOLINTEND(cppcoreguidelines-owning-memory)

liara_result liara_renderer_print(const liara_renderer_t* renderer, const char* message, size_t message_length) {
    if (renderer == nullptr || message == nullptr) { return LIARA_RESULT_NULL_POINTER; }
    if (message_length == 0) { return LIARA_RESULT_INVALID_ARGUMENT; }
    if (renderer->m_Valid != 1) { return LIARA_RESULT_INVALID_STATE; }
    std::cout << renderer->m_AnsiCode;
    std::cout << std::string(message, message_length) << '\n';
    std::cout << "\033[0m";  // Reset colors after printing
    return LIARA_RESULT_SUCCESS;
}

void liara_renderer_set_text_color(const liara_renderer_t* renderer, const uint32_t color) {
    // Basic error handling since it's a temporary test method, that will be rapidly removed in the future.
    if (renderer == nullptr) { return; }
    if (renderer->m_Valid != 1) { return; }

    renderer->m_TextColor = color;
    renderer->m_AnsiCode = "\033[38;2;" + std::to_string((color >> 16) & 0xFF) + ";"
                           + std::to_string((color >> 8) & 0xFF) + ";" + std::to_string(color & 0xFF) + "m"
                           + "\033[48;2;" + std::to_string((renderer->m_BackgroundColor >> 16) & 0xFF) + ";"
                           + std::to_string((renderer->m_BackgroundColor >> 8) & 0xFF) + ";"
                           + std::to_string(renderer->m_BackgroundColor & 0xFF) + "m";
}

void liara_renderer_set_background_color(const liara_renderer_t* renderer, const uint32_t color) {
    // Basic error handling since it's a temporary test method, that will be rapidly removed in the future.
    if (renderer == nullptr) { return; }
    if (renderer->m_Valid != 1) { return; }

    renderer->m_BackgroundColor = color;
    renderer->m_AnsiCode = "\033[38;2;" + std::to_string((renderer->m_TextColor >> 16) & 0xFF) + ";"
                           + std::to_string((renderer->m_TextColor >> 8) & 0xFF) + ";"
                           + std::to_string(renderer->m_TextColor & 0xFF) + "m" + "\033[48;2;"
                           + std::to_string((color >> 16) & 0xFF) + ";" + std::to_string((color >> 8) & 0xFF) + ";"
                           + std::to_string(color & 0xFF) + "m";
}
