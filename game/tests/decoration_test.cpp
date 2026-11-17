#include <doctest/doctest.h>

#include <set>

#include "presentation/decoration.h"

using namespace presentation;

TEST_CASE("decorations are deterministic") {
    for (int column = 0; column < 40; ++column) {
        CHECK(standingDecoration(column, 27) == standingDecoration(column, 27));
        CHECK(hangingDecoration(column, 22) == hangingDecoration(column, 22));
    }
}

TEST_CASE("standing decorations") {
    std::set<int> seen;
    int none = 0;
    constexpr int kTiles = 40 * 30;

    for (int row = 0; row < 30; ++row) {
        for (int column = 0; column < 40; ++column) {
            const int frame = standingDecoration(column, row);
            if (frame == kNoDecoration) {
                ++none;
            } else {
                CHECK(frame >= kDecorFirstStanding);
                CHECK(frame <= kDecorLastStanding);
                seen.insert(frame);
            }
        }
    }

    SUBCASE("every kind of standing decoration shows up somewhere") {
        CHECK(seen.size() == static_cast<std::size_t>(kDecorLastStanding - kDecorFirstStanding + 1));
    }

    SUBCASE("about half of the tiles stay bare") {
        CHECK(none > kTiles * 40 / 100);
        CHECK(none < kTiles * 60 / 100);
    }
}

TEST_CASE("hanging decorations") {
    int vines = 0;
    for (int row = 0; row < 30; ++row) {
        for (int column = 0; column < 40; ++column) {
            const int frame = hangingDecoration(column, row);
            if (frame != kNoDecoration) {
                ++vines;
                CHECK(frame >= kDecorFirstHanging);
                CHECK(frame <= kDecorLastHanging);
            }
        }
    }

    CHECK(vines > 0);
    CHECK(vines < 40 * 30 / 2);  // sparse
}
