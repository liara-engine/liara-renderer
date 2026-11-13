#pragma once

/**
 * @file Log.h
 *
 * TEMPORARY. This sink stands in for two things that do not exist yet: the core's logger (v0.2) and the diagnostics
 * callback `liara-interfaces/docs/errors.md` describes for asynchronous Vulkan messages. Every message the renderer
 * emits goes through these functions, so replacing the sink changes this file and Log.cpp and no call site.
 *
 * Each call writes one line, `[liara-renderer] <level>: <message>`, to stderr. While the console demo covers the
 * terminal, the console renderer holds the lines instead, and they are written when it leaves the terminal's alternate
 * screen; a crash in between loses them, which is why the hold is only used when stdout and stderr are both terminals.
 */

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string_view>

namespace Liara::Renderer::Log
{
    /** @brief How many lines a hold keeps. Beyond it lines are dropped, and the release says how many. */
    inline constexpr std::size_t HOLD_CAPACITY = 1000;

    /** @brief Warnings and errors reported since the process started, held or not. */
    struct Counts
    {
        uint64_t m_Warnings = 0;
        uint64_t m_Errors = 0;

        friend bool operator==(const Counts&, const Counts&) = default;
    };

    /** @brief Report something worth knowing that is not a problem, such as the GPU the renderer selected. */
    void Info(std::string_view message) noexcept;

    /** @brief Report a problem the renderer works around, and every validation warning. */
    void Warning(std::string_view message) noexcept;

    /** @brief Report a failure, and every validation error. */
    void Error(std::string_view message) noexcept;

    /** @brief Keep lines in memory instead of writing them. Holds nest: lines are written once every hold is released.
     */
    void Hold() noexcept;

    /**
     * @brief Release one hold. Releasing the last writes the held lines in order, then, if any were dropped, one
     * `warning` line saying how many. Without a hold, does nothing.
     */
    void Release() noexcept;

    /** @brief The counts so far. The line reporting dropped lines is not counted. */
    [[nodiscard]] Counts GetCounts() noexcept;

    /** @brief Where lines go from now on; null restores stderr. For tests. */
    void SetStream(std::FILE* stream) noexcept;
}  // namespace Liara::Renderer::Log
