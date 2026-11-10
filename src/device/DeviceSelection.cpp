#include "DeviceSelection.h"

#include <cstddef>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <utility>

namespace Liara::Renderer
{
    namespace
    {
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
        std::optional<std::string> RejectionReason(const DeviceCandidate& candidate) {
            if (candidate.m_ApiVersion < VK_API_VERSION_1_3) {
                return std::format("Vulkan {}.{} is below the required 1.3",
                                   VK_API_VERSION_MAJOR(candidate.m_ApiVersion),
                                   VK_API_VERSION_MINOR(candidate.m_ApiVersion));
            }
            if (!candidate.m_GraphicsFamily.has_value()) { return "no queue family supports graphics"; }
            return std::nullopt;
        }
    }  // namespace

    DeviceSelection SelectDevice(const std::span<const DeviceCandidate> candidates) {
        DeviceSelection selection;
        int chosenRank = 0;
        for (size_t index = 0; const DeviceCandidate& candidate : candidates) {
            if (auto reason = RejectionReason(candidate)) {
                selection.m_Rejections.push_back({.m_Index = index, .m_Reason = std::move(*reason)});
            }
            // Strictly better only, so a tie keeps the earlier index.
            else if (!selection.m_Chosen.has_value() || Rank(candidate.m_Type) < chosenRank) {
                selection.m_Chosen = index;
                chosenRank = Rank(candidate.m_Type);
            }
            ++index;
        }
        return selection;
    }
}  // namespace Liara::Renderer
