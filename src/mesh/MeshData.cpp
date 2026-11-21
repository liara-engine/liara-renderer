#include "MeshData.h"

#include <liara/mesh.h>
#include <liara/result.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

namespace Liara::Renderer
{
    namespace
    {
        constexpr size_t TRIANGLE = 3;
        constexpr size_t LARGEST_DRAW = UINT32_MAX;
    }  // namespace

    liara_result_t ValidateMeshData(const liara_mesh_id_t mesh, const liara_mesh_data_t& data) noexcept {
        if (data.struct_version != LIARA_MESH_DATA_VERSION) { return LIARA_RESULT_VERSION_MISMATCH; }
        if (mesh == LIARA_MESH_ID_NONE) { return LIARA_RESULT_INVALID_ARGUMENT; }
        if (data.positions == nullptr) { return LIARA_RESULT_NULL_POINTER; }
        if (data.indices == nullptr && data.index_count > 0) { return LIARA_RESULT_NULL_POINTER; }

        if (data.vertex_count == 0) { return LIARA_RESULT_INVALID_ARGUMENT; }
        if (data.index_count == 0 && data.vertex_count % TRIANGLE != 0) { return LIARA_RESULT_INVALID_ARGUMENT; }
        if (data.index_count % TRIANGLE != 0) { return LIARA_RESULT_INVALID_ARGUMENT; }

        if (data.vertex_count > LARGEST_DRAW || data.index_count > LARGEST_DRAW) { return LIARA_RESULT_OUT_OF_RANGE; }
        if (data.index_count > 0) {
            const std::span indices(data.indices, data.index_count);
            const bool inRange =
                std::ranges::all_of(indices, [&data](const uint32_t index) { return index < data.vertex_count; });
            if (!inRange) { return LIARA_RESULT_OUT_OF_RANGE; }
        }
        return LIARA_RESULT_SUCCESS;
    }
}  // namespace Liara::Renderer
