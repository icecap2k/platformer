#include "presentation/autotile.h"

#include <cstdint>

namespace presentation {

int exposureMask(bool north, bool south, bool east, bool west) {
    return (north ? kExposedNorth : 0) | (south ? kExposedSouth : 0) | (east ? kExposedEast : 0) |
           (west ? kExposedWest : 0);
}

int slopeMask(bool south_exposed, bool high_side_exposed) {
    return (south_exposed ? 1 : 0) | (high_side_exposed ? 2 : 0);
}

int rockSheetRow(int tile_id, int column, int row) {
    std::uint32_t h = static_cast<std::uint32_t>(column) * 2654435761U ^
                      static_cast<std::uint32_t>(row) * 40503U;
    h ^= h >> 15;
    h *= 0x2C1B3C6DU;
    h ^= h >> 12;
    const int variant = static_cast<int>(h % kRockVariants);
    const int material = (tile_id == 1) ? 0 : 1;
    return material * kRockVariants + variant;
}

}  // namespace presentation
