// TEMPORARY: see Log.h. Replaced when the core's logger exists (v0.2), together with the ABI path that reaches it.

#include "Log.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <format>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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
            std::size_t m_HoldDepth = 0;
            std::vector<std::string> m_Held;
            std::size_t m_Dropped = 0;
            Counts m_Counts;
        };

        State& GetState() noexcept {
            static State state;
            return state;
        }

        std::FILE* Out(const State& state) noexcept { return state.m_Stream != nullptr ? state.m_Stream : stderr; }

        std::string FormatLine(const std::string_view level, const std::string_view message) {
            std::string line;
            line.reserve(PREFIX.size() + level.size() + 2 + message.size() + 1);
            line.append(PREFIX).append(level).append(": ").append(message).push_back('\n');
            return line;
        }

        void WriteLine(std::FILE* out, const std::string_view level, const std::string_view message) noexcept {
            // One fwrite per line, so that two threads logging at once cannot interleave inside a line. Building the
            // line can only fail on allocation, and then the three parts go out separately rather than not at all.
            try {
                const std::string line = FormatLine(level, message);
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
                if (level == Level::Warning) { ++state.m_Counts.m_Warnings; }
                if (level == Level::Error) { ++state.m_Counts.m_Errors; }
                if (state.m_HoldDepth == 0) {
                    WriteLine(Out(state), LevelName(level), message);
                    return;
                }
                if (state.m_Held.size() < HOLD_CAPACITY) {
                    state.m_Held.push_back(FormatLine(LevelName(level), message));
                }
                else { ++state.m_Dropped; }
            }
            catch (...) {
                // A lock that cannot be taken, or a line that cannot be kept: write it now rather than lose it.
                WriteLine(Out(state), LevelName(level), message);
            }
        }
    }  // namespace

    void Info(const std::string_view message) noexcept { Emit(Level::Info, message); }

    void Warning(const std::string_view message) noexcept { Emit(Level::Warning, message); }

    void Error(const std::string_view message) noexcept { Emit(Level::Error, message); }

    // std::mutex::lock throws only on a system error that a valid, non-recursive use never meets. Were it to, noexcept
    // turns it into termination, which is the honest outcome for a mutex that no longer works.

    void Hold() noexcept {
        State& state = GetState();
        const std::scoped_lock lock(state.m_Mutex);
        ++state.m_HoldDepth;
    }

    void Release() noexcept {
        State& state = GetState();
        const std::scoped_lock lock(state.m_Mutex);
        if (state.m_HoldDepth == 0) { return; }
        --state.m_HoldDepth;
        if (state.m_HoldDepth > 0) { return; }

        const std::vector<std::string> held = std::exchange(state.m_Held, {});
        const std::size_t dropped = std::exchange(state.m_Dropped, 0);
        std::FILE* out = Out(state);
        for (const std::string& line : held) { (void)std::fwrite(line.data(), 1, line.size(), out); }
        if (dropped > 0) {
            try {
                WriteLine(out,
                          LevelName(Level::Warning),
                          std::format("{} further messages were dropped while the console demo covered the terminal",
                                      dropped));
            }
            catch (...) {
                WriteLine(out,
                          LevelName(Level::Warning),
                          "further messages were dropped while the console demo covered the terminal");
            }
        }
        (void)std::fflush(out);
    }

    Counts GetCounts() noexcept {
        State& state = GetState();
        const std::scoped_lock lock(state.m_Mutex);
        return state.m_Counts;
    }

    void SetStream(std::FILE* stream) noexcept {
        State& state = GetState();
        const std::scoped_lock lock(state.m_Mutex);
        state.m_Stream = stream;
    }
}  // namespace Liara::Renderer::Log
