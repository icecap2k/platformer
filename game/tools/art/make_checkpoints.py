#!/usr/bin/env python3
"""Draws the checkpoint sprites of game/assets/sprites/checkpoints.png (standard library only).

    python3 game/tools/art/make_checkpoints.py
    python3 game/tools/art/make_checkpoints.py --preview preview.png   # 10x enlarged, does not write the sheet

Three 12x20 frames: 0 = a dark crystal lantern on a stone post (not reached), 1 and 2 = the same
lantern lit, with a teal flame whose halo pulses (the renderer alternates them). Teal, to be
different from the warm coins and fruits and the violet enemies. The renderer centres it on the
checkpoint's hitbox and stands it on the floor, so it is taller than the hitbox.
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from make_sprites import write_png  # noqa: E402

OUT = Path(__file__).resolve().parents[2] / "assets" / "sprites" / "checkpoints.png"
OUTLINE = (28, 20, 32, 255)
STONE = {"s": (132, 128, 148, 255), "t": (92, 88, 112, 255)}
GLASS_OFF = (58, 64, 84, 255)

WIDTH, HEIGHT = 12, 20
ART = [
    "............",
    ".....OO.....",
    "....OssO....",
    "..OOOOOOOO..",
    "..OsssttsO..",
    "...OggggO...",
    "..OggffggO..",
    "..OgffffgO..",
    "..OgffffgO..",
    "..OggffggO..",
    "...OggggO...",
    "..OsssttsO..",
    "...OOOOOO...",
    "....OstO....",
    "....OstO....",
    "....OstO....",
    "....OstO....",
    "...OsstsO...",
    "..OssttttO..",
    ".OOOOOOOOOO.",
]


def frame(lit, pulse):
    flame = (150, 255, 234, 255) if lit else GLASS_OFF
    glass = (60, 150, 148, 255) if lit else (46, 50, 68, 255)
    rows = [[(0, 0, 0, 0)] * WIDTH for _ in range(HEIGHT)]
    if lit:  # a round translucent halo behind the lantern
        radius, peak = (7.0, 130) if pulse else (6.2, 95)
        for y in range(HEIGHT):
            for x in range(WIDTH):
                distance = ((x + 0.5 - 6) ** 2 + (y + 0.5 - 8) ** 2) ** 0.5
                if distance < radius:
                    rows[y][x] = (110, 255, 225, round(peak * (1.0 - distance / radius) ** 0.7))
    for y, art_row in enumerate(ART):
        assert len(art_row) == WIDTH, f"row {y} must have {WIDTH} characters"
        for x, char in enumerate(art_row):
            if char == ".":
                continue
            if char == "O":
                rows[y][x] = OUTLINE
            elif char in "st":
                rows[y][x] = STONE[char]
            elif char == "f":
                rows[y][x] = flame
            else:  # g
                rows[y][x] = glass
    if lit and pulse:
        rows[7][5] = rows[8][5] = (230, 255, 248, 255)
    return rows


def render():
    sheet = [[(0, 0, 0, 0)] * (3 * WIDTH) for _ in range(HEIGHT)]
    for index, (lit, pulse) in enumerate([(False, False), (True, False), (True, True)]):
        for y, row in enumerate(frame(lit, pulse)):
            for x, colour in enumerate(row):
                sheet[y][index * WIDTH + x] = colour
    return sheet


def write_preview(path, sheet, scale=10):
    background = (30, 28, 56, 255)
    rows = []
    for row in sheet:
        wide = []
        for px in row:
            if px[3] == 0:
                colour = background
            elif px[3] < 255:
                a = px[3] / 255.0
                colour = tuple(int(c * a + b * (1 - a)) for c, b in zip(px[:3], background[:3])) + (255,)
            else:
                colour = px
            wide.extend([colour] * scale)
        rows.extend([wide] * scale)
    write_png(path, rows)


if __name__ == "__main__":
    if "--preview" in sys.argv:
        write_preview(Path(sys.argv[sys.argv.index("--preview") + 1]), render())
    else:
        write_png(OUT, render())
        print(f"wrote {OUT}")
