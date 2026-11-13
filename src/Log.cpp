// TEMPORARY: see Log.h. Replaced when the core's logger exists (v0.2), together with the ABI path that reaches it.

#include "Log.h"

#include <cstdio>
#include <string>
#include <string_view>

namespace Liara::Renderer::Log
{
    namespace
    {
        constexpr std::string_view PREFIX = "[liara-renderer] ";

        void Write(const std::string_view level, const std::string_view message) noexcept {
            // One fwrite per line, so that two threads logging at once cannot interleave inside a line. Building the
            // line can only fail on allocation, and then the three parts go out separately rather than not at all.
            try {
                std::string line;
                line.reserve(PREFIX.size() + level.size() + 2 + message.size() + 1);
                line.append(PREFIX).append(level).append(": ").append(message).push_back('\n');
                (void)std::fwrite(line.data(), 1, line.size(), stderr);
            }
            catch (...) {
                (void)std::fwrite(PREFIX.data(), 1, PREFIX.size(), stderr);
                (void)std::fwrite(level.data(), 1, level.size(), stderr);
                (void)std::fputs(": ", stderr);
                (void)std::fwrite(message.data(), 1, message.size(), stderr);
                (void)std::fputc('\n', stderr);
            }
        }
    }  // namespace

    void Info(const std::string_view message) noexcept { Write("info", message); }

    void Warning(const std::string_view message) noexcept { Write("warning", message); }

    void Error(const std::string_view message) noexcept { Write("error", message); }
}  // namespace Liara::Renderer::Log
