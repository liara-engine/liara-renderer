// NOLINTBEGIN(readability-identifier-naming)
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "Log.h"
#include "SinkCapture.h"

#include <algorithm>
#include <cstddef>
#include <string>

#include <doctest/doctest.h>

// The sink alone, compiled into this test from src/Log.cpp: no renderer, no GPU. Each case routes it into a temporary
// file and reads what it wrote.

namespace Log = Liara::Renderer::Log;

namespace
{
    size_t LineCount(const std::string& text) { return static_cast<size_t>(std::ranges::count(text, '\n')); }
}  // namespace

TEST_CASE("Log - a line is written at once when nothing holds it") {
    const SinkCapture sink;
    Log::Info("direct");
    CHECK(sink.Written() == "[liara-renderer] info: direct\n");
}

TEST_CASE("Log - a held line is written on release, in order") {
    const SinkCapture sink;
    Log::Hold();
    Log::Warning("first");
    Log::Error("second");
    CHECK(sink.Written().empty());
    Log::Release();
    CHECK(sink.Written() == "[liara-renderer] warning: first\n[liara-renderer] error: second\n");
}

TEST_CASE("Log - holds nest, and lines are written when the last one is released") {
    const SinkCapture sink;
    Log::Hold();
    Log::Hold();
    Log::Warning("nested");
    Log::Release();
    CHECK(sink.Written().empty());
    Log::Release();
    CHECK(sink.Written() == "[liara-renderer] warning: nested\n");
}

TEST_CASE("Log - beyond the capacity, lines are dropped and counted") {
    const SinkCapture sink;
    Log::Hold();
    for (size_t i = 0; i < Log::HOLD_CAPACITY + 5; ++i) { Log::Info("line"); }
    Log::Release();
    const std::string written = sink.Written();
    CHECK(LineCount(written) == Log::HOLD_CAPACITY + 1);
    CHECK(written.ends_with("[liara-renderer] warning: 5 further messages were dropped while the console demo "
                            "covered the terminal\n"));
}

TEST_CASE("Log - warnings and errors are counted, held or not, and information is not") {
    const SinkCapture sink;
    const Log::Counts before = Log::GetCounts();
    Log::Info("not counted");
    Log::Warning("counted");
    Log::Hold();
    Log::Error("counted while held");
    const Log::Counts after = Log::GetCounts();
    Log::Release();
    CHECK(after.m_Warnings == before.m_Warnings + 1);
    CHECK(after.m_Errors == before.m_Errors + 1);
}

TEST_CASE("Log - releasing without a hold writes nothing") {
    const SinkCapture sink;
    Log::Release();
    CHECK(sink.Written().empty());
}

// NOLINTEND(readability-identifier-naming)
