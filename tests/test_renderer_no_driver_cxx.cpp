// NOLINTBEGIN(readability-identifier-naming)
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <liara/renderer/renderer.h>
#include <liara/result.h>

#include <cstddef>

#include <doctest/doctest.h>

// Runs with VK_DRIVER_FILES and VK_ICD_FILENAMES pointing at a file that does not exist (tests/CMakeLists.txt), so a
// Vulkan loader, where there is one, finds no driver. On a machine with no loader at all, such as a CI runner, creation
// fails one step earlier for that reason instead, and the expected result is the same. Either way no GPU is involved,
// and the test runs in CI.

TEST_CASE("liara_renderer_create - no usable driver") {
    // Any address the call has no reason to write: it must come back unchanged.
    static std::max_align_t storage {};
    auto* const sentinel = reinterpret_cast<liara_renderer_handle_t*>(&storage);

    liara_renderer_handle_t* renderer = sentinel;
    CHECK(liara_renderer_create(&renderer) == LIARA_RESULT_NOT_SUPPORTED);
    CHECK(renderer == sentinel);
}

// NOLINTEND(readability-identifier-naming)
