// NOLINTBEGIN(readability-identifier-naming)
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "device/DeviceSelection.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <doctest/doctest.h>

// The device selection policy, tested without a GPU: DeviceSelection.cpp is compiled into this executable on its own,
// and the candidates are made up.

using Liara::Renderer::DeviceCandidate;
using Liara::Renderer::PreferenceOutcome;
using Liara::Renderer::SelectDevice;
using Liara::Renderer::SelectionCriteria;

namespace
{
    DeviceCandidate Candidate(std::string name,
                              const vk::PhysicalDeviceType type,
                              const uint32_t apiVersion = VK_API_VERSION_1_3,
                              const std::optional<uint32_t> graphics = 0U,
                              const bool swapchain = true,
                              const std::optional<uint32_t> presentFamily = 0U) {
        return {
            .m_Name = std::move(name),
            .m_Type = type,
            .m_ApiVersion = apiVersion,
            .m_DriverName = "driver",
            .m_DriverInfo = "1.0",
            .m_GraphicsFamily = graphics,
            .m_SupportsSwapchain = swapchain,
            .m_GraphicsPresentFamily = presentFamily,
        };
    }

    constexpr SelectionCriteria Presenting(const std::string_view preferredName = {}) {
        return {.m_RequirePresentation = true, .m_PreferredName = preferredName};
    }

    constexpr uint32_t VULKAN_1_2 = VK_MAKE_API_VERSION(0, 1, 2, 0);

    bool Contains(const std::string& text, const char* part) { return text.find(part) != std::string::npos; }
}  // namespace

TEST_CASE("SelectDevice - an empty list selects nothing") {
    const auto selection = SelectDevice({});
    CHECK_FALSE(selection.m_Chosen.has_value());
    CHECK(selection.m_Rejections.empty());
}

TEST_CASE("SelectDevice - a 1.2 discrete GPU loses to a 1.3 integrated one") {
    const std::array candidates {Candidate("old", vk::PhysicalDeviceType::eDiscreteGpu, VULKAN_1_2),
                                 Candidate("igpu", vk::PhysicalDeviceType::eIntegratedGpu)};
    const auto selection = SelectDevice(candidates);
    CHECK(selection.m_Chosen == 1U);
    REQUIRE(selection.m_Rejections.size() == 1);
    CHECK(selection.m_Rejections.at(0).m_Index == 0);
    CHECK(Contains(selection.m_Rejections.at(0).m_Reason, "1.2"));
}

TEST_CASE("SelectDevice - a candidate with no graphics queue family is rejected") {
    const std::array candidates {
        Candidate("compute only", vk::PhysicalDeviceType::eDiscreteGpu, VK_API_VERSION_1_3, std::nullopt),
        Candidate("igpu", vk::PhysicalDeviceType::eIntegratedGpu)};
    const auto selection = SelectDevice(candidates);
    CHECK(selection.m_Chosen == 1U);
    REQUIRE(selection.m_Rejections.size() == 1);
    CHECK(selection.m_Rejections.at(0).m_Index == 0);
    CHECK(Contains(selection.m_Rejections.at(0).m_Reason, "graphics"));
}

TEST_CASE("SelectDevice - discrete beats integrated beats virtual beats CPU, whatever the order") {
    const std::array all {Candidate("cpu", vk::PhysicalDeviceType::eCpu),
                          Candidate("virtual", vk::PhysicalDeviceType::eVirtualGpu),
                          Candidate("igpu", vk::PhysicalDeviceType::eIntegratedGpu),
                          Candidate("dgpu", vk::PhysicalDeviceType::eDiscreteGpu)};
    CHECK(SelectDevice(all).m_Chosen == 3U);

    const std::array noDiscrete {Candidate("cpu", vk::PhysicalDeviceType::eCpu),
                                 Candidate("virtual", vk::PhysicalDeviceType::eVirtualGpu),
                                 Candidate("igpu", vk::PhysicalDeviceType::eIntegratedGpu)};
    CHECK(SelectDevice(noDiscrete).m_Chosen == 2U);

    const std::array cpuAndVirtual {Candidate("cpu", vk::PhysicalDeviceType::eCpu),
                                    Candidate("virtual", vk::PhysicalDeviceType::eVirtualGpu)};
    CHECK(SelectDevice(cpuAndVirtual).m_Chosen == 1U);

    const std::array otherAndCpu {Candidate("other", vk::PhysicalDeviceType::eOther),
                                  Candidate("cpu", vk::PhysicalDeviceType::eCpu)};
    CHECK(SelectDevice(otherAndCpu).m_Chosen == 1U);
}

TEST_CASE("SelectDevice - between two discrete GPUs, the first enumerated wins") {
    const std::array candidates {Candidate("first", vk::PhysicalDeviceType::eDiscreteGpu),
                                 Candidate("second", vk::PhysicalDeviceType::eDiscreteGpu)};
    const auto selection = SelectDevice(candidates);
    CHECK(selection.m_Chosen == 0U);
    CHECK(selection.m_Rejections.empty());
}

TEST_CASE("SelectDevice - a candidate failing both requirements gets one reason, the version") {
    const std::array candidates {Candidate("hopeless", vk::PhysicalDeviceType::eDiscreteGpu, VULKAN_1_2, std::nullopt)};
    const auto selection = SelectDevice(candidates);
    CHECK_FALSE(selection.m_Chosen.has_value());
    REQUIRE(selection.m_Rejections.size() == 1);
    CHECK(Contains(selection.m_Rejections.at(0).m_Reason, "1.2"));
    CHECK_FALSE(Contains(selection.m_Rejections.at(0).m_Reason, "graphics"));
}

