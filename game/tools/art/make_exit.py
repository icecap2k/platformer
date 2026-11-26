#!/usr/bin/env python3
"""Draws the cave exit sprite of game/assets/sprites/exit.png (standard library only).

    python3 game/tools/art/make_exit.py
    python3 game/tools/art/make_exit.py --preview preview.png   # 10x enlarged, does not write the sheet

Two 22x32 frames: a rough stone archway with daylight pouring in; the light brightens and dims
between the frames. The renderer centres it on the exit's hitbox and sits it on the floor.
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from make_sprites import write_png  # noqa: E402

OUT = Path(__file__).resolve().parents[2] / "assets" / "sprites" / "exit.png"
OUTLINE = (28, 20, 32, 255)
STONE = (120, 116, 140, 255)
STONE_DARK = (84, 80, 108, 255)

WIDTH, HEIGHT = 22, 32
ARCH_TOP = [
    "......OOOOOOOOOO......",
    "...OOOssssssssssOOO...",
    "..OssddLLLLLLLLddssO..",
    ".OsddLLLLLLLLLLLLddsO.",
]
ARCH_SIDE = "OsdLLLLLLLLLLLLLLLLdsO"
ARCH_BOTTOM = ["OsddLLLLLLLLLLLLLLddsO", "O" * WIDTH]
ARCH = ARCH_TOP + [ARCH_SIDE] * (HEIGHT - len(ARCH_TOP) - len(ARCH_BOTTOM)) + ARCH_BOTTOM
JOINTS = (9, 17, 25)  # rows where a seam between two blocks of the pillars shows


def frame(bright):
    light = (255, 250, 200, 255) if bright else (236, 232, 170, 255)
    light_edge = (255, 232, 140, 255) if bright else (222, 206, 120, 255)
    rows = [[(0, 0, 0, 0)] * WIDTH for _ in range(HEIGHT)]
    for y, art_row in enumerate(ARCH):
        assert len(art_row) == WIDTH, f"row {y} must have {WIDTH} characters"
        for x, char in enumerate(art_row):
            if char == ".":
                continue
            rows[y][x] = {"O": OUTLINE, "s": STONE, "d": STONE_DARK, "L": light}[char]
    for y in JOINTS:
        for x in (1, 2, WIDTH - 3, WIDTH - 2):
            rows[y][x] = STONE_DARK
    for y in range(2, HEIGHT - 1):                   # a softer edge on the light
        for x in (3, WIDTH - 4):
            if rows[y][x] == light:
                rows[y][x] = light_edge
    if bright:                                       # brighter rays near the middle
        for y in range(8, HEIGHT - 4):
            for x in range(WIDTH // 2 - 2, WIDTH // 2 + 2):
                rows[y][x] = (255, 255, 235, 255)
    return rows


def render():
    sheet = [[(0, 0, 0, 0)] * (2 * WIDTH) for _ in range(HEIGHT)]
    for index, bright in enumerate([False, True]):
        for y, row in enumerate(frame(bright)):
            for x, colour in enumerate(row):
                sheet[y][index * WIDTH + x] = colour
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
