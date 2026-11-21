// TEMPORARY: see Log.h. Replaced when the core's logger exists (v0.2), together with the ABI path that reaches it.

#include "Log.h"

#include <cstdint>
#include <cstdio>
#include <mutex>
#include <string>
#include <string_view>

namespace Liara::Renderer::Log
{
    namespace
    {
        constexpr std::string_view PREFIX = "[liara-renderer] ";

        enum class Level : std::uint8_t
        {
            Info,
            Warning,
            Error
        };

        constexpr std::string_view LevelName(const Level level) noexcept {
            switch (level) {
                case Level::Info: return "info";
                case Level::Warning: return "warning";
                case Level::Error: return "error";
            }
            return "info";
        }

        /** Everything the sink shares between threads; the debug messenger may call it from a driver thread. */
        struct State
        {
            std::mutex m_Mutex;
            std::FILE* m_Stream = nullptr;  ///< Null: stderr.
        };

        State& GetState() noexcept {
            static State state;
            return state;
        }

        std::FILE* Out(const State& state) noexcept { return state.m_Stream != nullptr ? state.m_Stream : stderr; }

        void WriteLine(std::FILE* out, const std::string_view level, const std::string_view message) noexcept {
            // One fwrite per line, so that two threads logging at once cannot interleave inside a line. Building the
            // line can only fail on allocation, and then the three parts go out separately rather than not at all.
            try {
                std::string line;
                line.reserve(PREFIX.size() + level.size() + 2 + message.size() + 1);
                line.append(PREFIX).append(level).append(": ").append(message).push_back('\n');
                (void)std::fwrite(line.data(), 1, line.size(), out);
            }
            catch (...) {
                (void)std::fwrite(PREFIX.data(), 1, PREFIX.size(), out);
                (void)std::fwrite(level.data(), 1, level.size(), out);
                (void)std::fputs(": ", out);
                (void)std::fwrite(message.data(), 1, message.size(), out);
                (void)std::fputc('\n', out);
            }
        }

        void Emit(const Level level, const std::string_view message) noexcept {
            State& state = GetState();
            try {
                const std::scoped_lock lock(state.m_Mutex);
                WriteLine(Out(state), LevelName(level), message);
            }
            catch (...) {
                // A lock that cannot be taken: write the line anyway rather than lose it.
                WriteLine(Out(state), LevelName(level), message);
            }
        }
    }  // namespace

    void Info(const std::string_view message) noexcept { Emit(Level::Info, message); }

    void Warning(const std::string_view message) noexcept { Emit(Level::Warning, message); }

    void Error(const std::string_view message) noexcept { Emit(Level::Error, message); }

    // std::mutex::lock throws only on a system error that a valid, non-recursive use never meets. Were it to, noexcept
    // turns it into termination, which is the honest outcome for a mutex that no longer works.
    void SetStream(std::FILE* stream) noexcept {
        State& state = GetState();
        const std::scoped_lock lock(state.m_Mutex);
        state.m_Stream = stream;
    }
}  // namespace Liara::Renderer::Log
