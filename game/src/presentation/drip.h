#pragma once

#include <cstdint>

namespace presentation {

// Water drops that fall from the stalactites. Purely cosmetic: the drop is a
// function of the tile position and the clock, so it needs no state, never
// touches the game, and a room looks the same every run.
//
// Every stalactite has its own rhythm, so they never drip together:
//   - its own cycle length (5 to 12 seconds) and its own offset in time;
//   - in each cycle a drop forms at a random moment, and about one cycle in
//     three has no drop at all.
//
// Pure logic (no SDL), so it is unit tested.

enum class DripPhase {
    None,     // nothing to draw
    Forming,  // a drop swells at the tip of the stalactite
    Falling,  // the drop falls (accelerating, like water)
    Splash,   // it hits the ground below
};

struct Drip {
    DripPhase phase = DripPhase::None;
    int frame = 0;             // frame of assets/sprites/decor.png to draw
    float fall_offset = 0.0F;  // while Falling: pixels the drop has fallen
};

// Frames of the drop animation in decor.png (see decoration.h).
inline constexpr int kDripFormingSmall = 8;
inline constexpr int kDripFormingLarge = 9;
inline constexpr int kDripFalling = 10;
inline constexpr int kDripSplashA = 11;
inline constexpr int kDripSplashB = 12;

inline constexpr float kMinFallDistance = 8.0F;  // closer to the ground than this: no drop

// The drop of the stalactite hanging from the tile at (column, row) at time
// `time_ms`. `fall_distance` is how many pixels the drop can fall before it
// reaches something solid (or the bottom of the screen).
Drip dripAt(int column, int row, float fall_distance, std::uint64_t time_ms);

// How long a drop takes to fall `distance` pixels, in milliseconds.
float fallTimeMs(float distance);

}  // namespace presentation
