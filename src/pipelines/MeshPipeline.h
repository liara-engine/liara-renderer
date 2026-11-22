#pragma once

/**
 * @file MeshPipeline.h
 */

#include <vulkan/vulkan_raii.hpp>

namespace Liara::Renderer
{
    class Device;

    /**
     * @brief The one graphics pipeline every mesh is drawn with, for one colour format.
     *
     * Dynamic rendering into a single colour attachment, no depth, no blending, no culling (the winding convention
     * arrives with the camera). Positions are read as `R32G32B32_SFLOAT` from binding 0 and colours as `R32_UINT` from
     * binding 1, unpacked in the shader, which avoids depending on an 8-bit vertex format's optional support. Viewport
     * and scissor are dynamic, so a resize changes no pipeline. The pipeline layout is empty.
     */
    class MeshPipeline
    {
    public:
        /**
         * @param colorFormat The format of the images it draws into. Whether it is an `_SRGB` one decides the vertex
         *        shader's `DECODE_SRGB` specialization: the hardware encodes on write, so colours are decoded first.
         */
        MeshPipeline(const Device& device, vk::Format colorFormat);
        ~MeshPipeline() = default;

        MeshPipeline(const MeshPipeline&) = delete;
        MeshPipeline& operator=(const MeshPipeline&) = delete;
        MeshPipeline(MeshPipeline&&) = delete;
        MeshPipeline& operator=(MeshPipeline&&) = delete;

        [[nodiscard]] const vk::raii::Pipeline& Get() const noexcept;

        /** @brief The colour format it was built for. */
        [[nodiscard]] vk::Format GetColorFormat() const noexcept;

    private:
        vk::raii::PipelineLayout m_Layout;
        vk::raii::Pipeline m_Pipeline;
        vk::Format m_ColorFormat;
    };
}  // namespace Liara::Renderer
