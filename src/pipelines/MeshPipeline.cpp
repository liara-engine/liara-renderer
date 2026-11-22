#include "MeshPipeline.h"

#include "Shaders.h"

#include "device/Device.h"
#include "swapchain/SwapchainChoices.h"

#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <cstdint>
#include <span>

namespace Liara::Renderer
{
    namespace
    {
        constexpr uint32_t POSITION_STRIDE = 3 * sizeof(float);
        constexpr uint32_t COLOR_STRIDE = sizeof(uint32_t);

        vk::raii::ShaderModule Module(const vk::raii::Device& device, const std::span<const uint32_t> code) {
            return device.createShaderModule(vk::ShaderModuleCreateInfo({}, code.size_bytes(), code.data()));
        }

        vk::raii::Pipeline Build(const vk::raii::Device& device,
                                 const vk::raii::PipelineLayout& layout,
                                 const vk::Format colorFormat) {
            const vk::raii::ShaderModule vertex = Module(device, Shaders::MeshVertex());
            const vk::raii::ShaderModule fragment = Module(device, Shaders::MeshFragment());

            // constant_id 0 of mesh.vert: decode the sRGB-encoded colours when the hardware encodes on write.
            const vk::Bool32 decodeSrgb = IsSrgbFormat(colorFormat) ? vk::True : vk::False;
            const vk::SpecializationMapEntry decodeEntry(0, 0, sizeof(decodeSrgb));
            const vk::SpecializationInfo specialization(1, &decodeEntry, sizeof(decodeSrgb), &decodeSrgb);

            const std::array stages {
                vk::PipelineShaderStageCreateInfo({},
                                                  vk::ShaderStageFlagBits::eVertex,
                                                  *vertex,
                                                  "main",
                                                  &specialization),
                vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, *fragment, "main"),
            };

            const std::array bindings {
                vk::VertexInputBindingDescription(0, POSITION_STRIDE, vk::VertexInputRate::eVertex),
                vk::VertexInputBindingDescription(1, COLOR_STRIDE, vk::VertexInputRate::eVertex),
            };
            const std::array attributes {
                vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, 0),
                vk::VertexInputAttributeDescription(1, 1, vk::Format::eR32Uint, 0),
            };
            const vk::PipelineVertexInputStateCreateInfo vertexInput({}, bindings, attributes);
            const vk::PipelineInputAssemblyStateCreateInfo inputAssembly({}, vk::PrimitiveTopology::eTriangleList);
            // Counts only: both are dynamic.
            const vk::PipelineViewportStateCreateInfo viewport({}, 1, nullptr, 1, nullptr);
            const vk::PipelineRasterizationStateCreateInfo rasterization({},
                                                                         vk::False,
                                                                         vk::False,
                                                                         vk::PolygonMode::eFill,
                                                                         vk::CullModeFlagBits::eNone,
                                                                         vk::FrontFace::eCounterClockwise,
                                                                         vk::False,
                                                                         0.0F,
                                                                         0.0F,
                                                                         0.0F,
                                                                         1.0F);
            const vk::PipelineMultisampleStateCreateInfo multisample({}, vk::SampleCountFlagBits::e1);
            const vk::PipelineColorBlendAttachmentState blendAttachment(
                vk::False,
                vk::BlendFactor::eOne,
                vk::BlendFactor::eZero,
                vk::BlendOp::eAdd,
                vk::BlendFactor::eOne,
                vk::BlendFactor::eZero,
                vk::BlendOp::eAdd,
                vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB
                    | vk::ColorComponentFlagBits::eA);
            const vk::PipelineColorBlendStateCreateInfo blend({}, vk::False, vk::LogicOp::eCopy, blendAttachment);
            const std::array dynamicStates {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
            const vk::PipelineDynamicStateCreateInfo dynamic({}, dynamicStates);

            const vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> createInfo {
                vk::GraphicsPipelineCreateInfo({},
                                               stages,
                                               &vertexInput,
                                               &inputAssembly,
                                               nullptr,
                                               &viewport,
                                               &rasterization,
                                               &multisample,
                                               nullptr,
                                               &blend,
                                               &dynamic,
                                               *layout),
                vk::PipelineRenderingCreateInfo(0, colorFormat),
            };
            return device.createGraphicsPipeline(nullptr, createInfo.get<vk::GraphicsPipelineCreateInfo>());
        }
    }  // namespace

    MeshPipeline::MeshPipeline(const Device& device, const vk::Format colorFormat)
        : m_Layout(device.Get().createPipelineLayout({}))
        , m_Pipeline(Build(device.Get(), m_Layout, colorFormat))
        , m_ColorFormat(colorFormat) {}

    const vk::raii::Pipeline& MeshPipeline::Get() const noexcept { return m_Pipeline; }

    vk::Format MeshPipeline::GetColorFormat() const noexcept { return m_ColorFormat; }
}  // namespace Liara::Renderer
