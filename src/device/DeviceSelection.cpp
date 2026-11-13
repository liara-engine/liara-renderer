#include "DeviceSelection.h"

#include <algorithm>
#include <cstddef>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace Liara::Renderer
{
    namespace
    {
        /** @brief How many values Rank returns, so that adding it moves a candidate after every ranked one. */
        constexpr int RANK_COUNT = 5;

        /** @brief Lower is preferred. */
        constexpr int Rank(const vk::PhysicalDeviceType type) {
            switch (type) {
                case vk::PhysicalDeviceType::eDiscreteGpu: return 0;
                case vk::PhysicalDeviceType::eIntegratedGpu: return 1;
                case vk::PhysicalDeviceType::eVirtualGpu: return 2;
                case vk::PhysicalDeviceType::eCpu: return 3;
                default: return 4;
            }
        }

        /** @brief The first requirement the candidate fails, or nothing when it is eligible. */
        std::optional<std::string> RejectionReason(const DeviceCandidate& candidate, const bool requirePresentation) {
            if (candidate.m_ApiVersion < VK_API_VERSION_1_3) {
                return std::format("Vulkan {}.{} is below the required 1.3",
                                   VK_API_VERSION_MAJOR(candidate.m_ApiVersion),
                                   VK_API_VERSION_MINOR(candidate.m_ApiVersion));
            }
            if (!candidate.m_GraphicsFamily.has_value()) { return "no queue family supports graphics"; }
            if (requirePresentation) {
                if (!candidate.m_SupportsSwapchain) { return "VK_KHR_swapchain is not supported"; }
                if (!candidate.m_GraphicsPresentFamily.has_value()) {
                    return "no queue family supports both graphics and presentation to the window";
                }
            }
            return std::nullopt;
        }

        constexpr char ToLowerAscii(const char character) noexcept {
            return (character >= 'A' && character <= 'Z') ? static_cast<char>(character - 'A' + 'a') : character;
        }

        /** @brief Whether `text` contains `part`, comparing ASCII letters without regard to case. */
        bool ContainsIgnoringAsciiCase(const std::string_view text, const std::string_view part) {
            return !std::ranges::search(text, part, {}, ToLowerAscii, ToLowerAscii).empty();
        }
    }  // namespace

    DeviceSelection SelectDevice(const std::span<const DeviceCandidate> candidates, const SelectionRequest& request) {
        const bool hasPreference = !request.m_PreferredName.empty();
        DeviceSelection selection;
        bool anyNameMatches = false;
        // Lower is preferred: a non-matching candidate ranks after every matching one.
        int chosenRank = 0;
        bool chosenMatches = false;
        for (size_t index = 0; const DeviceCandidate& candidate : candidates) {
            const bool matches = hasPreference && ContainsIgnoringAsciiCase(candidate.m_Name, request.m_PreferredName);
            anyNameMatches = anyNameMatches || matches;
            if (auto reason = RejectionReason(candidate, request.m_RequirePresentation)) {
                selection.m_Rejections.push_back({.m_Index = index, .m_Reason = std::move(*reason)});
            }
            else {
                const int rank = Rank(candidate.m_Type) + (matches || !hasPreference ? 0 : RANK_COUNT);
                // Strictly better only, so a tie keeps the earlier index.
                if (!selection.m_Chosen.has_value() || rank < chosenRank) {
                    selection.m_Chosen = index;
                    chosenRank = rank;
                    chosenMatches = matches;
                }
            }
            ++index;
        }

        if (!hasPreference) { selection.m_Preference = PreferenceOutcome::None; }
        else if (chosenMatches) { selection.m_Preference = PreferenceOutcome::Honoured; }
        else if (anyNameMatches) { selection.m_Preference = PreferenceOutcome::MatchedOnlyRejected; }
        else { selection.m_Preference = PreferenceOutcome::NoMatch; }
        return selection;
    }
}  // namespace Liara::Renderer
