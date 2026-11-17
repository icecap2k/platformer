#include "presentation/drip.h"

#include <cmath>

namespace presentation {

namespace {

constexpr std::uint64_t kFormingMs = 600;   // two frames of 300 ms
constexpr std::uint64_t kSplashMs = 240;    // two frames of 120 ms
constexpr std::uint64_t kMinCycleMs = 5000;
constexpr std::uint64_t kCycleSpreadMs = 7000;
constexpr float kDropGravity = 0.0012F;     // pixels per ms^2: 100 px take about 0.4 s

// Mixes a tile position and a purpose (`salt`) into an unrelated number.
std::uint32_t mix(int column, int row, std::uint32_t salt) {
    std::uint32_t h = static_cast<std::uint32_t>(column) * 73856093U ^
                      static_cast<std::uint32_t>(row) * 19349663U ^ salt * 83492791U;
    h ^= h >> 13;
    h *= 0x5BD1E995U;
    h ^= h >> 15;
    return h;
}

}  // namespace

float fallTimeMs(float distance) {
    return std::sqrt(2.0F * distance / kDropGravity);
}

Drip dripAt(int column, int row, float fall_distance, std::uint64_t time_ms) {
    if (fall_distance < kMinFallDistance) {
        return {};
    }

    const std::uint64_t cycle = kMinCycleMs + mix(column, row, 1) % kCycleSpreadMs;
    const std::uint64_t shifted = time_ms + mix(column, row, 2) % cycle;
    const std::uint32_t cycle_index = static_cast<std::uint32_t>(shifted / cycle);
    const std::uint64_t local = shifted % cycle;

    // About one cycle in three stays dry, so the rhythm is not predictable.
    if (mix(column, row, 3 + cycle_index * 7U) % 3 == 0) {
        return {};
    }

    const auto fall_ms = static_cast<std::uint64_t>(fallTimeMs(fall_distance));
    const std::uint64_t total = kFormingMs + fall_ms + kSplashMs;
    if (total >= cycle) {
        return {};
    }
    const std::uint64_t start = mix(column, row, 4 + cycle_index * 7U) % (cycle - total);
    if (local < start || local >= start + total) {
        return {};
    }

    const std::uint64_t t = local - start;
    Drip drip;
    if (t < kFormingMs) {
        drip.phase = DripPhase::Forming;
        drip.frame = (t < kFormingMs / 2) ? kDripFormingSmall : kDripFormingLarge;
    } else if (t < kFormingMs + fall_ms) {
        const float seconds_falling = static_cast<float>(t - kFormingMs);
        drip.phase = DripPhase::Falling;
        drip.frame = kDripFalling;
        drip.fall_offset = std::fmin(0.5F * kDropGravity * seconds_falling * seconds_falling, fall_distance);
    } else {
        drip.phase = DripPhase::Splash;
        drip.frame = (t - kFormingMs - fall_ms < kSplashMs / 2) ? kDripSplashA : kDripSplashB;
    }
    return drip;
}

}  // namespace presentation
