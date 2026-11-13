#include "Terminal.h"

// fileno and _fileno are POSIX and Microsoft extensions, declared by <stdio.h> and not by <cstdio>.
#include <stdio.h>  // NOLINT(modernize-deprecated-headers)

#ifdef _WIN32
    #include <io.h>
#else
    #include <unistd.h>
#endif

namespace Liara::Renderer
{
    bool StandardStreamsAreTerminals() noexcept {
#ifdef _WIN32
        return _isatty(_fileno(stdout)) != 0 && _isatty(_fileno(stderr)) != 0;
#else
        return isatty(fileno(stdout)) != 0 && isatty(fileno(stderr)) != 0;
#endif
    }
}  // namespace Liara::Renderer
