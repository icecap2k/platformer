#include "presentation/animation.h"

#include "domain/enemies/enemy_patrols.h"

namespace presentation {

namespace {
constexpr std::uint64_t kIdleFrameMs = 220;
constexpr std::uint64_t kRunFrameMs = 80;
constexpr std::uint64_t kLandMs = 100;
constexpr std::uint64_t kCoinFrameMs = 140;
constexpr std::uint64_t kFruitFrameMs = 160;  // a quick breath: a full pulse every 0.64 s
constexpr std::uint64_t kEnemyFrameMs = 180;
constexpr std::uint64_t kFruitBobMs = 300;    // the bob moves one pixel every 0.3 s
// Vertical speed (pixels per step) below which the player is at the top of a jump.
constexpr float kApexSpeed = 1.0F;
}  // namespace

PlayerPose playerPose(const domain::Player& player, std::uint64_t time_ms,
                      std::uint64_t ms_since_landing) {
    if (player.hurt_steps > 0) {
        return PlayerPose::Hurt;
    }
    if (player.on_ground && player.slide_steps > 0) {
        return PlayerPose::Slide;  // on the ground and still carrying the speed of a ramp
    }
    if (player.on_ground && player.crouching) {
        return PlayerPose::Crouch;  // one pose, whether it is still or crawling
    }
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

bool isVisibleWhileInvulnerable(int invulnerable_steps) {
    // 3 steps shown, 3 steps hidden: a quick blink (10 times a second).
    return invulnerable_steps <= 0 || (invulnerable_steps / 3) % 2 == 0;
}

bool facesLeft(const domain::Player& player, bool was_facing_left) {
    if (player.hurt_steps > 0) {
        return was_facing_left;  // moving backwards, but still facing the danger
    }
    if (player.velocity_x < 0.0F) {
        return true;
    }
    if (player.velocity_x > 0.0F) {
        return false;
    }
    return was_facing_left;
}

int fruitFrame(std::uint64_t time_ms) {
    return static_cast<int>((time_ms / kFruitFrameMs) % 4);
}

int fruitBob(std::uint64_t time_ms) {
    constexpr int kBob[4] = {0, -1, 0, 1};  // middle, up, middle, down
    return kBob[(time_ms / kFruitBobMs) % 4];
}

int coinFrame(std::uint64_t time_ms) {
    return static_cast<int>((time_ms / kCoinFrameMs) % 4);
}

bool isVisibleWhileFading(int steps_left) {
    if (steps_left > domain::kSquashBlinkSteps) {
        return true;
    }
    return (steps_left / 3) % 2 == 0;
}

int checkpointFrame(std::uint64_t time_ms) {
    return 1 + static_cast<int>((time_ms / 350) % 2);
}

int exitFrame(std::uint64_t time_ms) {
    return static_cast<int>((time_ms / 600) % 2);
}

int enemyFrame(std::uint64_t time_ms) {
    return static_cast<int>((time_ms / kEnemyFrameMs) % 2);
}

}  // namespace presentation
