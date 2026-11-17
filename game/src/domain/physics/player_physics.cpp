#include "domain/physics/player_physics.h"

#include <algorithm>
#include <cmath>

namespace domain {

namespace {

int tileIndexAt(float pixel, int tile_size) {
    return static_cast<int>(std::floor(pixel / static_cast<float>(tile_size)));
}

// Index of the last tile that a span ending at `end_pixel` actually covers.
// A span that ends exactly on a tile border does NOT cover the next tile.
int lastTileIndexBefore(float end_pixel, int tile_size) {
    return static_cast<int>(std::ceil(end_pixel / static_cast<float>(tile_size))) - 1;
}

bool anySolidInColumn(const TileMap& map, int tile_x, int first_row, int last_row) {
    for (int row = first_row; row <= last_row; ++row) {
        if (map.isSolid(tile_x, row)) {
            return true;
        }
    }
    return false;
}

bool anySolidInRow(const TileMap& map, int tile_y, int first_column, int last_column) {
    for (int column = first_column; column <= last_column; ++column) {
        if (map.isSolid(column, tile_y)) {
            return true;
        }
    }
    return false;
}

void moveHorizontally(Player& player, const TileMap& map) {
    const int tile_size = map.tileSize();
    player.x += player.velocity_x;

    const int first_row = tileIndexAt(player.y, tile_size);
    const int last_row = lastTileIndexBefore(player.y + player.height, tile_size);

    if (player.velocity_x > 0.0F) {
        const int column = lastTileIndexBefore(player.x + player.width, tile_size);
        if (anySolidInColumn(map, column, first_row, last_row)) {
            player.x = static_cast<float>(column * tile_size) - player.width;
        }
    } else if (player.velocity_x < 0.0F) {
        const int column = tileIndexAt(player.x, tile_size);
        if (anySolidInColumn(map, column, first_row, last_row)) {
            player.x = static_cast<float>((column + 1) * tile_size);
        }
    }
    // Map edges are NOT handled here: whether leaving the room is possible is
    // a rule of the room graph (see rules/room_transition.h), not of physics.
}

void moveVertically(Player& player, const TileMap& map) {
    const int tile_size = map.tileSize();
    player.y += player.velocity_y;
    player.on_ground = false;

    const int first_column = tileIndexAt(player.x, tile_size);
    const int last_column = lastTileIndexBefore(player.x + player.width, tile_size);

    if (player.velocity_y > 0.0F) {
        const int row = lastTileIndexBefore(player.y + player.height, tile_size);
        if (anySolidInRow(map, row, first_column, last_column)) {
            player.y = static_cast<float>(row * tile_size) - player.height;
            player.velocity_y = 0.0F;
            player.on_ground = true;
        }
    } else if (player.velocity_y < 0.0F) {
        const int row = tileIndexAt(player.y, tile_size);
        if (anySolidInRow(map, row, first_column, last_column)) {
            player.y = static_cast<float>((row + 1) * tile_size);
            player.velocity_y = 0.0F;  // bumped the head: stop rising
        }
    }
}

}  // namespace

void stepPlayer(Player& player, const InputState& input, const TileMap& map) {
    // Horizontal movement: direct control, no acceleration.
    player.velocity_x = static_cast<float>(input.horizontalAxis()) * kRunSpeed;
    moveHorizontally(player, map);

    // Jump: only allowed while standing on something.
    if (input.jump && player.on_ground) {
        player.velocity_y = -kJumpSpeed;
    }

    // Gravity, with a terminal velocity so falls stay controllable.
    player.velocity_y = std::min(player.velocity_y + kGravity, kMaxFallSpeed);
    moveVertically(player, map);
}

}  // namespace domain
