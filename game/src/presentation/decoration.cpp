#include "presentation/decoration.h"

#include <cstdint>

namespace presentation {

namespace {

// Small integer hash (a few multiply/xor-shift rounds) so neighbouring tiles
// get unrelated values.
std::uint32_t hashTile(int column, int row, std::uint32_t seed) {
    std::uint32_t h = static_cast<std::uint32_t>(column) * 73856093U ^
                      static_cast<std::uint32_t>(row) * 19349663U ^ seed;
    h ^= h >> 13;
    h *= 0x5BD1E995U;
    h ^= h >> 15;
    return h;
}

// FNV-1a, so that two rooms with the same floor do not grow the same plants.
std::uint32_t hashName(const std::string& name) {
    std::uint32_t h = 2166136261U;
    for (const char c : name) {
        h = (h ^ static_cast<unsigned char>(c)) * 16777619U;
    }
    return h;
}

constexpr float kHudWidth = 48.0F;  // coin count (top left), orbs (centre), hearts (right)
constexpr float kHudHeight = 24.0F;

domain::AABB grown(const domain::AABB& box, float sides, float above, float below) {
    return domain::AABB{box.x - sides, box.y - above, box.width + 2.0F * sides, box.height + above + below};
}

// The areas that must stay clear, so scenery never hides or imitates something that matters.
std::vector<domain::AABB> reservedAreas(const domain::Room& room) {
    const float screen = static_cast<float>(room.map().width() * room.map().tileSize());
    std::vector<domain::AABB> areas{domain::AABB{0.0F, 0.0F, kHudWidth, kHudHeight},
                                    domain::AABB{screen - kHudWidth, 0.0F, kHudWidth, kHudHeight},
                                    domain::AABB{(screen - kHudWidth) * 0.5F, 0.0F, kHudWidth, kHudHeight}};
    for (const auto& hazard : room.hazards()) {
        areas.push_back(grown(hazard.bounds, 8.0F, 16.0F, 0.0F));
    }
    for (const auto& trap : room.stalactites()) {
        areas.push_back(grown(trap.bounds, 24.0F, 0.0F, 64.0F));  // including the way down
    }
    for (const auto& coin : room.coins()) {
        areas.push_back(grown(coin.bounds(), 8.0F, 8.0F, 0.0F));
    }
    for (const auto& fruit : room.fruits()) {
        areas.push_back(grown(fruit.bounds(), 8.0F, 8.0F, 0.0F));
    }
    for (const auto& checkpoint : room.checkpoints()) {
        areas.push_back(grown(checkpoint.bounds(), 12.0F, 16.0F, 0.0F));
    }
    for (const auto& orb : room.orbs()) {
        areas.push_back(grown(orb.bounds(), 12.0F, 12.0F, 8.0F));
    }
    for (const auto& exit : room.exits()) {
        areas.push_back(grown(exit.bounds(), 16.0F, 32.0F, 0.0F));
    }
    return areas;
}

class Planter {
public:
    explicit Planter(const domain::Room& room)
        : map_(room.map()), reserved_(reservedAreas(room)), seed_(hashName(room.id())),
          taken_(static_cast<std::size_t>(map_.width() * map_.height()), false) {}

    // Goes over every tile and, `chance` % of the times, tries to grow one of `types` there (the
    // tile being the one touching the rock): a random one first, then the others until one fits.
    template <std::size_t N>
    void plant(const std::array<PropType, N>& types, std::uint32_t chance, std::uint32_t pass) {
        for (int row = 0; row < map_.height(); ++row) {
            for (int column = 0; column < map_.width(); ++column) {
                const std::uint32_t h = hashTile(column, row, seed_ + pass * 7919U);
                if (h % 100U >= chance) {
                    continue;
                }
                for (std::size_t i = 0; i < N; ++i) {
                    if (tryAt(types[((h >> 8) + i) % N], column, row, (h >> 20) % 2U == 1U)) {
                        break;
                    }
                }
            }
        }
    }

    std::vector<Prop> props() const { return props_; }

private:
    bool inside(int column, int row) const {
        return column >= 0 && column < map_.width() && row >= 0 && row < map_.height();
    }

    bool rock(int column, int row) const { return inside(column, row) && map_.isSolid(column, row); }

    bool tryAt(PropType type, int column, int row, bool coin_toss) {
        const PropShape shape = propShape(type);
        Prop prop{type, column, row, false};
        if (shape.attach == Attach::Floor) {
            prop.row = row - shape.height + 1;  // (column, row) is its bottom left tile
            prop.mirrored = coin_toss;
        } else if (shape.attach == Attach::Ceiling) {
            prop.mirrored = coin_toss && type != PropType::StalactiteCluster;  // the drop needs the tip in place
        } else if (rock(column - 1, row)) {
            prop.mirrored = false;
        } else if (rock(column + 1, row)) {
            prop.column = column - shape.width + 1;
            prop.mirrored = true;
        } else {
            return false;
        }
        if (!fits(prop, shape)) {
            return false;
        }
        for (int r = prop.row; r < prop.row + shape.height; ++r) {
            for (int c = prop.column; c < prop.column + shape.width; ++c) {
                taken_[static_cast<std::size_t>(r * map_.width() + c)] = true;
            }
        }
        props_.push_back(prop);
        return true;
    }

