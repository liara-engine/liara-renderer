// NOLINTBEGIN(readability-identifier-naming)
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "SinkCapture.h"

#include "device/Allocator.h"
#include "device/Buffer.h"
#include "device/Device.h"
#include "device/Instance.h"
#include "device/Loader.h"
#include "mesh/Meshes.h"
#include "passes/ViewPass.h"
#include "pipelines/MeshPipeline.h"
#include "swapchain/SwapchainChoices.h"

#include <liara/mesh.h>
#include <liara/native_window.h>
#include <liara/renderer/packet.h>
#include <liara/result.h>

#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <vk_mem_alloc.h>

// <X11/Xlib.h>, which Vulkan-Hpp includes for the Xlib surface (VK_USE_PLATFORM_XLIB_KHR, defined for every file that
// sees Vulkan-Hpp in this module), defines None as a macro, and doctest names an enumerator None. Nothing below uses
// Xlib's.
#ifdef None
    #undef None
#endif

#include <doctest/doctest.h>

// The view pass, proven by pixels: a device without a surface draws into an offscreen image, which is copied back and
// read. It needs a GPU, so `gpu`, and CTest fails it on any renderer warning or error, validation messages included.
// The cases that expect a warning capture the sink, and check the capture for errors themselves, since what is captured
// never reaches CTest.
//
// The test links liara_renderer_objects, the very code the module ships, rather than the library's ABI: the ABI has no
// offscreen target to draw into, and adding one for a test is what this avoids.

using Liara::Renderer::Allocator;
using Liara::Renderer::Buffer;
using Liara::Renderer::BufferAccess;
using Liara::Renderer::Device;
using Liara::Renderer::Instance;
using Liara::Renderer::IsSrgbFormat;
using Liara::Renderer::Loader;
using Liara::Renderer::Meshes;
using Liara::Renderer::MeshPipeline;
using Liara::Renderer::ViewPass;

namespace
{
    constexpr uint32_t SIZE = 64;
    constexpr uint32_t DARK_BLUE = 0xFF1A2440U;
    constexpr liara_render_viewport_t FULL {.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F};

    constexpr liara_mesh_id_t TRIANGLE = 1;
    constexpr liara_mesh_id_t QUAD = 2;
    constexpr liara_mesh_id_t WHITE_TRIANGLE = 3;

    constexpr std::array<float, 9> TRIANGLE_POSITIONS {0.0F, 0.5F, 0.0F, 0.5F, -0.5F, 0.0F, -0.5F, -0.5F, 0.0F};
    constexpr std::array<uint32_t, 3> TRIANGLE_COLORS {0xFFFF0000U, 0xFF00FF00U, 0xFF0000FFU};
    constexpr std::array<float, 12>
        QUAD_POSITIONS {-0.9F, 0.6F, 0.0F, -0.6F, 0.6F, 0.0F, -0.6F, 0.9F, 0.0F, -0.9F, 0.9F, 0.0F};
    constexpr std::array<uint32_t, 4> QUAD_COLORS {0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU};
    constexpr std::array<uint32_t, 6> QUAD_INDICES {0, 1, 2, 2, 3, 0};

    struct Rgb
    {
        int m_R = 0;
        int m_G = 0;
        int m_B = 0;
    };

    bool Near(const Rgb& actual, const Rgb& expected, const int tolerance) {
        return std::abs(actual.m_R - expected.m_R) <= tolerance && std::abs(actual.m_G - expected.m_G) <= tolerance
               && std::abs(actual.m_B - expected.m_B) <= tolerance;
    }

    constexpr liara_mesh_data_t Data(const std::span<const float> positions,
                                     const uint32_t* colors,
                                     const std::span<const uint32_t> indices = {}) {
        return {
            .struct_version = LIARA_MESH_DATA_VERSION,
            .reserved = 0,
            .positions = positions.data(),
            .colors = colors,
            .vertex_count = positions.size() / 3,
            .indices = indices.empty() ? nullptr : indices.data(),
            .index_count = indices.size(),
        };
    }

