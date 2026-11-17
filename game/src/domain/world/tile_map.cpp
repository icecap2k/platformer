#include "domain/world/tile_map.h"

#include <cstddef>
#include <stdexcept>
#include <utility>

namespace domain {

TileMap::TileMap(int width, int height, int tile_size, std::vector<int> tiles)
    : width_(width), height_(height), tile_size_(tile_size), tiles_(std::move(tiles)) {
    if (width_ <= 0 || height_ <= 0 || tile_size_ <= 0) {
        throw std::invalid_argument("TileMap dimensions must be positive");
    }
    if (tiles_.size() != static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_)) {
        throw std::invalid_argument("TileMap tile count does not match width * height");
    }
}

int TileMap::tileAt(int tile_x, int tile_y) const {
    if (tile_x < 0 || tile_y < 0 || tile_x >= width_ || tile_y >= height_) {
        return 0;
    }
    return tiles_[static_cast<std::size_t>(tile_y) * static_cast<std::size_t>(width_) +
                  static_cast<std::size_t>(tile_x)];
}

}  // namespace domain
