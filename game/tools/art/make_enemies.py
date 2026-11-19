#!/usr/bin/env python3
"""Draws the enemy sprites of game/assets/sprites/enemies.png (standard library only).

    python3 game/tools/art/make_enemies.py
    python3 game/tools/art/make_enemies.py --preview preview.png   # 10x enlarged, does not write the sheet

Frames 0 and 1 (8x8 each): the cave beetle walker, facing RIGHT, legs in the two phases of
its walk. The renderer mirrors it to face left. Its hitbox is the whole 8x8 tile.
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from make_player_sprites import write_png  # noqa: E402

OUT = Path(__file__).resolve().parents[2] / "assets" / "sprites" / "enemies.png"
OUTLINE = (28, 20, 32, 255)
# Cold violet shell, to read as hostile next to the warm coins and fruits.
PALETTE = {"a": (158, 120, 214, 255), "b": (114, 84, 176, 255), "d": (74, 54, 124, 255),
           "e": (255, 226, 120, 255), "l": (54, 40, 88, 255)}

BEETLE_A = [
    "........",
    "..OOOO..",
    ".OaabbO.",
    "OabbbbdO",
    "ObbbbddO",
    "OddddddO",
    ".Ol.lO.O",
    "........",
]
BEETLE_B = [
    "........",
    "..OOOO..",
    ".OaabbO.",
    "OabbbbdO",
    "ObbbbddO",
    "OddddddO",
    "..O.l.O.",
    ".O....O.",
]
EYE = (6, 3)  # where the eye goes: the front (right) of the shell


def frame_pixels(art):
    rows = [[(0, 0, 0, 0)] * 8 for _ in range(8)]
    for y, art_row in enumerate(art):
        assert len(art_row) == 8, f"row {y} must have 8 characters"
        for x, char in enumerate(art_row):
            if char != ".":
                rows[y][x] = OUTLINE if char == "O" else PALETTE[char]
    rows[EYE[1]][EYE[0]] = PALETTE["e"]
    return rows


def render():
    sheet = [[(0, 0, 0, 0)] * 16 for _ in range(8)]
    for index, art in enumerate((BEETLE_A, BEETLE_B)):
        for y, row in enumerate(frame_pixels(art)):
            for x, colour in enumerate(row):
                sheet[y][index * 8 + x] = colour
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
