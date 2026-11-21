#include "Meshes.h"

#include "device/Buffer.h"
#include "device/Device.h"

#include <liara/mesh.h>
#include <liara/result.h>

#include <vulkan/vulkan_raii.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace Liara::Renderer
{
    namespace
    {
        constexpr vk::DeviceSize REGION_ALIGNMENT = 16;
        constexpr vk::DeviceSize POSITION_SIZE = 3 * sizeof(float);
        constexpr uint32_t WHITE = 0xFFFFFFFFU;

        constexpr vk::DeviceSize AlignUp(const vk::DeviceSize value) {
            return (value + REGION_ALIGNMENT - 1) / REGION_ALIGNMENT * REGION_ALIGNMENT;
        }

        /** Where each stream goes in the mesh's buffer, and the buffer's size. */
        struct Layout
        {
            vk::DeviceSize m_Positions = 0;
            vk::DeviceSize m_Colors = 0;
            vk::DeviceSize m_Indices = 0;
            vk::DeviceSize m_Size = 0;
        };

        Layout LayOut(const liara_mesh_data_t& data) {
            Layout layout;
            layout.m_Colors = AlignUp(data.vertex_count * POSITION_SIZE);
            layout.m_Indices = AlignUp(layout.m_Colors + (data.vertex_count * sizeof(uint32_t)));
            layout.m_Size = layout.m_Indices + (data.index_count * sizeof(uint32_t));
            return layout;
        }

        /** Copy the staging buffer into the mesh's, and make the copy visible to vertex input, then wait for it. */
        void CopyAndWait(const Device& device, const Buffer& staging, const Buffer& destination) {
            const vk::raii::Device& logical = device.Get();
            vk::raii::CommandBuffers commands(
                logical,
                vk::CommandBufferAllocateInfo(*device.GetCommandPool(), vk::CommandBufferLevel::ePrimary, 1));
            const vk::raii::CommandBuffer& command = commands.front();

            command.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
            command.copyBuffer(staging.Get(), destination.Get(), vk::BufferCopy(0, 0, destination.GetSize()));
            // Waiting on the fence below gives the host an execution dependency only: without this barrier, a later
            // submission's vertex fetch would have no memory dependency on the copy.
            const vk::BufferMemoryBarrier2 visible(
                vk::PipelineStageFlagBits2::eCopy,
                vk::AccessFlagBits2::eTransferWrite,
                vk::PipelineStageFlagBits2::eVertexAttributeInput | vk::PipelineStageFlagBits2::eIndexInput,
                vk::AccessFlagBits2::eVertexAttributeRead | vk::AccessFlagBits2::eIndexRead,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                destination.Get(),
                0,
                vk::WholeSize);
            command.pipelineBarrier2(vk::DependencyInfo({}, {}, visible, {}));
            command.end();

            const vk::raii::Fence done = logical.createFence({});
            const vk::CommandBufferSubmitInfo submitted(*command);
            device.GetQueue().submit2(vk::SubmitInfo2({}, {}, submitted, {}), *done);
            (void)logical.waitForFences(*done, vk::True, UINT64_MAX);
        }
    }  // namespace

    Meshes::Meshes(const Device& device, const Allocator& allocator)
        : m_Device(&device)
        , m_Allocator(&allocator) {}

    liara_result_t Meshes::Upload(const liara_mesh_id_t mesh, const liara_mesh_data_t& data) {
        if (m_Meshes.contains(mesh)) { return LIARA_RESULT_RESOURCE_ALREADY_EXISTS; }

        const Layout layout = LayOut(data);
        Buffer staging(*m_Allocator, layout.m_Size, vk::BufferUsageFlagBits::eTransferSrc, BufferAccess::HostWrite);
        staging.Write(layout.m_Positions, std::as_bytes(std::span(data.positions, data.vertex_count * 3)));
        if (data.colors != nullptr) {
            staging.Write(layout.m_Colors, std::as_bytes(std::span(data.colors, data.vertex_count)));
        }
        else {
            const std::vector<uint32_t> white(data.vertex_count, WHITE);
            staging.Write(layout.m_Colors, std::as_bytes(std::span(white)));
        }
        if (data.index_count > 0) {
            staging.Write(layout.m_Indices, std::as_bytes(std::span(data.indices, data.index_count)));
        }

        Buffer buffer(*m_Allocator,
                      layout.m_Size,
                      vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer
                          | vk::BufferUsageFlagBits::eTransferDst,
                      BufferAccess::DeviceOnly);
        CopyAndWait(*m_Device, staging, buffer);

        // ValidateMeshData guaranteed both counts fit in 32 bits.
        m_Meshes.emplace(mesh,
                         GpuMesh {
                             .m_Buffer = std::move(buffer),
                             .m_PositionsOffset = layout.m_Positions,
                             .m_ColorsOffset = layout.m_Colors,
                             .m_IndicesOffset = layout.m_Indices,
                             .m_VertexCount = static_cast<uint32_t>(data.vertex_count),
                             .m_IndexCount = static_cast<uint32_t>(data.index_count),
                         });
        return LIARA_RESULT_SUCCESS;
    }

    liara_result_t Meshes::Release(const liara_mesh_id_t mesh) {
        const auto found = m_Meshes.find(mesh);
        if (found == m_Meshes.end()) { return LIARA_RESULT_RESOURCE_NOT_FOUND; }
        // A frame in flight may still read it.
        m_Device->Get().waitIdle();
        m_Meshes.erase(found);
        return LIARA_RESULT_SUCCESS;
    }

    const GpuMesh* Meshes::Find(const liara_mesh_id_t mesh) const noexcept {
        const auto found = m_Meshes.find(mesh);
        return found == m_Meshes.end() ? nullptr : &found->second;
    }

    std::size_t Meshes::Count() const noexcept { return m_Meshes.size(); }
}  // namespace Liara::Renderer
