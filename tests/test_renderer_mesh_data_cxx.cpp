// NOLINTBEGIN(readability-identifier-naming)
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "mesh/MeshData.h"

#include <liara/mesh.h>
#include <liara/result.h>

#include <array>
#include <cstddef>
#include <cstdint>

#include <doctest/doctest.h>

// The checks an upload makes on its data, tested without a GPU: MeshData.cpp is compiled into this executable on its
// own.

using Liara::Renderer::ValidateMeshData;

namespace
{
    constexpr std::array<float, 9> TRIANGLE_POSITIONS {0.0F, 0.5F, 0.0F, 0.5F, -0.5F, 0.0F, -0.5F, -0.5F, 0.0F};
    constexpr std::array<uint32_t, 3> TRIANGLE_COLORS {0xFFFF0000U, 0xFF00FF00U, 0xFF0000FFU};
    constexpr std::array<float, 12>
        QUAD_POSITIONS {-0.9F, 0.6F, 0.0F, -0.6F, 0.6F, 0.0F, -0.6F, 0.9F, 0.0F, -0.9F, 0.9F, 0.0F};
    constexpr std::array<uint32_t, 6> QUAD_INDICES {0, 1, 2, 2, 3, 0};

    constexpr liara_mesh_data_t Data(const float* positions,
                                     const uint32_t* colors,
                                     const size_t vertexCount,
                                     const uint32_t* indices = nullptr,
                                     const size_t indexCount = 0) {
        return {
            .struct_version = LIARA_MESH_DATA_VERSION,
            .reserved = 0,
            .positions = positions,
            .colors = colors,
            .vertex_count = vertexCount,
            .indices = indices,
            .index_count = indexCount,
        };
    }

    constexpr liara_mesh_data_t Triangle() {
        return Data(TRIANGLE_POSITIONS.data(), TRIANGLE_COLORS.data(), TRIANGLE_POSITIONS.size() / 3);
    }

    constexpr liara_mesh_data_t Quad() {
        return Data(QUAD_POSITIONS.data(),
                    nullptr,
                    QUAD_POSITIONS.size() / 3,
                    QUAD_INDICES.data(),
                    QUAD_INDICES.size());
    }
}  // namespace

TEST_CASE("ValidateMeshData - a triangle with colours, one without, and an indexed quad") {
    CHECK(ValidateMeshData(1, Triangle()) == LIARA_RESULT_SUCCESS);
    liara_mesh_data_t uncoloured = Triangle();
    uncoloured.colors = nullptr;
    CHECK(ValidateMeshData(1, uncoloured) == LIARA_RESULT_SUCCESS);
    CHECK(ValidateMeshData(1, Quad()) == LIARA_RESULT_SUCCESS);
}

TEST_CASE("ValidateMeshData - versions it does not understand") {
    for (const uint32_t version : {0U, 2U}) {
        liara_mesh_data_t data = Triangle();
        data.struct_version = version;
        CHECK(ValidateMeshData(1, data) == LIARA_RESULT_VERSION_MISMATCH);
    }
}

TEST_CASE("ValidateMeshData - the version is checked before the identifier") {
    liara_mesh_data_t data = Triangle();
    data.struct_version = 2;
    CHECK(ValidateMeshData(LIARA_MESH_ID_NONE, data) == LIARA_RESULT_VERSION_MISMATCH);
}

TEST_CASE("ValidateMeshData - the identifier NONE, checked before the positions") {
    CHECK(ValidateMeshData(LIARA_MESH_ID_NONE, Triangle()) == LIARA_RESULT_INVALID_ARGUMENT);
    liara_mesh_data_t data = Triangle();
    data.positions = nullptr;
    CHECK(ValidateMeshData(LIARA_MESH_ID_NONE, data) == LIARA_RESULT_INVALID_ARGUMENT);
}

TEST_CASE("ValidateMeshData - null positions, checked before the counts") {
    liara_mesh_data_t data = Triangle();
    data.positions = nullptr;
    CHECK(ValidateMeshData(1, data) == LIARA_RESULT_NULL_POINTER);
    data.vertex_count = 0;
    CHECK(ValidateMeshData(1, data) == LIARA_RESULT_NULL_POINTER);
}

TEST_CASE("ValidateMeshData - null indices with a count") {
    liara_mesh_data_t data = Quad();
    data.indices = nullptr;
    data.index_count = 3;
    CHECK(ValidateMeshData(1, data) == LIARA_RESULT_NULL_POINTER);
}

TEST_CASE("ValidateMeshData - counts that make no list of triangles") {
    liara_mesh_data_t noVertex = Triangle();
    noVertex.vertex_count = 0;
    CHECK(ValidateMeshData(1, noVertex) == LIARA_RESULT_INVALID_ARGUMENT);

    // Four vertices without indices: one triangle and a stray vertex.
    const liara_mesh_data_t fourVertices = Data(QUAD_POSITIONS.data(), nullptr, 4);
    CHECK(ValidateMeshData(1, fourVertices) == LIARA_RESULT_INVALID_ARGUMENT);

    liara_mesh_data_t fourIndices = Quad();
    fourIndices.index_count = 4;
    CHECK(ValidateMeshData(1, fourIndices) == LIARA_RESULT_INVALID_ARGUMENT);
}

TEST_CASE("ValidateMeshData - an index past the last vertex") {
    liara_mesh_data_t data = Quad();
    const std::array<uint32_t, 6> indices {0, 1, 2, 2, 3, 4};
    data.indices = indices.data();
    CHECK(ValidateMeshData(1, data) == LIARA_RESULT_OUT_OF_RANGE);
}

TEST_CASE("ValidateMeshData - a vertex count past what a draw can count") {
    if constexpr (sizeof(size_t) > sizeof(uint32_t)) {
        // A multiple of 3, so that only the range can refuse it; the positions are never read.
        liara_mesh_data_t data = Triangle();
        data.vertex_count = size_t {UINT32_MAX} + 3;
        CHECK(ValidateMeshData(1, data) == LIARA_RESULT_OUT_OF_RANGE);
    }
}

// NOLINTEND(readability-identifier-naming)
