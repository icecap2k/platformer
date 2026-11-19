#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "domain/world/tile_map.h"

namespace presentation {

// Fireflies drifting through the cave. Purely cosmetic (ADR-0008): where they are is a
// function of the room and the clock, so they need no state, never touch the game and a
// room looks the same every run.
//
// Each firefly repeats a cycle of four parts:
//   hover 3 s (flapping in place)  ->  fly one leg  ->  hover 3 s  ->  fly back
// A leg turns the firefly round and curves it: 5 tiles sideways and 2 tiles down (the way
// back retraces it, so it climbs again). Every firefly has its own offset in time, so
// they never start moving together.
//
// Pure logic (no SDL), so it is unit tested.

inline constexpr int kFirefliesPerRoom = 3;
inline constexpr int kFireflyLegTilesAcross = 5;
inline constexpr int kFireflyLegTilesDown = 2;
inline constexpr std::uint64_t kFireflyHoverMs = 3000;
inline constexpr std::uint64_t kFireflyLegMs = 2400;
inline constexpr std::uint64_t kFireflyCycleMs = 2 * (kFireflyHoverMs + kFireflyLegMs);

// Where a firefly lives: the tile it hovers over at the start of its cycle, the way its
// first leg goes (-1 left, +1 right) and its offset in time. The whole box of its flight
// (the 5 x 2 tiles of the leg, plus the one it occupies) is free of ground.
struct FireflyRoute {
    int column = 0;
    int row = 0;
    int direction = 1;
    std::uint64_t offset_ms = 0;
    int tile_size = 8;

    bool operator==(const FireflyRoute&) const = default;
};

struct Firefly {
    float x = 0.0F;  // top-left corner of its 8x8 sprite
    float y = 0.0F;
    bool facing_left = false;
    bool hovering = false;
    int frame = 0;  // 0..3 in assets/sprites/fireflies.png: flap + 2 * dim
};

// The routes of the room (up to kFirefliesPerRoom: a cramped room may have fewer).
// Deterministic: the same room id and map always give the same routes.
std::vector<FireflyRoute> fireflyRoutes(const std::string& room_id, const domain::TileMap& map);

// Where the firefly of `route` is at `time_ms`.
Firefly fireflyAt(const FireflyRoute& route, std::uint64_t time_ms);

}  // namespace presentation
