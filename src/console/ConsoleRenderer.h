#pragma once

#include "../Log.h"

#include <liara/renderer/packet.h>
#include <liara/result.h>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace Liara::Renderer
{
    class ConsoleRenderer
    {
    public:
        /**
         * @param holdLogs Hold the renderer's log lines while the grid covers the terminal, from the first frame until
         *        destruction, so that a line written meanwhile is not overwritten by the next frame.
         */
        explicit ConsoleRenderer(const bool holdLogs = false)
            : m_HoldLogs(holdLogs) {}

        /**
         * Leaves the terminal's alternate screen if a frame was ever drawn, so that whatever was written before the
         * first frame (the host's output, the renderer's creation logs) is on screen again, then writes the log lines
         * held meanwhile. A renderer that never drew writes nothing.
         */
        ~ConsoleRenderer() {
            if (m_Initialized) {
                std::cout << "\033[0m\033[?25h\033[?1049l";
                std::cout.flush();
            }
            if (m_Holding) { Log::Release(); }
        }

        ConsoleRenderer(const ConsoleRenderer&) = delete;
        ConsoleRenderer& operator=(const ConsoleRenderer&) = delete;
        ConsoleRenderer(ConsoleRenderer&&) = delete;
        ConsoleRenderer& operator=(ConsoleRenderer&&) = delete;

        /**
         * Submits a frame for rendering. The frame is represented by a grid of cells, each with a specified color.
         * Only the cells that have changed since the last frame will be updated in the console output.
         *
         * @param packet The render packet containing the frame data, including grid dimensions, background color,
         *               and an array of drawables to render.
         * @return A liara_result_t indicating success or failure of the operation.
         */
        [[nodiscard]] liara_result_t SubmitFrame(const liara_render_packet_t& packet) {
            if (packet.grid_width == 0 || packet.grid_height == 0) { return LIARA_RESULT_INVALID_ARGUMENT; }
            if (packet.drawable_count > 0 && packet.drawables == nullptr) { return LIARA_RESULT_NULL_POINTER; }

            EnsureGrid(packet.grid_width, packet.grid_height);

            std::vector<uint32_t> frame(static_cast<size_t>(m_Width) * m_Height, packet.background_color);
            for (size_t i = 0; i < packet.drawable_count; ++i) {
                const auto& [x, y, color] = packet.drawables[i];
                const auto gridX = static_cast<int32_t>(x);
                const auto gridY = static_cast<int32_t>(y);
                // NOLINTBEGIN(modernize-use-integer-sign-comparison)
                // False positive
                if (gridX < 0 || gridY < 0 || gridX >= static_cast<int32_t>(m_Width)
                    || gridY >= static_cast<int32_t>(m_Height)) {
                    // NOLINTEND(modernize-use-integer-sign-comparison)
                    continue;
                }
                frame.at((static_cast<size_t>(gridY) * m_Width) + static_cast<size_t>(gridX)) = color;
            }

            Present(frame);
            m_PreviousFrame = std::move(frame);
            return LIARA_RESULT_SUCCESS;
        }

    private:
        /**
         * Ensures that the internal grid is initialized and matches the specified dimensions.
         * If the grid is uninitialized or the dimensions have changed, it will reinitialize the grid
         * and clear the previous frame to force a repaint of all cells.
         *
         * @param width The width of the grid.
         * @param height The height of the grid.
         */
        void EnsureGrid(const uint16_t width, const uint16_t height) {
            const bool isFirstFrame = !m_Initialized;
            m_Initialized = true;

            if (!isFirstFrame && width == m_Width && height == m_Height) { return; }

            m_Width = width;
            m_Height = height;
            m_PreviousFrame.assign(static_cast<size_t>(width) * height, 0x00000000U);

            // The first frame switches to the alternate screen (which starts blank) instead of clearing the main one,
            // the way less and vim do, so the main screen's content survives the demo. A later size change clears.
            if (isFirstFrame) {
                std::cout << "\033[?1049h\033[?25l";
                if (m_HoldLogs) {
                    Log::Hold();
                    m_Holding = true;
                }
            }
            else { std::cout << "\033[2J"; }
            m_StatusDirty = true;
        }

        /**
         * Presents the specified frame to the console, updating only the cells that have changed
         * since the last frame. Each cell is represented by two block characters ("██") to create
         * a more visually appealing output.
         *
         * @param frame The frame to present, represented as a vector of 32-bit color values.
         */
        void Present(const std::vector<uint32_t>& frame) {
            for (uint16_t y = 0; y < m_Height; ++y) {
                for (uint16_t x = 0; x < m_Width; ++x) {
                    const size_t index = (static_cast<size_t>(y) * m_Width) + x;
                    if (frame.at(index) == m_PreviousFrame.at(index)) { continue; }
                    std::cout << "\033[" << (y + 1) << ';' << ((x * 2) + 1) << 'H' << ColorToAnsi(frame.at(index))
                              << "██\033[0m";
                }
            }
            WriteStatus();
            std::cout.flush();
        }

        /**
         * Writes, on the line under the grid, how many warnings and errors the renderer has reported, so that a message
         * held while the grid covers the terminal is visible as a count at once. Only when the counts changed, or the
         * grid was (re)drawn from scratch.
         */
        void WriteStatus() {
            const Log::Counts counts = Log::GetCounts();
            if (!m_StatusDirty && counts == m_StatusCounts) { return; }
            m_StatusDirty = false;
            m_StatusCounts = counts;
            std::cout << "\033[" << (m_Height + 1) << ";1H\033[0mrenderer: " << counts.m_Warnings << " warnings, "
                      << counts.m_Errors << " errors" << (m_Holding ? " (shown on exit)" : "") << "\033[K";
        }

        /**
         * Converts a 32-bit color value to an ANSI escape code string for setting the foreground color.
         * The color is expected to be in the format 0xRRGGBBAA, where RR, GG, BB are the red, green,
         * and blue components respectively. The alpha component is ignored.
         *
         * @param color The 32-bit color value to convert.
         * @return A string containing the ANSI escape code for the specified color.
         */
        static std::string ColorToAnsi(const uint32_t color) {
            return "\033[38;2;" + std::to_string((color >> 16U) & 0xFFU) + ";" + std::to_string((color >> 8U) & 0xFFU)
                   + ";" + std::to_string(color & 0xFFU) + "m";
        }

        bool m_HoldLogs = false;
        bool m_Holding = false;
        bool m_StatusDirty = true;
        Log::Counts m_StatusCounts;
        bool m_Initialized = false;
        uint16_t m_Width = 0;
        uint16_t m_Height = 0;
        std::vector<uint32_t> m_PreviousFrame;
    };
}  // namespace Liara::Renderer
