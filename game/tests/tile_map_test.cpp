#include <doctest/doctest.h>

#include <stdexcept>

#include "domain/world/tile_map.h"

using domain::TileMap;

TEST_CASE("TileMap") {
    // 3x2 grid:  0 1 0
    //            2 0 0
    const TileMap map(3, 2, 8, {0, 1, 0, 2, 0, 0});

    SUBCASE("reports its size in tiles and pixels") {
        CHECK(map.width() == 3);
        CHECK(map.height() == 2);
        CHECK(map.pixelWidth() == 24.0F);
        CHECK(map.pixelHeight() == 16.0F);
    }

    SUBCASE("reads tiles in row-major order") {
        CHECK(map.tileAt(1, 0) == 1);
        CHECK(map.tileAt(0, 1) == 2);
        CHECK(map.tileAt(0, 0) == 0);
    }

    SUBCASE("non-zero tiles are solid") {
        CHECK(map.isSolid(1, 0));
        CHECK_FALSE(map.isSolid(2, 1));
    }

    SUBCASE("cells outside the grid are empty") {
        CHECK_FALSE(map.isSolid(-1, 0));
        CHECK_FALSE(map.isSolid(0, -1));
        CHECK_FALSE(map.isSolid(3, 0));
        CHECK_FALSE(map.isSolid(0, 2));
    }

    SUBCASE("rejects a tile list of the wrong size") {
        CHECK_THROWS_AS(TileMap(3, 2, 8, {0, 1}), std::invalid_argument);
    }

    SUBCASE("rejects non-positive dimensions") {
        CHECK_THROWS_AS(TileMap(0, 2, 8, {}), std::invalid_argument);
    }
}

TEST_CASE("TileMap ramps") {
    using domain::Side;
    using domain::TileShape;
    // 4x2 grid, tile 8 px:   5 6 1 0
    //                        1 1 0 0       (5 = rises to the right, 6 = to the left)
    const TileMap map(4, 2, 8, {domain::kTileSlopeRight, domain::kTileSlopeLeft, 1, 0, 1, 1, 0, 0});

    SUBCASE("ids 5 and 6 are ramps; they are not solid but are ground") {
        CHECK(map.shape(0, 0) == TileShape::SlopeRight);
        CHECK(map.shape(1, 0) == TileShape::SlopeLeft);
        CHECK(map.shape(2, 0) == TileShape::Solid);
        CHECK(map.shape(3, 0) == TileShape::Empty);

        CHECK_FALSE(map.isSolid(0, 0));
        CHECK_FALSE(map.isSolid(1, 0));
        CHECK(map.isSlope(0, 0));
        CHECK_FALSE(map.isSlope(2, 0));
        CHECK(map.isGround(0, 0));
        CHECK(map.isGround(2, 0));
        CHECK_FALSE(map.isGround(3, 0));
        CHECK_FALSE(map.isGround(-1, 0));  // outside the grid
    }

    SUBCASE("a ramp rising to the right is low on the left and high on the right") {
        CHECK(map.slopeSurfaceY(0, 0, 0.0F) == doctest::Approx(8.0F));  // bottom of the tile
        CHECK(map.slopeSurfaceY(0, 0, 4.0F) == doctest::Approx(4.0F));
        CHECK(map.slopeSurfaceY(0, 0, 8.0F) == doctest::Approx(0.0F));  // top of the tile
    }

    SUBCASE("a ramp rising to the left is the mirror image: high on the left, low on the right") {
        // The tile spans x = 8..16.
        CHECK(map.slopeSurfaceY(1, 0, 8.0F) == doctest::Approx(0.0F));   // top of the tile
        CHECK(map.slopeSurfaceY(1, 0, 12.0F) == doctest::Approx(4.0F));
        CHECK(map.slopeSurfaceY(1, 0, 16.0F) == doctest::Approx(8.0F));  // bottom of the tile
    }

    SUBCASE("positions outside the tile are clamped; other tiles have no slope surface") {
        CHECK(map.slopeSurfaceY(0, 0, -10.0F) == doctest::Approx(8.0F));
        CHECK(map.slopeSurfaceY(0, 0, 99.0F) == doctest::Approx(0.0F));
        CHECK_FALSE(map.slopeSurfaceY(2, 0, 20.0F).has_value());
        CHECK_FALSE(map.slopeSurfaceY(3, 0, 28.0F).has_value());
    }

    SUBCASE("which faces of the cell a tile fills") {
        for (const Side face : {Side::North, Side::South, Side::East, Side::West}) {
            CHECK(map.blocksFace(2, 0, face));         // a block fills everything
            CHECK_FALSE(map.blocksFace(3, 0, face));   // nothing fills an empty cell
        }
        CHECK(map.blocksFace(0, 0, Side::South));
        CHECK(map.blocksFace(0, 0, Side::East));
        CHECK_FALSE(map.blocksFace(0, 0, Side::North));
        CHECK_FALSE(map.blocksFace(0, 0, Side::West));

        CHECK(map.blocksFace(1, 0, Side::South));
        CHECK(map.blocksFace(1, 0, Side::West));
        CHECK_FALSE(map.blocksFace(1, 0, Side::North));
        CHECK_FALSE(map.blocksFace(1, 0, Side::East));
    }
}
