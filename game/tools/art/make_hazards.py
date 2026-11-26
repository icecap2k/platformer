#!/usr/bin/env python3
"""Draws the hazard sprites of game/assets/sprites/hazards.png (standard library only).

    python3 game/tools/art/make_hazards.py
    python3 game/tools/art/make_hazards.py --preview preview.png   # 10x enlarged, does not write the sheet

At x = 0 (8x16): a patch of floor spikes. At x = 8 (8x8, top): the stalactite trap, paler than
the scenery's stalactites and cracked, so it reads as loose. A `^` tile's hitbox is 12 px tall (see
tools/level_tools/text_level.h), shorter than the points, so brushing the tips is forgiven: the
renderer repeats the spikes along a hazard's rectangle, aligned to its bottom edge.
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from make_sprites import write_png  # noqa: E402

OUT = Path(__file__).resolve().parents[2] / "assets" / "sprites" / "hazards.png"
OUTLINE = (28, 20, 32, 255)

# Tall spikes: in each tile one long stone point with a shorter one beside it, so a patch
# makes an irregular zig-zag. The lower half normally sits inside a one-tile trench in the
# floor (see ADR-0020). The renderer draws the sprite 1 px lower than the hazard rectangle's
# bottom edge, so the dark last row merges with the outline of the rock below.
SPIKE_ART = [
    "..O.....",
    ".OaO....",
    ".OaO....",
    ".OabO...",
    ".OabO...",
    "OaabO.O.",
    "OaabOOaO",
    "OaabdOaO",
    "OaabdOab",
    "OaabdOab",
    "OabbdOab",
    "OabbdabO",
    "OabbdabO",
    "ObbddbdO",
    "OdddddbO",
    "OOOOOOOO",
]
SPIKE_PALETTE = {"a": (246, 248, 255, 255), "b": (188, 200, 232, 255), "d": (118, 130, 176, 255)}
SPIKE_HEIGHT = 16


def spikes_sprite():
    rows = [[(0, 0, 0, 0)] * 8 for _ in range(SPIKE_HEIGHT)]
    assert len(SPIKE_ART) == SPIKE_HEIGHT
    for y, art_row in enumerate(SPIKE_ART):
        assert len(art_row) == 8, f"row {y} must have 8 characters"
        for x, char in enumerate(art_row):
            if char != ".":
                rows[y][x] = OUTLINE if char == "O" else SPIKE_PALETTE[char]
    return rows


TRAP_PALETTE = {"a": (222, 206, 170, 255), "b": (176, 158, 128, 255), "d": (120, 100, 84, 255),
                "c": (84, 60, 56, 255)}
TRAP = [
    "OOOOOOOO",
    "OabaabaO",
    "OabcabdO",
    ".ObcadO.",
    ".OacdbO.",
    "..ObcO..",
    "..OdbO..",
    "...OO...",
]


def render():
    sheet = [[(0, 0, 0, 0)] * 16 for _ in range(SPIKE_HEIGHT)]
    for y, row in enumerate(spikes_sprite()):
        for x, colour in enumerate(row):
            sheet[y][x] = colour
    for y, row in enumerate(TRAP):
        assert len(row) == 8, f"row {y} must have 8 characters"
        for x, char in enumerate(row):
            if char != ".":
                sheet[y][8 + x] = OUTLINE if char == "O" else TRAP_PALETTE[char]
    return sheet


def write_preview(path, sheet, scale=10):
    background = (70, 74, 120, 255)
    rows = []
    for row in sheet:
        wide = [px if px[3] else background for px in row for _ in range(scale)]
        rows.extend([wide] * scale)
    write_png(path, rows)


if __name__ == "__main__":
    if "--preview" in sys.argv:
        write_preview(Path(sys.argv[sys.argv.index("--preview") + 1]), render())
    else:
        write_png(OUT, render())
        print(f"wrote {OUT}")
