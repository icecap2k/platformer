#include <doctest/doctest.h>

#include <set>
#include <string>

#include "level_tools/text_level.h"
#include "presentation/backdrop.h"

using namespace presentation;

namespace {

// A 40 x 30 room with a floor at row 27, except where `gap` says there is a pit.
domain::Room cave(const std::string& id, bool gap = false) {
    std::string text = "---\n";
    for (int row = 0; row < 30; ++row) {
        std::string line(40, row >= 27 ? '#' : '.');
        if (row >= 27 && gap) {
            line.replace(8, 16, 16, '.');  // the whole second and third slices
        }
        text += line + "\n";
    }
    return level_tools::toRoom(id, level_tools::parseTextLevel(text));
}

}  // namespace

TEST_CASE("background mushrooms stand on the floor, spread across the room") {
    std::set<MushroomType> seen;
    for (const char* id : {"a", "b", "c", "d", "e", "f"}) {
        const auto mushrooms = backdropFor(cave(id));
        REQUIRE(mushrooms.size() == 5);
        for (std::size_t i = 0; i < mushrooms.size(); ++i) {
            seen.insert(mushrooms[i].type);
            CHECK(mushrooms[i].base_y == 27 * 8 + 2);
            CHECK(mushrooms[i].x > static_cast<float>(i) * 64.0F);  // one per slice of 64 px
            CHECK(mushrooms[i].x < static_cast<float>(i + 1) * 64.0F);
        }
    }
    CHECK(seen.size() == static_cast<std::size_t>(kMushroomTypes));
}

TEST_CASE("no mushroom floats over a pit") {
    const auto mushrooms = backdropFor(cave("a", true));
    CHECK(mushrooms.size() == 3);
    for (const auto& mushroom : mushrooms) {
        const bool over_pit = mushroom.x > 64.0F && mushroom.x < 192.0F;
        CHECK_FALSE(over_pit);
    }
}

TEST_CASE("background mushrooms are deterministic and animate through every frame") {
    const auto first = backdropFor(cave("room_02"));
    const auto again = backdropFor(cave("room_02"));
    REQUIRE(first.size() == again.size());
    CHECK(first[0].x == again[0].x);
    CHECK(first[0].offset_ms == again[0].offset_ms);

    std::set<int> frames;
    for (std::uint64_t time = 0; time < 4000; time += 50) {
        const int frame = mushroomFrame(first[0], time);
        CHECK(frame >= 0);
        CHECK(frame < kMushroomFrames);
        frames.insert(frame);
    }
    CHECK(frames.size() == static_cast<std::size_t>(kMushroomFrames));
}
