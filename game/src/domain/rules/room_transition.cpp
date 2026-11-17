#include "domain/rules/room_transition.h"

#include <algorithm>

namespace domain {

OpenEdges openEdgesOf(const Room& room) {
    OpenEdges open;
    open.north = room.neighbor(Side::North).has_value();
    open.south = room.neighbor(Side::South).has_value();
    open.east = room.neighbor(Side::East).has_value();
    open.west = room.neighbor(Side::West).has_value();
    return open;
}

void confineToMap(Player& player, const TileMap& map, const OpenEdges& open) {
    if (!open.west && player.x < 0.0F) {
        player.x = 0.0F;
    }
    if (!open.east && player.x + player.width > map.pixelWidth()) {
        player.x = map.pixelWidth() - player.width;
    }
    if (!open.north && player.y < 0.0F) {
        player.y = 0.0F;
        player.velocity_y = std::max(player.velocity_y, 0.0F);  // bump the ceiling
    }
    if (!open.south && player.y + player.height > map.pixelHeight()) {
        player.y = map.pixelHeight() - player.height;
        player.velocity_y = 0.0F;
        player.on_ground = true;  // the bottom edge acts as a floor
    }
}

std::optional<Side> exitedThrough(const Player& player, const TileMap& map) {
    const float center_x = player.x + player.width / 2.0F;
    const float center_y = player.y + player.height / 2.0F;

    if (center_x < 0.0F) {
        return Side::West;
    }
    if (center_x >= map.pixelWidth()) {
        return Side::East;
    }
    if (center_y < 0.0F) {
        return Side::North;
    }
    if (center_y >= map.pixelHeight()) {
        return Side::South;
    }
    return std::nullopt;
}

void enterRoom(Player& player, Side exited_through, const TileMap& new_map) {
    const float max_x = new_map.pixelWidth() - player.width;
    const float max_y = new_map.pixelHeight() - player.height;

    switch (exited_through) {
        case Side::East:  // appears on the west edge of the new room
            player.x = 0.0F;
            player.y = std::clamp(player.y, 0.0F, max_y);
            break;
        case Side::West:  // appears on the east edge
            player.x = max_x;
            player.y = std::clamp(player.y, 0.0F, max_y);
            break;
        case Side::South:  // appears at the top
            player.y = 0.0F;
            player.x = std::clamp(player.x, 0.0F, max_x);
            break;
        case Side::North:  // appears at the bottom
            player.y = max_y;
            player.x = std::clamp(player.x, 0.0F, max_x);
            break;
    }
}

}  // namespace domain
