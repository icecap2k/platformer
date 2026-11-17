#include "presentation/decoration.h"

#include <cstdint>

namespace presentation {

namespace {

// Small integer hash (a few multiply/xor-shift rounds) so neighbouring tiles
// get unrelated values.
std::uint32_t hashTile(int column, int row) {
    std::uint32_t h = static_cast<std::uint32_t>(column) * 73856093U ^
                      static_cast<std::uint32_t>(row) * 19349663U;
    h ^= h >> 13;
    h *= 0x5BD1E995U;
    h ^= h >> 15;
    return h;
}

}  // namespace

int standingDecoration(int column, int row) {
    const std::uint32_t slot = hashTile(column, row) % 12;
    // 6 of 12 slots are empty so the scenery stays sparse.
    const int kinds = kDecorLastStanding - kDecorFirstStanding + 1;
    if (slot >= static_cast<std::uint32_t>(kinds)) {
        return kNoDecoration;
    }
    return kDecorFirstStanding + static_cast<int>(slot);
}

int hangingDecoration(int column, int row) {
    const std::uint32_t h = hashTile(column + 1000, row);
    if (h % 4 != 0) {
        return kNoDecoration;
    }
    return kDecorFirstHanging + static_cast<int>((h >> 8) % 2);
}

}  // namespace presentation
