#pragma once

#include <optional>

#include "domain/entities/player.h"
#include "domain/world/room.h"
#include "domain/world/side.h"
#include "domain/world/tile_map.h"

namespace domain {

// Which edges of a room can be crossed. The others behave like walls.
struct OpenEdges {
    bool north = false;
    bool south = false;
    bool east = false;
    bool west = false;
};

OpenEdges openEdgesOf(const Room& room);

// Makes closed edges solid: the player cannot leave through them. Open edges
// are left alone so the player can walk out.
void confineToMap(Player& player, const TileMap& map, const OpenEdges& open);

// Which edge has the player's centre crossed, if any? Using the centre (not
// the whole body) means the room changes when the player is half way out.
std::optional<Side> exitedThrough(const Player& player, const TileMap& map);

// Places the player just inside `new_map`, on the edge opposite to the one it
// left through. Speed is kept, so a jump or a fall continues across the
// screens. The other coordinate is kept too (clamped to the new room).
void enterRoom(Player& player, Side exited_through, const TileMap& new_map);

}  // namespace domain
