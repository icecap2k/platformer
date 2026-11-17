#pragma once

namespace presentation {

// Rock tiles are drawn differently depending on which of their four sides touch
// open air ("exposed"): a platform's ends become pointed tips, its top is lit and
// its underside is jagged. The art is in assets/sprites/rocks.png, made by
// tools/art/make_rock_tiles.py: 16 columns (one per exposure mask) and 8 rows
// (4 texture variants of ground, then 4 of rock).
//
// Pure logic (no SDL), so it is unit tested.

// Bit values of the exposure mask. Keep in sync with the script.
inline constexpr int kExposedNorth = 1;
inline constexpr int kExposedSouth = 2;
inline constexpr int kExposedEast = 4;
inline constexpr int kExposedWest = 8;
inline constexpr int kExposureMasks = 16;

inline constexpr int kRockVariants = 4;
inline constexpr int kRockRows = 8;

// Which column of the sheet to use for a tile whose sides are exposed or not.
int exposureMask(bool north, bool south, bool east, bool west);

// Which row of the sheet: tile id 1 is ground (rows 0-3), any other solid id is
// rock (rows 4-7). The texture variant comes from a hash of the tile position,
// so a room always looks the same and long walls do not visibly repeat.
int rockSheetRow(int tile_id, int column, int row);

}  // namespace presentation
