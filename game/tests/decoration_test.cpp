#include <doctest/doctest.h>

#include <set>
#include <vector>

#include "infrastructure/assets/tiled_map_loader.h"
#include "level_tools/text_level.h"
#include "presentation/decoration.h"

using namespace presentation;

namespace {

// A 40 x 30 room: a floor, a ceiling, walls on both sides and a few platforms.
domain::Room cave(const std::string& id, const std::vector<std::string>& extra = {}) {
    std::string text = "---\n";
    for (int row = 0; row < 30; ++row) {
        std::string line(40, '.');
        if (row < 2 || row > 26) {
            line.assign(40, '#');
        }
        line[0] = line[39] = '#';
        if (row == 14) {
            line.replace(8, 24, 24, 'B');
        }
        for (std::size_t i = 0; i + 1 < extra.size(); i += 2) {
            if (std::stoi(extra[i]) == row) {
                line = extra[i + 1];
            }
        }
        text += line + "\n";
    }
    return level_tools::toRoom(id, level_tools::parseTextLevel(text));
}

bool overlap(const Prop& a, const Prop& b) {
    const PropShape sa = propShape(a.type);
    const PropShape sb = propShape(b.type);
    return a.column < b.column + sb.width && b.column < a.column + sa.width && a.row < b.row + sb.height &&
           b.row < a.row + sa.height;
}

}  // namespace

TEST_CASE("scenery is deterministic and differs between rooms") {
    const auto first = sceneryFor(cave("a"));
    const auto again = sceneryFor(cave("a"));
    const auto other = sceneryFor(cave("b"));
    REQUIRE(first.size() == again.size());
    bool different = first.size() != other.size();
    for (std::size_t i = 0; i < first.size(); ++i) {
        CHECK(first[i].type == again[i].type);
        CHECK(first[i].column == again[i].column);
        CHECK(first[i].row == again[i].row);
        different = different || first[i].column != other[i].column || first[i].type != other[i].type;
    }
    CHECK(different);
}

TEST_CASE("scenery fits the cave") {
    std::set<PropType> seen;
    for (const char* id : {"a", "b", "c", "d", "e", "f", "g", "h"}) {
        const domain::Room room = cave(id);
        const domain::TileMap& map = room.map();
        const auto props = sceneryFor(room);
        CHECK(props.size() > 8);
        for (std::size_t i = 0; i < props.size(); ++i) {
            const Prop& prop = props[i];
            const PropShape shape = propShape(prop.type);
            seen.insert(prop.type);
            INFO("room " << id << " prop " << static_cast<int>(prop.type) << " at " << prop.column << "," << prop.row);
            for (int r = prop.row; r < prop.row + shape.height; ++r) {
                for (int c = prop.column; c < prop.column + shape.width; ++c) {
                    CHECK_FALSE(map.isGround(c, r));
                }
            }
            if (shape.attach == Attach::Wall) {
                const int wall = prop.mirrored ? prop.column + shape.width : prop.column - 1;
                for (int r = prop.row; r < prop.row + shape.height; ++r) {
                    CHECK(map.isSolid(wall, r));
                }
            } else {
                const int support = shape.attach == Attach::Floor ? prop.row + shape.height : prop.row - 1;
                for (int c = prop.column; c < prop.column + shape.width; ++c) {
                    CHECK(map.isSolid(c, support));
                }
            }
            if (prop.type == PropType::StalactiteCluster) {
                CHECK_FALSE(prop.mirrored);
            }
            for (std::size_t j = i + 1; j < props.size(); ++j) {
                CHECK_FALSE(overlap(prop, props[j]));
            }
        }
    }
    CHECK(seen.size() == static_cast<std::size_t>(kPropTypeCount));  // every kind shows up somewhere
}

TEST_CASE("scenery keeps clear of what matters") {
    const std::string spiked = "#.....^^^^^^^^^^....C.....F.....K.....X#";
    for (const char* id : {"a", "b", "c", "d", "e", "f"}) {
        const domain::Room room = cave(id, {"26", spiked});
        const float size = static_cast<float>(room.map().tileSize());
        for (const Prop& prop : sceneryFor(room)) {
            const PropShape shape = propShape(prop.type);
            const float left = static_cast<float>(prop.column) * size;
            const float top = static_cast<float>(prop.row) * size;
            const float right = left + static_cast<float>(shape.width) * size;
            const float bottom = top + static_cast<float>(shape.height) * size;
            for (const auto& hazard : room.hazards()) {
                const auto& box = hazard.bounds;
                const bool touches = left < box.right() && box.x < right && top < box.bottom() && box.y < bottom;
                CHECK_FALSE(touches);
            }
            for (const auto& coin : room.coins()) {
                const auto box = coin.bounds();
                const bool touches = left < box.right() && box.x < right && top < box.bottom() && box.y < bottom;
                CHECK_FALSE(touches);
            }
            const bool under_hud = (left < 48.0F || right > 320.0F - 48.0F) && top < 24.0F;
            CHECK_FALSE(under_hud);
        }
    }
}
