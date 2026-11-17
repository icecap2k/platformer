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
//   .  empty        #  ground (tile 1)        B  brick (tile 2)
//   3  tile 3 of the tileset    4  tile 4 of the tileset (both solid, drawn as brick)
//   C  coin on an empty tile
inline constexpr int kTileGround = 1;
inline constexpr int kTileBrick = 2;
inline constexpr int kTileThree = 3;  // other solid tiles of the tileset, kept so Tiled maps
inline constexpr int kTileFour = 4;   // that use them still convert without loss

struct TextLevel {
    int tile_size = 8;
    int width = 0;   // in tiles
    int height = 0;  // in tiles
    std::vector<int> tiles;  // row-major, 0 = empty
    std::vector<domain::Coin> coins;
    std::map<domain::Side, domain::RoomId> neighbors;
};

// Throws std::runtime_error with a "line N: ..." message on bad input.
TextLevel parseTextLevel(const std::string& text);

// The level as a game Room (so it can be analysed without writing files).
domain::Room toRoom(const domain::RoomId& id, const TextLevel& level);

// The level as a Tiled map (.tmj JSON) that the game loads and Tiled opens.
std::string toTiledJson(const TextLevel& level);

// Draws an existing room as text. Coins snap to the nearest tile. Throws
// std::runtime_error for tile ids the text format cannot express, or for a coin
// that sits on a solid tile.
std::string toText(const domain::Room& room);

}  // namespace level_tools
