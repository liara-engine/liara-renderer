// NOLINTBEGIN(readability-identifier-naming)
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "TestCreateInfo.h"

#include <liara/renderer/renderer.h>
#include <liara/result.h>

#include <cstddef>

#include <doctest/doctest.h>

// Runs with the loader told to disable VK_LAYER_KHRONOS_validation (tests/CMakeLists.txt). A debug build of the
// renderer refuses to run without the layer, since a clean validation log has to mean the layer ran; a release build
// does not use it and creates normally, which needs a GPU, hence the `gpu` label.

TEST_CASE("liara_renderer_create - validation layer unavailable") {
    // Any address the call has no reason to write: it must come back unchanged.
    static std::max_align_t storage {};
    auto* const sentinel = reinterpret_cast<liara_renderer_handle_t*>(&storage);

    liara_renderer_handle_t* renderer = sentinel;
#ifdef LIARA_DEBUG
    CHECK(liara_renderer_create(&NO_WINDOW, &renderer) == LIARA_RESULT_NOT_SUPPORTED);
    CHECK(renderer == sentinel);
#else
    REQUIRE(liara_renderer_create(&NO_WINDOW, &renderer) == LIARA_RESULT_SUCCESS);
    CHECK(renderer != sentinel);
    liara_renderer_destroy(renderer);
#endif
}

// NOLINTEND(readability-identifier-naming)
