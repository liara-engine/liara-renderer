// NOLINTBEGIN(readability-identifier-naming)
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "Log.h"
#include "SinkCapture.h"

#include <doctest/doctest.h>

// The sink alone, compiled into this test from src/Log.cpp: no renderer, no GPU. Each case routes it into a temporary
// file and reads what it wrote.

namespace Log = Liara::Renderer::Log;

TEST_CASE("Log - a line is written at once") {
    const SinkCapture sink;
    Log::Info("direct");
    CHECK(sink.Written() == "[liara-renderer] info: direct\n");
}

// NOLINTEND(readability-identifier-naming)