    constexpr liara_render_view_t View(const liara_render_viewport_t viewport,
                                       const std::span<const liara_render_drawable_t> drawables,
                                       const liara_render_target_id_t target = LIARA_RENDER_TARGET_WINDOW) {
        return {
            .target = target,
            .viewport = viewport,
            .clear_color = DARK_BLUE,
            .reserved = 0,
            .drawables = drawables.empty() ? nullptr : drawables.data(),
            .drawable_count = drawables.size(),
        };
    }

    /** What an offscreen image held after a frame: B8G8R8A8 bytes, row after row. */
    class Pixels
    {
    public:
        explicit Pixels(std::vector<std::byte> bytes)
            : m_Bytes(std::move(bytes)) {}

        [[nodiscard]] Rgb At(const uint32_t x, const uint32_t y) const {
            const size_t offset = ((static_cast<size_t>(y) * SIZE) + x) * 4;
            return {.m_R = std::to_integer<int>(m_Bytes.at(offset + 2)),
                    .m_G = std::to_integer<int>(m_Bytes.at(offset + 1)),
                    .m_B = std::to_integer<int>(m_Bytes.at(offset))};
        }

    private:
        std::vector<std::byte> m_Bytes;
    };

    /** A device without a surface, its meshes, and the triangle the launcher draws, uploaded as TRIANGLE. */
    struct Harness
    {
        Loader m_Loader;
        Instance m_Instance {m_Loader, LIARA_NATIVE_WINDOW_KIND_NONE};
        Device m_Device {m_Instance.Get(), nullptr, ""};
        Allocator m_Allocator {m_Loader, m_Instance, m_Device};
        Meshes m_Meshes {m_Device, m_Allocator};

        Harness() {
            // An exception rather than a REQUIRE, here and in Target: doctest reports either as a failed case, and
            // clang-tidy's analyzer reports a leak inside doctest for a REQUIRE in these helpers.
            const liara_mesh_data_t triangle = Data(TRIANGLE_POSITIONS, TRIANGLE_COLORS.data());
            if (m_Meshes.Upload(TRIANGLE, triangle) != LIARA_RESULT_SUCCESS) {
                throw std::runtime_error("the triangle could not be uploaded");
            }
        }
    };

    /** A SIZE x SIZE colour image, a pipeline and a view pass for its format, and a buffer to read it back. */
    class Target
    {
    public:
        Target(Harness& harness, const vk::Format format)
            : m_Harness(&harness)
            , m_Format(format)
            , m_Pipeline(harness.m_Device, format)
            , m_Pass(harness.m_Meshes)
            , m_Readback(harness.m_Allocator,
                         vk::DeviceSize {SIZE} * SIZE * 4,
                         vk::BufferUsageFlagBits::eTransferDst,
                         BufferAccess::HostRead) {
            const VkImageCreateInfo imageInfo =
                vk::ImageCreateInfo({},
                                    vk::ImageType::e2D,
                                    format,
                                    vk::Extent3D(SIZE, SIZE, 1),
                                    1,
                                    1,
                                    vk::SampleCountFlagBits::e1,
                                    vk::ImageTiling::eOptimal,
                                    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc);
            VmaAllocationCreateInfo allocationInfo {};
            allocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            VkImage image = VK_NULL_HANDLE;
            if (vmaCreateImage(harness.m_Allocator.Get(), &imageInfo, &allocationInfo, &image, &m_Allocation, nullptr)
                != VK_SUCCESS) {
                throw std::runtime_error("the offscreen image could not be created");
            }
            m_Image = image;
            m_View = harness.m_Device.Get().createImageView(
                vk::ImageViewCreateInfo({},
                                        m_Image,
                                        vk::ImageViewType::e2D,
                                        format,
                                        {},
                                        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));
        }

        ~Target() {
            m_View = nullptr;
            vmaDestroyImage(m_Harness->m_Allocator.Get(), static_cast<VkImage>(m_Image), m_Allocation);
        }

