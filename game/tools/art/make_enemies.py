#!/usr/bin/env python3
"""Draws the enemy sprites of game/assets/sprites/enemies.png (standard library only).

    python3 game/tools/art/make_enemies.py
    python3 game/tools/art/make_enemies.py --preview preview.png   # 10x enlarged, does not write the sheet

Frames 0 and 1 (8x8 each): the cave beetle walker, facing RIGHT, legs in the two phases of
its walk. The renderer mirrors it to face left. Its hitbox is the whole 8x8 tile.
Frame 2: the beetle squashed flat on the floor (shown for 1 s after a stomp).
A light outline and a pale shell, so it stands out against the dark cave.
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from make_player_sprites import write_png  # noqa: E402

OUT = Path(__file__).resolve().parents[2] / "assets" / "sprites" / "enemies.png"
OUTLINE = (238, 228, 255, 255)  # pale lilac: it separates the beetle from the dark background
# Soft violet shell, to read as hostile next to the warm coins and fruits.
PALETTE = {"a": (196, 168, 244, 255), "b": (156, 124, 214, 255), "d": (112, 84, 172, 255),
           "e": (255, 226, 120, 255), "l": (80, 58, 128, 255)}

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
SQUASHED = [
    "........",
    "........",
    "........",
    "........",
    "........",
    ".OOOOOO.",
    "OabbbbdO",
    "OlOddOlO",
]
EYE = (6, 3)  # where the eye goes: the front (right) of the shell


def frame_pixels(art, eye=True):
    rows = [[(0, 0, 0, 0)] * 8 for _ in range(8)]
    for y, art_row in enumerate(art):
        assert len(art_row) == 8, f"row {y} must have 8 characters"
        for x, char in enumerate(art_row):
            if char != ".":
                rows[y][x] = OUTLINE if char == "O" else PALETTE[char]
    if eye:
        rows[EYE[1]][EYE[0]] = PALETTE["e"]
    return rows


def render():
    sheet = [[(0, 0, 0, 0)] * 24 for _ in range(8)]
    for index, art in enumerate((BEETLE_A, BEETLE_B, SQUASHED)):
        for y, row in enumerate(frame_pixels(art, eye=index < 2)):
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
