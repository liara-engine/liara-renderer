#include "Errors.h"

#include "Log.h"
#include "Unsupported.h"

#include <liara/result.h>

#include <vulkan/vulkan.hpp>

#include <exception>
#include <new>

namespace Liara::Renderer
{
    liara_result_t ResultFromCurrentException() noexcept {
        try {
            throw;
        }
        catch (const Unsupported& error) {
            Log::Error(error.what());
            return LIARA_RESULT_NOT_SUPPORTED;
        }
        // No driver, or a layer or extension the renderer asked for: this machine cannot run it.
        catch (const vk::IncompatibleDriverError& error) {
            Log::Error(error.what());
            return LIARA_RESULT_NOT_SUPPORTED;
        }
        catch (const vk::LayerNotPresentError& error) {
            Log::Error(error.what());
            return LIARA_RESULT_NOT_SUPPORTED;
        }
        catch (const vk::ExtensionNotPresentError& error) {
            Log::Error(error.what());
            return LIARA_RESULT_NOT_SUPPORTED;
        }
        catch (const vk::OutOfHostMemoryError& error) {
            Log::Error(error.what());
            return LIARA_RESULT_OUT_OF_MEMORY;
        }
        catch (const vk::OutOfDeviceMemoryError& error) {
            Log::Error(error.what());
            return LIARA_RESULT_OUT_OF_MEMORY;
        }
        catch (const std::bad_alloc& error) {
            Log::Error(error.what());
            return LIARA_RESULT_OUT_OF_MEMORY;
        }
        catch (const std::exception& error) {
            Log::Error(error.what());
            return LIARA_RESULT_FAILED;
        }
        catch (...) {
            Log::Error("unknown exception");
            return LIARA_RESULT_FAILED;
        }
    }
}  // namespace Liara::Renderer
