#pragma once

namespace presentation {

// Purely cosmetic scenery. Decorations never affect the game: they are chosen
// by a deterministic hash of the tile position, so a room always looks the
// same and nothing needs to be stored in the level files.

inline constexpr int kNoDecoration = -1;

// Frames in assets/sprites/decor.png (8x8 each, left to right).
inline constexpr int kDecorFirstStanding = 0;  // tufts, flowers, mushroom, bush
inline constexpr int kDecorLastStanding = 5;
inline constexpr int kDecorFirstHanging = 6;  // vines
inline constexpr int kDecorLastHanging = 7;

// What grows on top of the solid tile at (column, row), or kNoDecoration.
int standingDecoration(int column, int row);

// What hangs from underneath the solid tile at (column, row), or kNoDecoration.
int hangingDecoration(int column, int row);

}  // namespace presentation
