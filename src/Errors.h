#pragma once

/**
 * @file Errors.h
 */

#include <liara/result.h>

namespace Liara::Renderer
{
    /**
     * @brief Translate the exception being handled into the result code the shim returns, after logging its message at
     * error level.
     *
     * Call it only from inside a `catch` block: it rethrows `std::current_exception()` to tell the types apart.
     * `Unsupported`, `vk::IncompatibleDriverError`, `vk::LayerNotPresentError` and `vk::ExtensionNotPresentError` give
     * `LIARA_RESULT_NOT_SUPPORTED`; `vk::OutOfHostMemoryError`, `vk::OutOfDeviceMemoryError` and `std::bad_alloc` give
     * `LIARA_RESULT_OUT_OF_MEMORY`; anything else gives `LIARA_RESULT_FAILED`.
     */
    [[nodiscard]] liara_result_t ResultFromCurrentException() noexcept;
}  // namespace Liara::Renderer
