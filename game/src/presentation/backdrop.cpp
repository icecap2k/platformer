#include "presentation/backdrop.h"

#include <optional>
#include <string>

namespace presentation {

namespace {

constexpr int kSlices = 5;
constexpr std::uint64_t kFrameMs = 220;  // a slow pulse: about 1.8 s a loop
constexpr int kSunk = 2;                 // pixels the foot goes behind the floor

// A small deterministic mixer (splitmix64 finaliser).
std::uint64_t mix(std::uint64_t value) {
    value += 0x9E3779B97F4A7C15ULL;
    value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ULL;
    value = (value ^ (value >> 27)) * 0x94D049BB133111EBULL;
    return value ^ (value >> 31);
}

std::uint64_t seedOf(const std::string& room_id) {
    std::uint64_t hash = 1469598103934665603ULL;
    for (const char character : room_id) {
        hash = (hash ^ static_cast<unsigned char>(character)) * 1099511628211ULL;
    }
    return hash;
}

// The top row of the ground that rises from the bottom of the screen in `column`, if it is a floor
// (it starts in the lowest third of the room).
std::optional<int> floorRow(const domain::TileMap& map, int column) {
    int row = map.height();
    while (row > 0 && map.isGround(column, row - 1)) {
        --row;
    }
    if (row == map.height() || row < map.height() * 2 / 3) {
        return std::nullopt;
    }
    return row;
}

}  // namespace

std::vector<BackdropMushroom> backdropFor(const domain::Room& room) {
    const domain::TileMap& map = room.map();
    const std::uint64_t seed = seedOf(room.id());
    const int slice = map.width() / kSlices;

    std::vector<BackdropMushroom> mushrooms;
    for (int index = 0; index < kSlices; ++index) {
        const std::uint64_t h = mix(seed ^ mix(static_cast<std::uint64_t>(index) + 1));
        // Any column of the slice, starting from a random one, until one has a floor.
        for (int step = 0; step < slice; ++step) {
            const int column = index * slice + static_cast<int>((h + static_cast<std::uint64_t>(step)) %
                                                                static_cast<std::uint64_t>(slice));
            const auto row = floorRow(map, column);
            if (!row) {
                continue;
            }
            const auto type = static_cast<MushroomType>((h >> 8) % kMushroomTypes);
            const float x = (static_cast<float>(column) + 0.5F) * static_cast<float>(map.tileSize());
            mushrooms.push_back(BackdropMushroom{type, x,
                                                 static_cast<float>(*row * map.tileSize() + kSunk),
                                                 ((h >> 16) & 1U) != 0, (h >> 24) % (kFrameMs * kMushroomFrames)});
            break;
        }
    }
    return mushrooms;
}

int mushroomFrame(const BackdropMushroom& mushroom, std::uint64_t time_ms) {
    return static_cast<int>(((time_ms + mushroom.offset_ms) / kFrameMs) % kMushroomFrames);
}

}  // namespace presentation
