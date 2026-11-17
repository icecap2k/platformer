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
