#include "presentation/animation.h"

namespace presentation {

namespace {
constexpr std::uint64_t kIdleFrameMs = 220;
constexpr std::uint64_t kRunFrameMs = 80;
constexpr std::uint64_t kLandMs = 100;
constexpr std::uint64_t kCoinFrameMs = 140;
// Vertical speed (pixels per step) below which the player is at the top of a jump.
constexpr float kApexSpeed = 1.0F;
}  // namespace

PlayerPose playerPose(const domain::Player& player, std::uint64_t time_ms,
                      std::uint64_t ms_since_landing) {
    if (!player.on_ground) {
        if (player.velocity_y < -kApexSpeed) {
            return PlayerPose::Rise;
        }
        return player.velocity_y > kApexSpeed ? PlayerPose::Fall : PlayerPose::Apex;
    }
    if (player.velocity_x == 0.0F) {
        if (ms_since_landing < kLandMs) {
            return PlayerPose::Land;
        }
        return static_cast<PlayerPose>(static_cast<int>(PlayerPose::Idle0) +
                                       static_cast<int>((time_ms / kIdleFrameMs) % 4));
    }
    return static_cast<PlayerPose>(static_cast<int>(PlayerPose::Run0) +
                                   static_cast<int>((time_ms / kRunFrameMs) % 6));
}

bool facesLeft(const domain::Player& player, bool was_facing_left) {
    if (player.velocity_x < 0.0F) {
        return true;
    }
    if (player.velocity_x > 0.0F) {
        return false;
    }
    return was_facing_left;
}

int coinFrame(std::uint64_t time_ms) {
    return static_cast<int>((time_ms / kCoinFrameMs) % 4);
}

}  // namespace presentation
