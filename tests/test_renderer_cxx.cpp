// NOLINTBEGIN(readability-identifier-naming)
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <liara/renderer/renderer.h>
#include <liara/result.h>

#include <cstdint>

#include <doctest/doctest.h>

TEST_CASE("liara_renderer_abi_version") {
    const uint32_t version = liara_renderer_abi_version();
    CHECK(version > 0);
}

TEST_CASE("liara_renderer_create - success") {
    liara_renderer_t* renderer = nullptr;
    const liara_result result = liara_renderer_create(&renderer);

    CHECK(result == LIARA_RESULT_SUCCESS);
    CHECK(renderer != nullptr);

    liara_renderer_destroy(renderer);
}

TEST_CASE("liara_renderer_create - null pointer") {
    const liara_result result = liara_renderer_create(nullptr);
    CHECK(result == LIARA_RESULT_NULL_POINTER);
}

TEST_CASE("liara_renderer_destroy - success") {
    liara_renderer_t* renderer = nullptr;
    liara_renderer_create(&renderer);

    const liara_result result = liara_renderer_destroy(renderer);
    CHECK(result == LIARA_RESULT_SUCCESS);
}

TEST_CASE("liara_renderer_destroy - null pointer") {
    const liara_result result = liara_renderer_destroy(nullptr);
    CHECK(result == LIARA_RESULT_NULL_POINTER);
}

TEST_CASE("liara_renderer_destroy - invalid state") {
    liara_renderer_t* renderer = nullptr;
    liara_renderer_create(&renderer);
    liara_renderer_destroy(renderer);

    const liara_result result = liara_renderer_destroy(renderer);
    CHECK(result == LIARA_RESULT_INVALID_STATE);
}

TEST_CASE("liara_renderer_print - success") {
    liara_renderer_t* renderer = nullptr;
    liara_renderer_create(&renderer);

    const char* message = "Hello, World!";
    const liara_result result = liara_renderer_print(renderer, message, 13);

    CHECK(result == LIARA_RESULT_SUCCESS);

    liara_renderer_destroy(renderer);
}

TEST_CASE("liara_renderer_print - null renderer") {
    const auto* const message = "Hello";
    const liara_result result = liara_renderer_print(nullptr, message, 5);
    CHECK(result == LIARA_RESULT_NULL_POINTER);
}

TEST_CASE("liara_renderer_print - null message") {
    liara_renderer_t* renderer = nullptr;
    liara_renderer_create(&renderer);

    const liara_result result = liara_renderer_print(renderer, nullptr, 5);
    CHECK(result == LIARA_RESULT_NULL_POINTER);

    liara_renderer_destroy(renderer);
}

TEST_CASE("liara_renderer_print - zero length") {
    liara_renderer_t* renderer = nullptr;
    liara_renderer_create(&renderer);

    const auto* const message = "test";
    const liara_result result = liara_renderer_print(renderer, message, 0);
    CHECK(result == LIARA_RESULT_INVALID_ARGUMENT);

    liara_renderer_destroy(renderer);
}

TEST_CASE("liara_renderer_print - invalid state") {
    liara_renderer_t* renderer = nullptr;
    liara_renderer_create(&renderer);
    liara_renderer_destroy(renderer);

    const auto* const message = "test";
    const liara_result result = liara_renderer_print(renderer, message, 4);
    CHECK(result == LIARA_RESULT_INVALID_STATE);
}

TEST_CASE("liara_renderer_set_text_color - basic") {
    liara_renderer_t* renderer = nullptr;
    liara_renderer_create(&renderer);

    liara_renderer_set_text_color(renderer, 0xFF0000FF);

    liara_renderer_destroy(renderer);
}

TEST_CASE("liara_renderer_set_text_color - null renderer") { liara_renderer_set_text_color(nullptr, 0xFF0000FF); }

TEST_CASE("liara_renderer_set_text_color - invalid state") {
    liara_renderer_t* renderer = nullptr;
    liara_renderer_create(&renderer);
    liara_renderer_destroy(renderer);

    liara_renderer_set_text_color(renderer, 0xFF0000FF);
}

TEST_CASE("liara_renderer_set_text_color - various colors") {
    liara_renderer_t* renderer = nullptr;
    liara_renderer_create(&renderer);

    SUBCASE("Red color") { liara_renderer_set_text_color(renderer, 0xFF0000FF); }
    SUBCASE("Green color") { liara_renderer_set_text_color(renderer, 0x00FF00FF); }
    SUBCASE("Blue color") { liara_renderer_set_text_color(renderer, 0x0000FFFF); }
    SUBCASE("White color") { liara_renderer_set_text_color(renderer, 0xFFFFFFFF); }

    liara_renderer_destroy(renderer);
}

TEST_CASE("liara_renderer_set_background_color - basic") {
    liara_renderer_t* renderer = nullptr;
    liara_renderer_create(&renderer);

    liara_renderer_set_background_color(renderer, 0xFF000000);

    liara_renderer_destroy(renderer);
}

TEST_CASE("liara_renderer_set_background_color - null renderer") {
    liara_renderer_set_background_color(nullptr, 0xFF000000);
}

TEST_CASE("liara_renderer_set_background_color - invalid state") {
    liara_renderer_t* renderer = nullptr;
    liara_renderer_create(&renderer);
    liara_renderer_destroy(renderer);

    liara_renderer_set_background_color(renderer, 0xFF000000);
}

TEST_CASE("liara_renderer_set_background_color - various colors") {
    liara_renderer_t* renderer = nullptr;
    liara_renderer_create(&renderer);

    SUBCASE("Black background") { liara_renderer_set_background_color(renderer, 0xFF000000); }
    SUBCASE("White background") { liara_renderer_set_background_color(renderer, 0xFFFFFFFF); }
    SUBCASE("Gray background") { liara_renderer_set_background_color(renderer, 0xFF808080); }

    liara_renderer_destroy(renderer);
}

TEST_CASE("liara_renderer - integration test") {
    liara_renderer_t* renderer = nullptr;
    liara_renderer_create(&renderer);

    liara_renderer_set_text_color(renderer, 0xFF0000FF);
    liara_renderer_set_background_color(renderer, 0xFFFFFFFF);

    const liara_result result = liara_renderer_print(renderer, "Test message", 12);
    CHECK(result == LIARA_RESULT_SUCCESS);

    liara_renderer_destroy(renderer);
}

TEST_CASE("liara_renderer - multiple colors integration") {
    liara_renderer_t* renderer = nullptr;
    liara_renderer_create(&renderer);

    liara_renderer_set_text_color(renderer, 0xFFFF0000);
    CHECK(liara_renderer_print(renderer, "Red text", 8) == LIARA_RESULT_SUCCESS);

    liara_renderer_set_text_color(renderer, 0xFF00FF00);
    CHECK(liara_renderer_print(renderer, "Green text", 10) == LIARA_RESULT_SUCCESS);

    liara_renderer_set_background_color(renderer, 0xFF0000FF);
    CHECK(liara_renderer_print(renderer, "Blue background", 15) == LIARA_RESULT_SUCCESS);

    liara_renderer_destroy(renderer);
}

// NOLINTEND(readability-identifier-naming)
