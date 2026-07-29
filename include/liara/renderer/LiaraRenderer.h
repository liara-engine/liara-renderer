#pragma once

#include <liara/renderer/packet.h>
#include <liara/result.h>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace Liara::Renderer
{
    class LiaraRenderer
    {
    public:
        LiaraRenderer() = default;

        ~LiaraRenderer() {
            if (m_Initialized) {
                std::cout << "\033[0m\033[?25h\n";
                std::cout.flush();
            }
        }

        LiaraRenderer(const LiaraRenderer&) = delete;
        LiaraRenderer& operator=(const LiaraRenderer&) = delete;
        LiaraRenderer(LiaraRenderer&&) = delete;
        LiaraRenderer& operator=(LiaraRenderer&&) = delete;

        /**
         * Submits a frame for rendering. The frame is represented by a grid of cells, each with a specified color.
         * Only the cells that have changed since the last frame will be updated in the console output.
         *
         * @param PACKET The render packet containing the frame data, including grid dimensions, background color,
         *               and an array of drawables to render.
         * @return A liara_result_t indicating success or failure of the operation.
         */
        [[nodiscard]] liara_result_t SubmitFrame(const liara_render_packet_t& PACKET) {
            if (PACKET.grid_width == 0 || PACKET.grid_height == 0) { return LIARA_RESULT_INVALID_ARGUMENT; }
            if (PACKET.drawable_count > 0 && PACKET.drawables == nullptr) { return LIARA_RESULT_NULL_POINTER; }

            EnsureGrid(PACKET.grid_width, PACKET.grid_height);

            std::vector<uint32_t> frame(static_cast<size_t>(m_Width) * m_Height, PACKET.background_color);
            for (size_t i = 0; i < PACKET.drawable_count; ++i) {
                const liara_render_drawable_t& DRAWABLE = PACKET.drawables[i];
                const auto GRID_X = static_cast<int32_t>(DRAWABLE.x);
                const auto GRID_Y = static_cast<int32_t>(DRAWABLE.y);
                if (GRID_X < 0 || GRID_Y < 0 || GRID_X >= static_cast<int32_t>(m_Width)
                    || GRID_Y >= static_cast<int32_t>(m_Height)) {
                    continue;
                }
                frame.at((static_cast<size_t>(GRID_Y) * m_Width) + static_cast<size_t>(GRID_X)) = DRAWABLE.color;
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
         * @param WIDTH The width of the grid.
         * @param HEIGHT The height of the grid.
         */
        void EnsureGrid(const uint16_t WIDTH, const uint16_t HEIGHT) {
            const bool IS_FIRST_FRAME = !m_Initialized;
            m_Initialized = true;

            if (!IS_FIRST_FRAME && WIDTH == m_Width && HEIGHT == m_Height) { return; }

            m_Width = WIDTH;
            m_Height = HEIGHT;
            m_PreviousFrame.assign(static_cast<size_t>(WIDTH) * HEIGHT, 0x00000000U);

            std::cout << "\033[2J";
            if (IS_FIRST_FRAME) { std::cout << "\033[?25l"; }
        }

        /**
         * Presents the specified frame to the console, updating only the cells that have changed
         * since the last frame. Each cell is represented by two block characters ("██") to create
         * a more visually appealing output.
         *
         * @param FRAME The frame to present, represented as a vector of 32-bit color values.
         */
        void Present(const std::vector<uint32_t>& FRAME) {
            for (uint16_t y = 0; y < m_Height; ++y) {
                for (uint16_t x = 0; x < m_Width; ++x) {
                    const size_t INDEX = (static_cast<size_t>(y) * m_Width) + x;
                    if (FRAME.at(INDEX) == m_PreviousFrame.at(INDEX)) { continue; }
                    std::cout << "\033[" << (y + 1) << ';' << ((x * 2) + 1) << 'H' << ColorToAnsi(FRAME.at(INDEX))
                              << "██\033[0m";
                }
            }
            std::cout.flush();
        }

        /**
         * Converts a 32-bit color value to an ANSI escape code string for setting the foreground color.
         * The color is expected to be in the format 0xRRGGBBAA, where RR, GG, BB are the red, green,
         * and blue components respectively. The alpha component is ignored.
         *
         * @param COLOR The 32-bit color value to convert.
         * @return A string containing the ANSI escape code for the specified color.
         */
        static std::string ColorToAnsi(const uint32_t COLOR) {
            return "\033[38;2;" + std::to_string((COLOR >> 16U) & 0xFFU) + ";" + std::to_string((COLOR >> 8U) & 0xFFU)
                   + ";" + std::to_string(COLOR & 0xFFU) + "m";
        }

        bool m_Initialized = false;
        uint16_t m_Width = 0;
        uint16_t m_Height = 0;
        std::vector<uint32_t> m_PreviousFrame;
    };
}  // namespace Liara::Renderer
