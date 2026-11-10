#pragma once

/**
 * @file Unsupported.h
 */

#include <stdexcept>

namespace Liara::Renderer
{
    /**
     * @brief Thrown for every reason the renderer cannot run on this machine that it detects itself: no Vulkan loader,
     * a loader or a device below Vulkan 1.3, no suitable device, or, in a debug build, a missing validation layer.
     *
     * The shim turns it into `LIARA_RESULT_NOT_SUPPORTED`. The message says what is missing and, where installing
     * something fixes it, what to install.
     */
    class Unsupported : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };
}  // namespace Liara::Renderer
