#pragma once

#include <vector>

namespace domain {

// A rectangular grid of tiles. Each cell holds a tile id; 0 means "empty".
// For now every non-zero tile is solid. Later epics can map ids to richer
// behaviours (one-way platforms, spikes...) without changing the callers.
class TileMap {
public:
    // `tiles` is row-major (left to right, top to bottom) and must contain
    // exactly width * height ids. Throws std::invalid_argument otherwise.
    TileMap(int width, int height, int tile_size, std::vector<int> tiles);

    int width() const { return width_; }    // in tiles
    int height() const { return height_; }  // in tiles
    int tileSize() const { return tile_size_; }
    float pixelWidth() const { return static_cast<float>(width_ * tile_size_); }
    float pixelHeight() const { return static_cast<float>(height_ * tile_size_); }

    // Cells outside the grid are empty (id 0).
    int tileAt(int tile_x, int tile_y) const;
    bool isSolid(int tile_x, int tile_y) const { return tileAt(tile_x, tile_y) != 0; }

private:
    int width_;
    int height_;
    int tile_size_;
    std::vector<int> tiles_;
};

}  // namespace domain
