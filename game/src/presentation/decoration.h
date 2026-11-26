#pragma once

#include <array>
#include <vector>

#include "domain/world/room.h"

namespace presentation {

// Purely cosmetic scenery: big mushrooms, crystals, vines... growing on the floor, hanging from
// the ceiling and sticking out of the walls. It never affects the game. It is a pure function of
// the room (its id, tiles and what is placed in it), so a room always looks the same and nothing
// needs to be stored in the level files.

// The value is the column of the prop's 32x32 cell in assets/sprites/decor.png, drawn by
// tools/art/make_cave_decor.py. Keep both in sync.
enum class PropType {
    GiantMushroom = 0, PinkMushrooms, CyanCrystals, VioletCrystals, Stalagmites, GlowPlant, Pebbles,
    SmallMushroom,                                            // on the floor
    StalactiteCluster = 8, Vines, Roots, HangingCrystals,     // from the ceiling
    ShelfMushrooms = 12, WallCrystals, WallVine, WallStalks,  // on a wall
};
inline constexpr int kPropTypeCount = 16;
inline constexpr int kPropCellSize = 32;

// The side of the prop that touches the rock.
enum class Attach { Floor, Ceiling, Wall };

struct PropShape {
    int width;   // in tiles
    int height;  // in tiles
    Attach attach;
};

PropShape propShape(PropType type);

// Where the water drop of a StalactiteCluster forms: the pixel just under its longest point,
// measured from the prop's top left corner.
inline constexpr int kDripTipX = 4;
inline constexpr int kDripTipY = 23;

struct Prop {
    PropType type;
    int column;     // top left tile
    int row;
    bool mirrored;  // wall props are drawn with the rock on their left: mirrored = rock on the right
};

// Everything that decorates a room. Props occupy empty tiles only, rest against solid rock, do not
// overlap each other and keep clear of hazards, pickups, checkpoints, exits, trap stalactites and
// the HUD.
std::vector<Prop> sceneryFor(const domain::Room& room);

}  // namespace presentation