        Target(const Target&) = delete;
        Target& operator=(const Target&) = delete;
        Target(Target&&) = delete;
        Target& operator=(Target&&) = delete;

        /** Draw `views` into the image through this target's pass, as a frame would, and read the image back. */
        Pixels Render(const std::span<const liara_render_view_t> views) {
            const vk::raii::Device& device = m_Harness->m_Device.Get();
            vk::raii::CommandBuffers commands(device,
                                              vk::CommandBufferAllocateInfo(*m_Harness->m_Device.GetCommandPool(),
                                                                            vk::CommandBufferLevel::ePrimary,
                                                                            1));
            const vk::raii::CommandBuffer& command = commands.front();
            const vk::ImageSubresourceRange colour(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
            using Stage = vk::PipelineStageFlagBits2;
            using Access = vk::AccessFlagBits2;

            command.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
            const vk::ImageMemoryBarrier2 toAttachment(Stage::eNone,
                                                       Access::eNone,
                                                       Stage::eColorAttachmentOutput,
                                                       Access::eColorAttachmentWrite,
                                                       vk::ImageLayout::eUndefined,
                                                       vk::ImageLayout::eColorAttachmentOptimal,
                                                       VK_QUEUE_FAMILY_IGNORED,
                                                       VK_QUEUE_FAMILY_IGNORED,
                                                       m_Image,
                                                       colour);
            command.pipelineBarrier2(vk::DependencyInfo({}, {}, {}, toAttachment));

            m_Pass.Record(command, m_View, vk::Extent2D(SIZE, SIZE), IsSrgbFormat(m_Format), views, m_Pipeline);

            const vk::ImageMemoryBarrier2 toSource(Stage::eColorAttachmentOutput,
                                                   Access::eColorAttachmentWrite,
                                                   Stage::eCopy,
                                                   Access::eTransferRead,
                                                   vk::ImageLayout::eColorAttachmentOptimal,
                                                   vk::ImageLayout::eTransferSrcOptimal,
                                                   VK_QUEUE_FAMILY_IGNORED,
                                                   VK_QUEUE_FAMILY_IGNORED,
                                                   m_Image,
                                                   colour);
            command.pipelineBarrier2(vk::DependencyInfo({}, {}, {}, toSource));
            command.copyImageToBuffer(
                m_Image,
                vk::ImageLayout::eTransferSrcOptimal,
                m_Readback.Get(),
                vk::BufferImageCopy(0,
                                    0,
                                    0,
                                    vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
                                    {0, 0, 0},
                                    {SIZE, SIZE, 1}));
            const vk::BufferMemoryBarrier2 toHost(Stage::eCopy,
                                                  Access::eTransferWrite,
                                                  Stage::eHost,
                                                  Access::eHostRead,
                                                  VK_QUEUE_FAMILY_IGNORED,
                                                  VK_QUEUE_FAMILY_IGNORED,
                                                  m_Readback.Get(),
                                                  0,
                                                  vk::WholeSize);
            command.pipelineBarrier2(vk::DependencyInfo({}, {}, toHost, {}));
            command.end();

            const vk::raii::Fence done = device.createFence({});
            const vk::CommandBufferSubmitInfo submitted(*command);
            m_Harness->m_Device.GetQueue().submit2(vk::SubmitInfo2({}, {}, submitted, {}), *done);
            if (device.waitForFences(*done, vk::True, UINT64_MAX) != vk::Result::eSuccess) {
                throw std::runtime_error("the frame did not complete");
            }

            std::vector<std::byte> bytes(m_Readback.GetSize());
            m_Readback.Read(0, bytes);
            return Pixels(std::move(bytes));
        }

    private:
        Harness* m_Harness;
        vk::Format m_Format;
        MeshPipeline m_Pipeline;
        ViewPass m_Pass;
        Buffer m_Readback;
        vk::Image m_Image;
        VmaAllocation m_Allocation = nullptr;
        vk::raii::ImageView m_View = nullptr;
    };

