#pragma once

namespace presentation {

// Purely cosmetic scenery. Decorations never affect the game: they are chosen
// by a deterministic hash of the tile position, so a room always looks the
// same and nothing needs to be stored in the level files.

inline constexpr int kNoDecoration = -1;

// Frames in assets/sprites/decor.png (8x8 each, left to right), drawn by
// tools/art/make_cave_decor.py.
inline constexpr int kDecorFirstStanding = 0;  // glowing mushrooms, crystals, pebbles
inline constexpr int kDecorLastStanding = 5;
inline constexpr int kDecorFirstHanging = 6;  // stalactite with a drip, glowing roots
inline constexpr int kDecorLastHanging = 7;
inline constexpr int kDecorStalactite = 6;  // the hanging frame that drips (drop frames: see drip.h, 8-12)

// What grows on top of the solid tile at (column, row), or kNoDecoration.
int standingDecoration(int column, int row);

// What hangs from underneath the solid tile at (column, row), or kNoDecoration.
int hangingDecoration(int column, int row);

}  // namespace presentation