    bool fits(const Prop& prop, const PropShape& shape) const {
        const int right = prop.column + shape.width - 1;
        const int bottom = prop.row + shape.height - 1;
        if (!inside(prop.column, prop.row) || !inside(right, bottom)) {
            return false;
        }
        // Empty tiles, with a free column on each side (and a free row above and below on a
        // wall) so props do not pile up.
        const int gap = shape.attach == Attach::Wall ? 1 : 0;
        for (int r = prop.row - gap; r <= bottom + gap; ++r) {
            for (int c = prop.column - 1; c <= right + 1; ++c) {
                if (!inside(c, r)) {
                    continue;
                }
                if (taken_[static_cast<std::size_t>(r * map_.width() + c)]) {
                    return false;
                }
                if (c >= prop.column && c <= right && r >= prop.row && r <= bottom && map_.isGround(c, r)) {
                    return false;
                }
            }
        }
        // Rock all along the side it grows from.
        if (shape.attach == Attach::Wall) {
            const int wall = prop.mirrored ? right + 1 : prop.column - 1;
            for (int r = prop.row; r <= bottom; ++r) {
                if (!rock(wall, r)) {
                    return false;
                }
            }
        } else {
            const int support = shape.attach == Attach::Floor ? bottom + 1 : prop.row - 1;
            for (int c = prop.column; c <= right; ++c) {
                if (!rock(c, support)) {
                    return false;
                }
            }
        }
        const float size = static_cast<float>(map_.tileSize());
        const domain::AABB box{static_cast<float>(prop.column) * size, static_cast<float>(prop.row) * size,
                               static_cast<float>(shape.width) * size, static_cast<float>(shape.height) * size};
        for (const domain::AABB& area : reserved_) {
            if (box.x < area.right() && area.x < box.right() && box.y < area.bottom() && area.y < box.bottom()) {
                return false;
            }
        }
        return true;
    }

    const domain::TileMap& map_;
    std::vector<domain::AABB> reserved_;
    std::uint32_t seed_;
    std::vector<bool> taken_;
    std::vector<Prop> props_;
};

}  // namespace

PropShape propShape(PropType type) {
    switch (type) {
        case PropType::GiantMushroom:
        case PropType::CyanCrystals: return {3, 3, Attach::Floor};
        case PropType::PinkMushrooms:
        case PropType::VioletCrystals:
        case PropType::GlowPlant: return {2, 2, Attach::Floor};
        case PropType::Stalagmites: return {2, 3, Attach::Floor};
        case PropType::Pebbles: return {2, 1, Attach::Floor};
        case PropType::SmallMushroom: return {1, 1, Attach::Floor};
        case PropType::StalactiteCluster: return {2, 3, Attach::Ceiling};
        case PropType::Vines: return {2, 4, Attach::Ceiling};
        case PropType::Roots: return {3, 2, Attach::Ceiling};
        case PropType::HangingCrystals: return {2, 2, Attach::Ceiling};
        case PropType::ShelfMushrooms: return {2, 3, Attach::Wall};
        case PropType::WallCrystals: return {2, 2, Attach::Wall};
        case PropType::WallVine: return {1, 4, Attach::Wall};
        case PropType::WallStalks: return {1, 2, Attach::Wall};
    }
    return {1, 1, Attach::Floor};
}

std::vector<Prop> sceneryFor(const domain::Room& room) {
    constexpr std::array kFloor{PropType::GiantMushroom, PropType::PinkMushrooms, PropType::CyanCrystals,
                                PropType::VioletCrystals, PropType::Stalagmites, PropType::GlowPlant,
                                PropType::Pebbles, PropType::SmallMushroom};
    constexpr std::array kCeiling{PropType::StalactiteCluster, PropType::Vines, PropType::Roots,
                                  PropType::HangingCrystals};
    // The big ones are listed twice: they are tried first more often.
    constexpr std::array kWall{PropType::ShelfMushrooms, PropType::WallCrystals, PropType::WallVine,
                               PropType::ShelfMushrooms, PropType::WallCrystals, PropType::WallStalks};
    Planter planter(room);
    planter.plant(kFloor, 30, 1);
    planter.plant(kCeiling, 30, 2);
    planter.plant(kWall, 45, 3);
    return planter.props();
}

}  // namespace presentation
