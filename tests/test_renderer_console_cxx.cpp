// NOLINTBEGIN(readability-identifier-naming)
#define DOCTEST_CONFIG_IMPLEMENT
#include "console/ConsoleRenderer.h"

#include <liara/renderer/packet.h>
#include <liara/result.h>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>

#include <doctest/doctest.h>

// The console renderer is private and header-only, so this test includes it directly and needs no renderer instance,
// hence no GPU. It checks what the terminal receives with std::cout redirected into a string, which is why doctest
// reports on std::cerr instead (see main at the bottom of the file).

namespace
{
    class CoutCapture
    {
    public:
        CoutCapture()
            : m_Previous(std::cout.rdbuf(m_Buffer.rdbuf())) {}

        ~CoutCapture() { std::cout.rdbuf(m_Previous); }

        CoutCapture(const CoutCapture&) = delete;
        CoutCapture& operator=(const CoutCapture&) = delete;
        CoutCapture(CoutCapture&&) = delete;
        CoutCapture& operator=(CoutCapture&&) = delete;

        [[nodiscard]] std::string Text() const { return m_Buffer.str(); }

    private:
        std::ostringstream m_Buffer;
        std::streambuf* m_Previous;
    };

    constexpr liara_render_packet_t Packet(const uint16_t width,
                                           const uint16_t height,
                                           const liara_render_drawable_t* drawables,
                                           const size_t count) {
        return {
            .struct_version = LIARA_RENDER_PACKET_VERSION,
            .grid_width = width,
            .grid_height = height,
            .background_color = 0xFF000000U,
            .drawables = drawables,
            .drawable_count = count,
        };
    }

    constexpr liara_render_packet_t ValidPacket() { return Packet(4, 4, nullptr, 0); }

    constexpr liara_render_packet_t ZeroWidthPacket() { return Packet(0, 4, nullptr, 0); }
}  // namespace

TEST_CASE("ConsoleRenderer - destroyed without a frame, writes nothing") {
    const CoutCapture capture;
    { const Liara::Renderer::ConsoleRenderer console; }
    CHECK(capture.Text().empty());
}

TEST_CASE("ConsoleRenderer - first frame enters the alternate screen, destruction leaves it") {
    const CoutCapture capture;
    {
        Liara::Renderer::ConsoleRenderer console;
        CHECK(console.SubmitFrame(ValidPacket()) == LIARA_RESULT_SUCCESS);
    }
    CHECK(capture.Text().starts_with("\033[?1049h"));
    CHECK(capture.Text().ends_with("\033[?1049l"));
}

TEST_CASE("ConsoleRenderer - a refused frame writes nothing") {
    const CoutCapture capture;
    {
        Liara::Renderer::ConsoleRenderer console;
        CHECK(console.SubmitFrame(ZeroWidthPacket()) == LIARA_RESULT_INVALID_ARGUMENT);
    }
    CHECK(capture.Text().empty());
}

TEST_CASE("ConsoleRenderer::SubmitFrame - zero grid dimensions") {
    const CoutCapture capture;
    Liara::Renderer::ConsoleRenderer console;
    CHECK(console.SubmitFrame(Packet(0, 4, nullptr, 0)) == LIARA_RESULT_INVALID_ARGUMENT);
    CHECK(console.SubmitFrame(Packet(4, 0, nullptr, 0)) == LIARA_RESULT_INVALID_ARGUMENT);
}

TEST_CASE("ConsoleRenderer::SubmitFrame - null drawables with nonzero count") {
    const CoutCapture capture;
    Liara::Renderer::ConsoleRenderer console;
    CHECK(console.SubmitFrame(Packet(4, 4, nullptr, 1)) == LIARA_RESULT_NULL_POINTER);
}

TEST_CASE("ConsoleRenderer::SubmitFrame - empty drawable list") {
    const CoutCapture capture;
    Liara::Renderer::ConsoleRenderer console;
    CHECK(console.SubmitFrame(ValidPacket()) == LIARA_RESULT_SUCCESS);
}

TEST_CASE("ConsoleRenderer::SubmitFrame - valid single drawable, moved and submitted again") {
    const CoutCapture capture;
    Liara::Renderer::ConsoleRenderer console;
    liara_render_drawable_t drawable {.x = 1.0F, .y = 1.0F, .color = 0xFFFF0000U};
    CHECK(console.SubmitFrame(Packet(4, 4, &drawable, 1)) == LIARA_RESULT_SUCCESS);

    // Exercises the frame-diffing path: the old cell is cleared and the new one painted.
    drawable.x = 2.0F;
    drawable.y = 2.0F;
    CHECK(console.SubmitFrame(Packet(4, 4, &drawable, 1)) == LIARA_RESULT_SUCCESS);
}

TEST_CASE("ConsoleRenderer::SubmitFrame - out-of-bounds drawable is clipped, not a crash") {
    const CoutCapture capture;
    Liara::Renderer::ConsoleRenderer console;
    const liara_render_drawable_t drawable {.x = 100.0F, .y = -5.0F, .color = 0xFFFFFFFFU};
    CHECK(console.SubmitFrame(Packet(4, 4, &drawable, 1)) == LIARA_RESULT_SUCCESS);
}

int main(const int argc, const char* const* argv) {
    doctest::Context context(argc, argv);
    context.setCout(&std::cerr);
    return context.run();
}

// NOLINTEND(readability-identifier-naming)
