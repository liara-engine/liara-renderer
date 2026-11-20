#pragma once

/**
 * @file Terminal.h
 */

namespace Liara::Renderer
{
    /**
     * @brief True when stdout and stderr are both terminals.
     *
     * Only then does the console renderer hold the renderer's messages while it covers the screen: with either stream
     * redirected, the messages cannot be overwritten, and writing them at once keeps them through a crash.
     */
    [[nodiscard]] bool StandardStreamsAreTerminals() noexcept;
}  // namespace Liara::Renderer
