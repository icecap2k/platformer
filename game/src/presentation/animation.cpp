#include "presentation/animation.h"

namespace presentation {

namespace {
constexpr std::uint64_t kWalkFrameMs = 120;
constexpr std::uint64_t kCoinFrameMs = 140;
}  // namespace

PlayerPose playerPose(const domain::Player& player, std::uint64_t time_ms) {
    if (!player.on_ground) {
        return PlayerPose::Jump;
    }
    if (player.velocity_x == 0.0F) {
        return PlayerPose::Idle;
    }
    return ((time_ms / kWalkFrameMs) % 2 == 0) ? PlayerPose::Walk1 : PlayerPose::Walk2;
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
