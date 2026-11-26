#pragma once

#include <map>
#include <string>
#include <vector>

#include "domain/world/room.h"

namespace level_tools {

// A level drawn as plain text. Example:
//
//   # lines starting with '#' above the separator are comments
//   east: room_02
//   ---
//   ........
//   ...C....
//   ..BBBB..
//   ########
//
// Header (above the line "---"): `north`, `south`, `east`, `west` = id of the
// neighbouring room; `tile_size` = pixels per tile (default 8).
// Grid (below it), one character per tile:
//   E and the orbs are TWO tiles big (16 px with the default tiles): centred on their tile and
//   resting on its bottom edge.
//   .  empty        #  ground (tile 1)        B  brick (tile 2)
//   3  tile 3 of the tileset    4  tile 4 of the tileset (both solid, drawn as brick)
//   C  coin on an empty tile    ^  spikes standing on the bottom of an empty tile
//   F  a healing fruit on an empty tile
//   X  the exit of the cave, on an empty tile: touching it wins the game
//   K  a checkpoint on an empty tile (it comes back there after losing every heart)
//   E  a patrolling enemy (a walker) standing on the floor: put it on an empty tile with ground
//      under it
//   R G Y  a floating orb (red-orange, green, yellow); all of them open the exit
//   V  a stalactite trap hanging in an empty tile (put it under a ceiling)
//   /  a 45 degree ramp rising to the right (tile 5)    \  rising to the left (tile 6)
//      A ramp must stand on solid tiles: filler under it and on its high side.
inline constexpr int kTileGround = 1;
inline constexpr int kTileBrick = 2;
inline constexpr int kTileThree = 3;  // other solid tiles of the tileset, kept so Tiled maps
inline constexpr int kTileFour = 4;   // that use them still convert without loss

// A `^` is a spike hazard as wide as its tile and one and a half tiles tall (12 px with the
// default tiles), standing on the tile's bottom edge: it reaches half way into the tile above.
// Shipped rooms write it in a one-tile trench in the floor. Spikes of any other shape live in
// Tiled maps only.
inline constexpr float kSpikeHeightTiles = 1.5F;

struct TextLevel {
    int tile_size = 8;
    int width = 0;   // in tiles
    int height = 0;  // in tiles
    std::vector<int> tiles;  // row-major, 0 = empty
    std::vector<domain::Coin> coins;
    std::vector<domain::Hazard> hazards;
    std::vector<domain::FallingStalactite> stalactites;
    std::vector<domain::Fruit> fruits;
    std::vector<domain::Enemy> enemies;
    std::vector<domain::Checkpoint> checkpoints;
    std::vector<domain::Exit> exits;
    std::vector<domain::Orb> orbs;
    std::map<domain::Side, domain::RoomId> neighbors;
};

// Throws std::runtime_error with a "line N: ..." message on bad input.
TextLevel parseTextLevel(const std::string& text);

// The level as a game Room (so it can be analysed without writing files).
domain::Room toRoom(const domain::RoomId& id, const TextLevel& level);

// The level as a Tiled map (.tmj JSON) that the game loads and Tiled opens.
std::string toTiledJson(const TextLevel& level);

// Draws an existing room as text. Coins snap to the nearest tile. Throws
// std::runtime_error for tile ids the text format cannot express, for a coin or fruit
// that sits on a solid tile, for a hazard that is not a tile-aligned patch of spikes (one or more tiles wide), or for a stalactite trap that is not
// exactly one tile.
std::string toText(const domain::Room& room);

}  // namespace level_tools
