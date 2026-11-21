// NOLINTBEGIN(readability-identifier-naming)
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "pipelines/Shaders.h"

#include <cstddef>
#include <cstdint>
#include <span>

#include <doctest/doctest.h>

// The built-in shaders, compiled at build time by vcpkg's glslc and embedded: Shaders.cpp is compiled into this
// executable on its own, with the generated SPIR-V it includes. It needs no GPU, so it runs in CI, and every CI leg,
// Windows included, proves the shaders compile.

namespace Shaders = Liara::Renderer::Shaders;

namespace
{
    constexpr uint32_t SPIRV_MAGIC = 0x07230203U;
    /** A SPIR-V module begins with five words: magic, version, generator, bound, schema. */
    constexpr size_t SPIRV_HEADER_WORDS = 5;

    /** A module longer than its header, starting with the magic number. */
    bool IsSpirv(const std::span<const uint32_t> code) {
        return code.size() > SPIRV_HEADER_WORDS && code.front() == SPIRV_MAGIC;
    }
}  // namespace

TEST_CASE("Shaders - the vertex shader is SPIR-V") { CHECK(IsSpirv(Shaders::MeshVertex())); }

TEST_CASE("Shaders - the fragment shader is SPIR-V") { CHECK(IsSpirv(Shaders::MeshFragment())); }

// NOLINTEND(readability-identifier-naming)
