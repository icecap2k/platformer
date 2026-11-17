#pragma once

#include <string>

#include "domain/world/room.h"

namespace infrastructure {

// Builds a Room from the text of a Tiled JSON map (.tmj).
//
// - The tiles come from the first tile layer.
// - Neighbours come from string custom properties on the map itself named
//   "north", "south", "east" and "west", whose value is the id of the room
//   on the other side of that edge.
// - Coins come from rectangle objects whose class is "coin".
//
// Throws std::runtime_error with a readable message if the map is malformed
// or uses an unsupported feature. It does not touch SDL, so it can be unit
// tested.
domain::Room parseTiledRoom(const std::string& room_id, const std::string& json_text);

// Reads the file at `path` and parses it with parseTiledRoom. The room id is
// the file name without extension ("assets/tiled/room_02.tmj" -> "room_02").
domain::Room loadTiledRoom(const std::string& path);

}  // namespace infrastructure
