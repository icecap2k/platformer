#include "presentation/firefly.h"

#include <algorithm>
#include <cmath>

namespace presentation {

namespace {

constexpr std::uint64_t kFlapMs = 90;
constexpr std::uint64_t kGlowMs = 700;
constexpr int kAttemptsPerFirefly = 40;
// The bob repeats every 1.2 s, which divides the 10.8 s cycle exactly: no jump when the cycle restarts.
constexpr float kBobRadiansPerMs = 6.2831853F / 1200.0F;

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

// Is the whole flight box (leg plus the tile the firefly occupies) inside the room and free of ground?
bool flightIsFree(const domain::TileMap& map, int column, int row, int direction) {
    const int first_column = std::min(column, column + direction * kFireflyLegTilesAcross);
    const int last_column = std::max(column, column + direction * kFireflyLegTilesAcross);
    const int last_row = row + kFireflyLegTilesDown;
    if (first_column < 0 || last_column >= map.width() || row < 0 || last_row >= map.height()) {
        return false;
    }
    for (int r = row; r <= last_row; ++r) {
        for (int c = first_column; c <= last_column; ++c) {
            if (map.isGround(c, r)) {
                return false;
            }
        }
    }
    return true;
}

}  // namespace

std::vector<FireflyRoute> fireflyRoutes(const std::string& room_id, const domain::TileMap& map) {
    std::vector<FireflyRoute> routes;
    const std::uint64_t seed = seedOf(room_id);
    for (int index = 0; index < kFirefliesPerRoom; ++index) {
        for (int attempt = 0; attempt < kAttemptsPerFirefly; ++attempt) {
            const std::uint64_t h = mix(seed ^ mix(static_cast<std::uint64_t>(index) * 1000 + attempt));
            FireflyRoute route;
            route.tile_size = map.tileSize();
            route.column = static_cast<int>(h % static_cast<std::uint64_t>(map.width()));
            route.row = static_cast<int>((h >> 16) % static_cast<std::uint64_t>(map.height()));
            route.direction = ((h >> 32) & 1U) != 0 ? 1 : -1;
            // Spread the starts over the whole cycle; index also separates them, so two
            // fireflies never begin within a second of each other.
            route.offset_ms = (static_cast<std::uint64_t>(index) * kFireflyCycleMs / kFirefliesPerRoom +
                               (h >> 40) % 1500) % kFireflyCycleMs;
            if (flightIsFree(map, route.column, route.row, route.direction)) {
                routes.push_back(route);
                break;
            }
        }
    }
    return routes;
}

Firefly fireflyAt(const FireflyRoute& route, std::uint64_t time_ms) {
    const float size = static_cast<float>(route.tile_size);
    const std::uint64_t t = (time_ms + route.offset_ms) % kFireflyCycleMs;
    const float across = static_cast<float>(kFireflyLegTilesAcross) * size * static_cast<float>(route.direction);
    const float down = static_cast<float>(kFireflyLegTilesDown) * size;

    // `along`: how far along the leg (0 = start point, 1 = far end); `direction`: where it heads.
    float along = 0.0F;
    int heading = route.direction;
    bool hovering = true;
    const std::uint64_t first_leg = kFireflyHoverMs;
    const std::uint64_t second_hover = kFireflyHoverMs + kFireflyLegMs;
    const std::uint64_t second_leg = second_hover + kFireflyHoverMs;
    if (t < first_leg) {
        along = 0.0F;  // hovering at the start, already turned the way it will go
    } else if (t < second_hover) {
        hovering = false;
        along = static_cast<float>(t - first_leg) / static_cast<float>(kFireflyLegMs);
    } else if (t < second_leg) {
        along = 1.0F;  // hovering at the far end, turned to come back
        heading = -route.direction;
    } else {
        hovering = false;
        heading = -route.direction;
        along = 1.0F - static_cast<float>(t - second_leg) / static_cast<float>(kFireflyLegMs);
    }

    // Sideways with a gentle ease at both ends; down with a curve that starts flat and ends steep.
    const float smooth = along * along * (3.0F - 2.0F * along);
    Firefly firefly;
    firefly.x = static_cast<float>(route.column) * size + across * smooth;
    firefly.y = static_cast<float>(route.row) * size + down * along * along;
    // A faint bob while it flaps in place; it dies out as the leg starts or ends, so there is no jump.
    const float hover_weight = std::clamp(1.0F - std::min(along, 1.0F - along) * 8.0F, 0.0F, 1.0F);
    firefly.y += std::sin(static_cast<float>(t) * kBobRadiansPerMs) * hover_weight;
    firefly.facing_left = heading < 0;
    firefly.hovering = hovering;
    const int flap = static_cast<int>(((time_ms + route.offset_ms) / kFlapMs) % 2);
    const int dim = static_cast<int>(((time_ms + route.offset_ms) / kGlowMs) % 2);
    firefly.frame = flap + 2 * dim;
    return firefly;
}

}  // namespace presentation
