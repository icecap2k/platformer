#!/usr/bin/env python3
"""Draws the cave exit sprite of game/assets/sprites/exit.png (standard library only).

    python3 game/tools/art/make_exit.py
    python3 game/tools/art/make_exit.py --preview preview.png   # 10x enlarged, does not write the sheet

Two 16x16 frames: a rough stone archway with daylight pouring in; the light brightens and dims
between the frames. The renderer centres it on the exit's hitbox and sits it on the floor.
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from make_player_sprites import write_png  # noqa: E402

OUT = Path(__file__).resolve().parents[2] / "assets" / "sprites" / "exit.png"
OUTLINE = (28, 20, 32, 255)
STONE = (120, 116, 140, 255)
STONE_DARK = (84, 80, 108, 255)

ARCH = [
    "....OOOOOOOO....",
    "..OOsssssssOOO..",
    ".OssdLLLLLLdssO.",
    ".OsdLLLLLLLLdsO.",
    "OsdLLLLLLLLLLdsO",
    "OsdLLLLLLLLLLdsO",
    "OsdLLLLLLLLLLdsO",
    "OsdLLLLLLLLLLdsO",
    "OsdLLLLLLLLLLdsO",
    "OsdLLLLLLLLLLdsO",
    "OsdLLLLLLLLLLdsO",
    "OsdLLLLLLLLLLdsO",
    "OsdLLLLLLLLLLdsO",
    "OsdLLLLLLLLLLdsO",
    "OsddLLLLLLLLddsO",
    "OOOOOOOOOOOOOOOO",
]


def frame(bright):
    light = (255, 250, 200, 255) if bright else (236, 232, 170, 255)
    light_edge = (255, 232, 140, 255) if bright else (222, 206, 120, 255)
    rows = [[(0, 0, 0, 0)] * 16 for _ in range(16)]
    for y, art_row in enumerate(ARCH):
        assert len(art_row) == 16, f"row {y} must have 16 characters"
        for x, char in enumerate(art_row):
            if char == ".":
                continue
            colour = {"O": OUTLINE, "s": STONE, "d": STONE_DARK, "L": light}[char]
            rows[y][x] = colour
    # a softer edge on the light, and a few brighter "rays" near the middle
    for y in range(2, 15):
        for x in (3, 12):
            if rows[y][x] == light:
                rows[y][x] = light_edge
    if bright:
        for y in range(6, 14):
            rows[y][7] = (255, 255, 235, 255)
            rows[y][8] = (255, 255, 235, 255)
    return rows


def render():
    sheet = [[(0, 0, 0, 0)] * 32 for _ in range(16)]
    for index, bright in enumerate([False, True]):
        for y, row in enumerate(frame(bright)):
            for x, colour in enumerate(row):
                sheet[y][index * 16 + x] = colour
    return sheet


def write_preview(path, sheet, scale=10):
    background = (30, 28, 56, 255)
    rows = []
    for row in sheet:
        wide = []
        for px in row:
            wide.extend([px if px[3] else background] * scale)
        rows.extend([wide] * scale)
    write_png(path, rows)


if __name__ == "__main__":
    if "--preview" in sys.argv:
        write_preview(Path(sys.argv[sys.argv.index("--preview") + 1]), render())
    else:
        write_png(OUT, render())
        print(f"wrote {OUT}")
