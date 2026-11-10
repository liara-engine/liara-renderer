#pragma once

/**
 * @file Log.h
 *
 * TEMPORARY. This sink stands in for two things that do not exist yet: the core's logger (v0.2) and the diagnostics
 * callback `liara-interfaces/docs/errors.md` describes for asynchronous Vulkan messages. Every message the renderer
 * emits goes through these three functions, so replacing the sink changes this file and Log.cpp and no call site.
 *
 * Each call writes one line, `[liara-renderer] <level>: <message>`, to stderr.
 */

#include <string_view>

namespace Liara::Renderer::Log
{
    /** @brief Report something worth knowing that is not a problem, such as the GPU the renderer selected. */
    void Info(std::string_view message) noexcept;

    /** @brief Report a problem the renderer works around, and every validation warning. */
    void Warning(std::string_view message) noexcept;

    /** @brief Report a failure, and every validation error. */
    void Error(std::string_view message) noexcept;
}  // namespace Liara::Renderer::Log