    size_t Occurrences(const std::string& text, const std::string_view needle) {
        size_t count = 0;
        for (size_t at = text.find(needle); at != std::string::npos; at = text.find(needle, at + needle.size())) {
            ++count;
        }
        return count;
    }

    // Pixel centres. The triangle's red vertex is at the top (y up): NDC (0, 0.3) is row 22 when y is not inverted,
    // close to the green and blue base when it is.
    constexpr uint32_t CENTRE_X = 32;
    constexpr uint32_t CENTROID_Y = 37;  // NDC (0.016, -0.172): barycentric (0.328, 0.344, 0.328)
    constexpr uint32_t UPPER_Y = 22;
    constexpr uint32_t QUAD_PIXEL = 8;  // NDC (-0.73, 0.73), inside the quad and outside the triangle
    constexpr uint32_t RIGHT_HALF_X = 48;

    const std::array<liara_render_drawable_t, 1> DRAW_TRIANGLE {{{.mesh = TRIANGLE}}};
}  // namespace

TEST_CASE("ViewPass - the triangle, full frame, on an sRGB target") {
    Harness harness;
    Target target(harness, vk::Format::eB8G8R8A8Srgb);
    const std::array views {View(FULL, DRAW_TRIANGLE)};
    const Pixels image = target.Render(views);

    // Linear (0.328, 0.344, 0.328), encoded to sRGB on write.
    CHECK(Near(image.At(CENTRE_X, CENTROID_Y), {.m_R = 155, .m_G = 158, .m_B = 155}, 4));
    const Rgb upper = image.At(CENTRE_X, UPPER_Y);
    CHECK(upper.m_R > 2 * upper.m_G);
    CHECK(upper.m_R > 2 * upper.m_B);
    CHECK(Near(image.At(0, 0), {.m_R = 0x1A, .m_G = 0x24, .m_B = 0x40}, 1));
}

TEST_CASE("ViewPass - half a viewport") {
    Harness harness;
    Target target(harness, vk::Format::eB8G8R8A8Srgb);
    const std::array views {View({.x = 0.0F, .y = 0.0F, .width = 0.5F, .height = 1.0F}, DRAW_TRIANGLE)};
    const Pixels image = target.Render(views);

    CHECK(Near(image.At(RIGHT_HALF_X, CENTRE_X), {.m_R = 0, .m_G = 0, .m_B = 0}, 0));
    CHECK(Near(image.At(0, 0), {.m_R = 0x1A, .m_G = 0x24, .m_B = 0x40}, 1));
}

TEST_CASE("ViewPass - a UNORM target") {
    Harness harness;
    Target target(harness, vk::Format::eB8G8R8A8Unorm);
    const std::array views {View(FULL, DRAW_TRIANGLE)};
    const Pixels image = target.Render(views);

    CHECK(Near(image.At(0, 0), {.m_R = 0x1A, .m_G = 0x24, .m_B = 0x40}, 1));
    const Rgb upper = image.At(CENTRE_X, UPPER_Y);
    CHECK(upper.m_R > 2 * upper.m_G);
    CHECK(upper.m_R > 2 * upper.m_B);
}

TEST_CASE("ViewPass - an indexed mesh") {
    Harness harness;
    REQUIRE(harness.m_Meshes.Upload(QUAD, Data(QUAD_POSITIONS, QUAD_COLORS.data(), QUAD_INDICES))
            == LIARA_RESULT_SUCCESS);
    Target target(harness, vk::Format::eB8G8R8A8Srgb);
    const std::array<liara_render_drawable_t, 2> drawables {
        {{.mesh = TRIANGLE}, {.mesh = QUAD}}
    };
    const std::array views {View(FULL, drawables)};
    const Pixels image = target.Render(views);

    // White at the top left: the quad is drawn, from its indices, with y up.
    CHECK(Near(image.At(QUAD_PIXEL, QUAD_PIXEL), {.m_R = 255, .m_G = 255, .m_B = 255}, 1));
}

TEST_CASE("ViewPass - a mesh without colours draws white") {
    Harness harness;
    REQUIRE(harness.m_Meshes.Upload(WHITE_TRIANGLE, Data(TRIANGLE_POSITIONS, nullptr)) == LIARA_RESULT_SUCCESS);
    Target target(harness, vk::Format::eB8G8R8A8Srgb);
    const std::array<liara_render_drawable_t, 1> drawables {{{.mesh = WHITE_TRIANGLE}}};
    const std::array views {View(FULL, drawables)};
    const Pixels image = target.Render(views);

    CHECK(Near(image.At(CENTRE_X, CENTROID_Y), {.m_R = 255, .m_G = 255, .m_B = 255}, 1));
}

TEST_CASE("ViewPass - no view draws black") {
    Harness harness;
    Target target(harness, vk::Format::eB8G8R8A8Srgb);
    const Pixels image = target.Render({});

    CHECK(Near(image.At(CENTRE_X, CENTROID_Y), {.m_R = 0, .m_G = 0, .m_B = 0}, 0));
    CHECK(Near(image.At(0, 0), {.m_R = 0, .m_G = 0, .m_B = 0}, 0));
}

TEST_CASE("ViewPass - a zero-area view is skipped") {
    const SinkCapture sink;
    Harness harness;
    Target target(harness, vk::Format::eB8G8R8A8Srgb);
    const std::array views {View({.x = 0.0F, .y = 0.0F, .width = 0.0F, .height = 1.0F}, DRAW_TRIANGLE),
                            View(FULL, DRAW_TRIANGLE)};
    const Pixels image = target.Render(views);

    CHECK(Near(image.At(CENTRE_X, CENTROID_Y), {.m_R = 155, .m_G = 158, .m_B = 155}, 4));
    CHECK(Near(image.At(0, 0), {.m_R = 0x1A, .m_G = 0x24, .m_B = 0x40}, 1));
    CHECK(Occurrences(sink.Written(), "warning:") == 0);
    CHECK(Occurrences(sink.Written(), "error:") == 0);
}

TEST_CASE("ViewPass - an unknown mesh warns once") {
    const SinkCapture sink;
    Harness harness;
    Target target(harness, vk::Format::eB8G8R8A8Srgb);
    const std::array<liara_render_drawable_t, 1> drawables {{{.mesh = 99}}};
    const std::array views {View(FULL, drawables)};
    (void)target.Render(views);
    (void)target.Render(views);

    CHECK(Occurrences(sink.Written(), "mesh 99") == 1);
    CHECK(Occurrences(sink.Written(), "error:") == 0);
}

TEST_CASE("ViewPass - an unknown target warns once") {
    const SinkCapture sink;
    Harness harness;
    Target target(harness, vk::Format::eB8G8R8A8Srgb);
    const std::array views {View(FULL, DRAW_TRIANGLE, 42)};
    (void)target.Render(views);
    const Pixels image = target.Render(views);

    CHECK(Occurrences(sink.Written(), "render target 42") == 1);
    CHECK(Occurrences(sink.Written(), "error:") == 0);
    CHECK(Near(image.At(0, 0), {.m_R = 0, .m_G = 0, .m_B = 0}, 0));
}

TEST_CASE("ViewPass - a released mesh is skipped with one warning") {
    const SinkCapture sink;
    Harness harness;
    Target target(harness, vk::Format::eB8G8R8A8Srgb);
    const std::array views {View(FULL, DRAW_TRIANGLE)};
    (void)target.Render(views);
    REQUIRE(harness.m_Meshes.Release(TRIANGLE) == LIARA_RESULT_SUCCESS);
    (void)target.Render(views);
    const Pixels image = target.Render(views);

    CHECK(Occurrences(sink.Written(), "mesh 1,") == 1);
    CHECK(Occurrences(sink.Written(), "error:") == 0);
    CHECK(Near(image.At(CENTRE_X, CENTROID_Y), {.m_R = 0x1A, .m_G = 0x24, .m_B = 0x40}, 1));
}

// NOLINTEND(readability-identifier-naming)
