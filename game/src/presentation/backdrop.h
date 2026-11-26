#pragma once

#include <cstdint>
#include <vector>

#include "domain/world/room.h"

namespace presentation {

// The big glowing mushrooms that grow far behind a room. Purely cosmetic (ADR-0008): where they
// stand is a function of the room and what they look like at a moment is a function of the clock.
// They are drawn between the cave picture and the rock, so the level hides whatever is in front.
//
// Pure logic (no SDL), so it is unit tested.

// The value is the row of assets/sprites/glow_mushrooms.png, drawn by
// tools/art/make_glow_mushrooms.py. Keep both in sync.
enum class MushroomType { Tall = 0, Wide, Cluster };
inline constexpr int kMushroomTypes = 3;
inline constexpr int kMushroomFrames = 8;
inline constexpr int kMushroomCellWidth = 48;
inline constexpr int kMushroomCellHeight = 64;

struct BackdropMushroom {
    MushroomType type;
    float x;       // centre of its foot
    float base_y;  // where it stands (the bottom of its cell)
    bool mirrored;
    std::uint64_t offset_ms;  // so they do not glow in unison
};

// Up to one mushroom in each of five slices of the room, standing on its floor. A slice whose
// floor is missing (a pit) or very high (a rock mass) stays empty.
std::vector<BackdropMushroom> backdropFor(const domain::Room& room);

// Frame (0..kMushroomFrames - 1) to draw at `time_ms`.
int mushroomFrame(const BackdropMushroom& mushroom, std::uint64_t time_ms);

}  // namespace presentation