TEST_CASE("SelectDevice - three ineligible candidates, three rejections in index order") {
    const std::array candidates {
        Candidate("a", vk::PhysicalDeviceType::eDiscreteGpu, VULKAN_1_2),
        Candidate("b", vk::PhysicalDeviceType::eIntegratedGpu, VK_API_VERSION_1_3, std::nullopt),
        Candidate("c", vk::PhysicalDeviceType::eCpu, VK_MAKE_API_VERSION(0, 1, 1, 0))};
    const auto selection = SelectDevice(candidates);
    CHECK_FALSE(selection.m_Chosen.has_value());
    REQUIRE(selection.m_Rejections.size() == 3);
    CHECK(selection.m_Rejections.at(0).m_Index == 0);
    CHECK(selection.m_Rejections.at(1).m_Index == 1);
    CHECK(selection.m_Rejections.at(2).m_Index == 2);
}

TEST_CASE("SelectDevice - without a preference, the outcome says so and the type order applies") {
    const std::array candidates {Candidate("igpu", vk::PhysicalDeviceType::eIntegratedGpu),
                                 Candidate("dgpu", vk::PhysicalDeviceType::eDiscreteGpu)};
    const auto selection = SelectDevice(candidates, Presenting());
    CHECK(selection.m_Chosen == 1U);
    CHECK(selection.m_Preference == PreferenceOutcome::NotGiven);
}

TEST_CASE("SelectDevice - a matching name beats a better-ranked type") {
    const std::array candidates {Candidate("AMD Radeon RX 6800", vk::PhysicalDeviceType::eDiscreteGpu),
                                 Candidate("Intel(R) UHD Graphics 770", vk::PhysicalDeviceType::eIntegratedGpu)};
    const auto selection = SelectDevice(candidates, Presenting("uhd"));
    CHECK(selection.m_Chosen == 1U);
    CHECK(selection.m_Preference == PreferenceOutcome::Honoured);
}

TEST_CASE("SelectDevice - a name matching nothing selects as without a preference") {
    const std::array candidates {Candidate("igpu", vk::PhysicalDeviceType::eIntegratedGpu),
                                 Candidate("dgpu", vk::PhysicalDeviceType::eDiscreteGpu)};
    const auto selection = SelectDevice(candidates, Presenting("no-such-gpu"));
    CHECK(selection.m_Chosen == 1U);
    CHECK(selection.m_Preference == PreferenceOutcome::NoMatch);
}

TEST_CASE("SelectDevice - a name matching only a rejected candidate selects as without a preference") {
    const std::array candidates {
        Candidate("Intel UHD", vk::PhysicalDeviceType::eIntegratedGpu, VK_API_VERSION_1_3, 0U, false),
        Candidate("AMD Radeon", vk::PhysicalDeviceType::eDiscreteGpu)};
    const auto selection = SelectDevice(candidates, Presenting("intel"));
    CHECK(selection.m_Chosen == 1U);
    CHECK(selection.m_Preference == PreferenceOutcome::MatchedOnlyRejected);
    REQUIRE(selection.m_Rejections.size() == 1);
    CHECK(selection.m_Rejections.at(0).m_Index == 0);
    CHECK(Contains(selection.m_Rejections.at(0).m_Reason, "VK_KHR_swapchain"));
}

TEST_CASE("SelectDevice - among matching candidates, the type order applies") {
    const std::array candidates {Candidate("Vendor GPU integrated", vk::PhysicalDeviceType::eIntegratedGpu),
                                 Candidate("Vendor GPU discrete", vk::PhysicalDeviceType::eDiscreteGpu),
                                 Candidate("Other", vk::PhysicalDeviceType::eDiscreteGpu)};
    const auto selection = SelectDevice(candidates, Presenting("VENDOR gpu"));
    CHECK(selection.m_Chosen == 1U);
    CHECK(selection.m_Preference == PreferenceOutcome::Honoured);
}

TEST_CASE("SelectDevice - presenting needs a family that does graphics and presentation") {
    const std::array candidates {
        Candidate("cannot present", vk::PhysicalDeviceType::eDiscreteGpu, VK_API_VERSION_1_3, 0U, true, std::nullopt),
        Candidate("can present", vk::PhysicalDeviceType::eIntegratedGpu)};
    const auto selection = SelectDevice(candidates, Presenting());
    CHECK(selection.m_Chosen == 1U);
    REQUIRE(selection.m_Rejections.size() == 1);
    CHECK(selection.m_Rejections.at(0).m_Index == 0);
    CHECK(Contains(selection.m_Rejections.at(0).m_Reason, "presentation"));
}

TEST_CASE("SelectDevice - without presentation, the presentation fields are ignored") {
    const std::array candidates {Candidate("no swapchain, no present family",
                                           vk::PhysicalDeviceType::eDiscreteGpu,
                                           VK_API_VERSION_1_3,
                                           0U,
                                           false,
                                           std::nullopt)};
    const auto selection = SelectDevice(candidates, SelectionCriteria {});
    CHECK(selection.m_Chosen == 0U);
    CHECK(selection.m_Rejections.empty());
}

// NOLINTEND(readability-identifier-naming)
