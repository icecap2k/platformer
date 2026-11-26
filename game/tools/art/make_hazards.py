#!/usr/bin/env python3
"""Draws the hazard sprites of game/assets/sprites/hazards.png (standard library only).

    python3 game/tools/art/make_hazards.py
    python3 game/tools/art/make_hazards.py --preview preview.png   # 10x enlarged, does not write the sheet

Frame 0 (8x8): a patch of floor spikes, sawtooth style (drawn programmatically). Frame 1 (8x8): the stalactite trap, paler than
the scenery's stalactites and cracked, so it reads as loose. The tips start 3 px below the top of the
frame, matching the hitbox of a `^` tile (see tools/level_tools/text_level.h):
the renderer repeats this frame along a hazard's rectangle, with its bottom
edge on the rectangle's bottom edge.
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from make_sprites import write_png  # noqa: E402

OUT = Path(__file__).resolve().parents[2] / "assets" / "sprites" / "hazards.png"
OUTLINE = (28, 20, 32, 255)

# Sawtooth spikes: one pointed tooth per tile (6 px tall, stepping out to 6 px
# wide), so a patch of several tiles makes a zig-zag. Row 7 is a dark line that
# merges with the floor's top outline: the renderer draws the sprite 1 px lower
# than the hazard rectangle's bottom edge, so the teeth look embedded in the floor.
SPIKE_ART = [
    "...OO...",
    "..OaaO..",
    "..OabO..",
    ".OaabdO.",
    ".OaabdO.",
    "OaabbddO",
    "OaabbddO",
    "OOOOOOOO",
]
SPIKE_PALETTE = {"a": (246, 248, 255, 255), "b": (188, 200, 232, 255), "d": (118, 130, 176, 255)}


def spikes_sprite():
    rows = [[(0, 0, 0, 0)] * 8 for _ in range(8)]
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
    sheet = [[(0, 0, 0, 0)] * 16 for _ in range(8)]
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
