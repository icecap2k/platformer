#include <doctest/doctest.h>

#include <set>

#include "presentation/autotile.h"

using namespace presentation;

TEST_CASE("exposureMask") {
    CHECK(exposureMask(false, false, false, false) == 0);
    CHECK(exposureMask(true, false, false, false) == kExposedNorth);
    CHECK(exposureMask(false, true, false, false) == kExposedSouth);
    CHECK(exposureMask(false, false, true, false) == kExposedEast);
    CHECK(exposureMask(false, false, false, true) == kExposedWest);
    CHECK(exposureMask(true, true, true, true) == kExposureMasks - 1);

    SUBCASE("every combination gets its own column inside the sheet") {
        std::set<int> seen;
        for (int bits = 0; bits < 16; ++bits) {
            const int mask = exposureMask(bits & 1, bits & 2, bits & 4, bits & 8);
            CHECK(mask >= 0);
            CHECK(mask < kExposureMasks);
            seen.insert(mask);
        }
        CHECK(seen.size() == 16);
    }
}

TEST_CASE("rockSheetRow") {
    SUBCASE("ground uses the first rows and any other tile uses the rock rows") {
        for (int column = 0; column < 40; ++column) {
            for (int row = 0; row < 30; ++row) {
                const int ground = rockSheetRow(1, column, row);
                const int rock = rockSheetRow(2, column, row);
                CHECK(ground >= 0);
                CHECK(ground < kRockVariants);
                CHECK(rock >= kRockVariants);
                CHECK(rock < kRockRows);
                CHECK(rockSheetRow(3, column, row) == rock);
            }
        }
    }

    SUBCASE("it is deterministic and uses all the variants") {
        std::set<int> variants;
        for (int column = 0; column < 40; ++column) {
            CHECK(rockSheetRow(2, column, 5) == rockSheetRow(2, column, 5));
            variants.insert(rockSheetRow(2, column, 5));
        }
        CHECK(variants.size() == kRockVariants);
    }
}
