#include "Errors.h"

#include "Log.h"
#include "Unsupported.h"

#include <liara/result.h>

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
