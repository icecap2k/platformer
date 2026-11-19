#!/usr/bin/env python3
"""Draws the firefly sprites of game/assets/sprites/fireflies.png (standard library only).

    python3 game/tools/art/make_fireflies.py
    python3 game/tools/art/make_fireflies.py --preview preview.png   # 10x enlarged, does not write the sheet

Four 8x8 frames, left to right: wings up / wings down, each with the glow bright and then dim
(frame = flap + 2 * dim). The body and a soft translucent halo are the same in all of them; a
glow that is only a few opaque pixels is invisible on the dark cave (see ADR-0012), so it is a halo.
The firefly faces RIGHT; the renderer mirrors it.
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from make_player_sprites import write_png  # noqa: E402

OUT = Path(__file__).resolve().parents[2] / "assets" / "sprites" / "fireflies.png"
BODY = (214, 255, 120, 255)
TAIL = (255, 246, 150, 255)
DARK = (84, 96, 60, 255)
WING = (224, 240, 255, 170)


def halo(strength):
    return (190, 255, 110, strength)


def frame(wings_up, bright):
    rows = [[(0, 0, 0, 0)] * 8 for _ in range(8)]
    soft, edge = (70, 34) if bright else (40, 18)
    for y in range(1, 7):
        for x in range(1, 7):
            if (x - 3.5) ** 2 + (y - 3.5) ** 2 <= 9.0:
                rows[y][x] = halo(edge)
    for y in range(2, 6):
        for x in range(2, 6):
            if (x - 3.5) ** 2 + (y - 3.5) ** 2 <= 3.0:
                rows[y][x] = halo(soft)
    rows[4][3] = DARK  # head (facing right: the tail glows on the left)
    rows[4][4] = DARK
    rows[4][2] = TAIL if bright else BODY
    rows[3][3] = BODY
    rows[3][4] = BODY
    if wings_up:
        rows[2][3] = WING
        rows[1][3] = WING
        rows[2][4] = WING
    else:
        rows[5][3] = WING
        rows[5][4] = WING
        rows[6][3] = WING
    return rows


def render():
    sheet = [[(0, 0, 0, 0)] * 32 for _ in range(8)]
    for index, (wings_up, bright) in enumerate([(True, True), (False, True), (True, False), (False, False)]):
        for y, row in enumerate(frame(wings_up, bright)):
            for x, colour in enumerate(row):
                sheet[y][index * 8 + x] = colour
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
