#pragma once

#include <optional>
#include <vector>

#include "domain/world/side.h"

namespace domain {

// What a tile is, physically. Ids 1-4 (and any other non-zero id) are whole
// solid blocks; 5 and 6 are 45 degree ramps. The ramp art and rules are in
// ADR-0010.
inline constexpr int kTileSlopeRight = 5;  // rises towards the right (bottom-right triangle)
inline constexpr int kTileSlopeLeft = 6;   // rises towards the left (bottom-left triangle)

enum class TileShape { Empty, Solid, SlopeRight, SlopeLeft };

// A rectangular grid of tiles. Each cell holds a tile id; 0 means "empty".
// Whole blocks are `isSolid`; ramps are not solid (the player walks onto them)
// but have a surface, see surfaceY. `isGround` is "has a surface": a block or
// a ramp.
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
    TileShape shape(int tile_x, int tile_y) const;

    // A whole block: collides on every side. Ramps are NOT solid in this sense.
    bool isSolid(int tile_x, int tile_y) const { return shape(tile_x, tile_y) == TileShape::Solid; }
    bool isSlope(int tile_x, int tile_y) const;
    // Something to stand on: a block or a ramp.
    bool isGround(int tile_x, int tile_y) const { return shape(tile_x, tile_y) != TileShape::Empty; }

    // For a ramp tile: the y (in pixels) of its surface above the pixel column
    // `x` (clamped to the tile). std::nullopt if the tile is not a ramp.
    std::optional<float> slopeSurfaceY(int tile_x, int tile_y, float x) const;

    // Does the tile fill the given face of its cell? A block fills all four; a
    // ramp that rises to the right fills its south and east faces, one that
    // rises to the left its south and west faces. Used to decide which sides of
    // a neighbour are really touching air.
    bool blocksFace(int tile_x, int tile_y, Side face) const;

private:
    int width_;
    int height_;
    int tile_size_;
    std::vector<int> tiles_;
};

}  // namespace domain
