#pragma once

#include <vector>

#include "domain/world/tile_map.h"

namespace test_maps {

constexpr int kTileSize = 8;
constexpr int kWidth = 40;   // 320 px
constexpr int kHeight = 30;  // 240 px
constexpr int kFloorRow = 27;  // floor top is at y = 216

// Empty room with a solid floor from row 27 down. Callers can add extra solid
// tiles with `solid` to build the scenario they need.
inline domain::TileMap makeMap(const std::vector<std::pair<int, int>>& solid = {}) {
    std::vector<int> tiles(static_cast<std::size_t>(kWidth * kHeight), 0);
    for (int row = kFloorRow; row < kHeight; ++row) {
        for (int column = 0; column < kWidth; ++column) {
            tiles[static_cast<std::size_t>(row * kWidth + column)] = 1;
        }
    }
    for (const auto& [column, row] : solid) {
        tiles[static_cast<std::size_t>(row * kWidth + column)] = 1;
    }
    return domain::TileMap(kWidth, kHeight, kTileSize, std::move(tiles));
}

constexpr float kFloorTop = static_cast<float>(kFloorRow * kTileSize);

}  // namespace test_maps
