#pragma once

/**
 * @file SinkCapture.h
 * @brief Routes the renderer's log sink into a temporary file for the life of the object, and reads it back.
 */

#include "Log.h"

#include <cstddef>
#include <cstdio>
#include <string>

class SinkCapture
{
public:
    SinkCapture()
        : m_File(std::tmpfile()) {
        Liara::Renderer::Log::SetStream(m_File);
    }

    ~SinkCapture() {
        Liara::Renderer::Log::SetStream(nullptr);
        // The FILE* from std::tmpfile is this object's to close; there is no gsl::owner in this code base.
        if (m_File != nullptr) { (void)std::fclose(m_File); }  // NOLINT(cppcoreguidelines-owning-memory)
    }

    SinkCapture(const SinkCapture&) = delete;
    SinkCapture& operator=(const SinkCapture&) = delete;
    SinkCapture(SinkCapture&&) = delete;
    SinkCapture& operator=(SinkCapture&&) = delete;

    /** @brief Everything the sink has written so far. */
    [[nodiscard]] std::string Written() const {
        std::string text;
        if (m_File == nullptr) { return text; }
        if (std::fflush(m_File) != 0 || std::fseek(m_File, 0, SEEK_SET) != 0) { return text; }
        char buffer[256];  // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
        size_t count = 0;
        while ((count = std::fread(buffer, 1, sizeof(buffer), m_File)) > 0) { text.append(buffer, count); }
        (void)std::fseek(m_File, 0, SEEK_END);
        return text;
    }

private:
    std::FILE* m_File;
};
