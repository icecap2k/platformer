#include "domain/world/tile_map.h"

#include <algorithm>
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

TileShape TileMap::shape(int tile_x, int tile_y) const {
    switch (tileAt(tile_x, tile_y)) {
        case 0:
            return TileShape::Empty;
        case kTileSlopeRight:
            return TileShape::SlopeRight;
        case kTileSlopeLeft:
            return TileShape::SlopeLeft;
        default:
            return TileShape::Solid;
    }
}

bool TileMap::isSlope(int tile_x, int tile_y) const {
    const TileShape tile_shape = shape(tile_x, tile_y);
    return tile_shape == TileShape::SlopeRight || tile_shape == TileShape::SlopeLeft;
}

std::optional<float> TileMap::slopeSurfaceY(int tile_x, int tile_y, float x) const {
    const TileShape tile_shape = shape(tile_x, tile_y);
    if (tile_shape != TileShape::SlopeRight && tile_shape != TileShape::SlopeLeft) {
        return std::nullopt;
    }
    const float size = static_cast<float>(tile_size_);
    const float left = static_cast<float>(tile_x) * size;
    const float local = std::min(std::max(x - left, 0.0F), size);
    const float top = static_cast<float>(tile_y) * size;
    // 45 degrees: the surface moves one pixel up for every pixel across.
    return tile_shape == TileShape::SlopeRight ? top + size - local : top + local;
}

bool TileMap::blocksFace(int tile_x, int tile_y, Side face) const {
    switch (shape(tile_x, tile_y)) {
        case TileShape::Empty:
            return false;
        case TileShape::Solid:
            return true;
        case TileShape::SlopeRight:
            return face == Side::South || face == Side::East;
        case TileShape::SlopeLeft:
            return face == Side::South || face == Side::West;
    }
    return false;
}

}  // namespace domain
