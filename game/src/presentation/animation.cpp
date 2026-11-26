#include "presentation/animation.h"

#include "domain/enemies/enemy_patrols.h"

namespace presentation {

namespace {
constexpr std::uint64_t kIdleFrameMs = 220;
constexpr std::uint64_t kRunFrameMs = 65;  // a full stride (two steps) every 0.52 s
constexpr std::uint64_t kLandMs = 100;
constexpr std::uint64_t kCoinFrameMs = 140;
constexpr std::uint64_t kFruitFrameMs = 160;  // a quick breath: a full pulse every 0.64 s
constexpr std::uint64_t kEnemyFrameMs = 110;
constexpr std::uint64_t kOrbFrameMs = 110;     // a heartbeat every 1.3 s
constexpr std::uint64_t kOrbBobMs = 260;
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
                                   static_cast<int>((time_ms / kRunFrameMs) % kRunFrames));
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

int orbFrame(domain::OrbColour colour, std::uint64_t time_ms) {
    const int offset = static_cast<int>(colour) * (kOrbFrames / 3);
    return static_cast<int>((time_ms / kOrbFrameMs + static_cast<std::uint64_t>(offset)) % kOrbFrames);
}

int orbBob(domain::OrbColour colour, std::uint64_t time_ms) {
    constexpr int kBob[8] = {0, -1, -2, -1, 0, 1, 2, 1};
    const std::uint64_t step = time_ms / kOrbBobMs + static_cast<std::uint64_t>(colour) * 3U;
    return kBob[step % 8];
}

int hairFlutter(PlayerPose pose, std::uint64_t time_ms) {
    std::uint64_t frame_ms = 140;  // running, sliding, rising
    switch (pose) {
        case PlayerPose::Idle0:
        case PlayerPose::Idle1:
        case PlayerPose::Idle2:
        case PlayerPose::Idle3:
        case PlayerPose::Land:
            frame_ms = 340;
            break;
        case PlayerPose::Crouch:
            frame_ms = 420;
            break;
        case PlayerPose::Fall:
        case PlayerPose::Hurt:
            frame_ms = 110;
            break;
        default:
            break;
    }
    return static_cast<int>((time_ms / frame_ms) % kHairFlutterFrames);
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

int exitSpriteFrame(bool open, std::uint64_t ms_open, std::uint64_t time_ms) {
    if (!open || ms_open < kGateDelayMs) {
        return kExitClosedFrame;
    }
    const std::uint64_t step = (ms_open - kGateDelayMs) / kGateFrameMs;
    if (step < static_cast<std::uint64_t>(kGateFrames)) {
        return kGateFirstFrame + static_cast<int>(step);
    }
    return exitFrame(time_ms);
}

int enemyFrame(std::uint64_t time_ms) {
    return static_cast<int>((time_ms / kEnemyFrameMs) % kEnemyWalkFrames);
}

}  // namespace presentation
