#!/usr/bin/env python3
"""Draws the bat sprites of game/assets/sprites/bats.png (standard library only).

    python3 game/tools/art/make_bats.py
    python3 game/tools/art/make_bats.py --preview preview.png   # 10x enlarged, does not write the sheet

A grid of 22x16 cells, facing RIGHT (the renderer mirrors it). Row 0: calm, with pale eyes.
Row 1: attacking, with red eyes, so the player can tell it has noticed them. Columns 0-3 are the
wing beat (up, level, down, level) and column 4 the bat knocked out on the floor. The 16x12
hitbox is centred sideways in the cell, at its bottom.

Like the beetle, it has a pale outline so that it stands out against the dark cave.
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from make_sprites import write_png  # noqa: E402

OUT = Path(__file__).resolve().parents[2] / "assets" / "sprites" / "bats.png"
CELL_W, CELL_H = 22, 16
MIDDLE = CELL_W // 2  # the wing tips stop one pixel short of the edges, leaving room for the outline
FRAMES = 5
OUTLINE = (238, 228, 255, 255)
PALETTE = {"a": (176, 140, 232, 255), "b": (132, 98, 196, 255), "d": (92, 66, 148, 255),
           "w": (112, 82, 172, 255), "f": (255, 255, 255, 255)}
EYES = [(255, 232, 140, 255), (255, 60, 60, 255)]  # calm, attacking

# The right half of each frame, from the middle of the bat outwards (the left half mirrors it,
# except for the face). '.' empty, a/b/d fur, w wing membrane, e eye, f fang.
BODY = [  # rows 4..12 of the cell, 4 columns on each side of the middle
    "..a.",   # pointed ears
    ".aa.",
    "aaa.",
    "aeb.",
    "abbd",
    "bfbd",
    "bbd.",
    "dd..",
    "d...",
]
WINGS = {  # (row, columns from the middle that are membrane)
    "up": [(2, (8, 9)), (3, (6, 9)), (4, (5, 9)), (5, (4, 8)), (6, (4, 7)), (7, (4, 6)), (8, (4, 4))],
    "level": [(6, (8, 9)), (7, (5, 9)), (8, (4, 9)), (9, (4, 8)), (10, (4, 4)), (10, (6, 7)), (11, (4, 4))],
    "down": [(8, (4, 5)), (9, (4, 7)), (10, (4, 8)), (11, (5, 9)), (12, (6, 9)), (13, (8, 9))],
}
KNOCKED_OUT = [  # the whole cell width, bottom rows
    "....aaaa....",
    "..wwaeeaww..",
    ".wwwabbawww.",
    "wwwdbbbbdwww",
]


def outlined(shape):
    out = dict(shape)
    for (x, y) in shape:
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            n = (x + dx, y + dy)
            if n not in shape and 0 <= n[0] < CELL_W and 0 <= n[1] < CELL_H:
                out[n] = "O"
    return out


def flying(wings, lift):
    shape = {}
    for row, (first, last) in WINGS[wings]:
        for offset in range(first - 1, last + 1):
            for x in (MIDDLE + offset, MIDDLE - 1 - offset):
                key = "d" if offset in (first - 1, first) else "w"
                shape[(x, row + lift)] = key
    for dy, art in enumerate(BODY):
        for dx, key in enumerate(art):
            if key == ".":
                continue
            y = 4 + dy + lift
            shape[(MIDDLE + dx, y)] = key
            shape[(MIDDLE - 1 - dx, y)] = key
    return outlined(shape)


def knocked_out():
    shape = {}
    for dy, art in enumerate(KNOCKED_OUT):
        for dx, key in enumerate(art):
            if key != ".":
                shape[(MIDDLE - len(art) // 2 + dx, CELL_H - len(KNOCKED_OUT) - 1 + dy)] = key
    return outlined(shape)


def render():
    sheet = [[(0, 0, 0, 0)] * (CELL_W * FRAMES) for _ in range(CELL_H * len(EYES))]
    for row, eye in enumerate(EYES):
        frames = [flying("up", 0), flying("level", 0), flying("down", -1), flying("level", 0), knocked_out()]
        for column, frame in enumerate(frames):
            for (x, y), key in frame.items():
                colour = OUTLINE if key == "O" else (eye if key == "e" else PALETTE[key])
                if column == FRAMES - 1 and key == "e":
                    colour = PALETTE["d"]  # closed
                sheet[row * CELL_H + y][column * CELL_W + x] = colour
    return sheet


def write_preview(path, sheet, scale=10):
    background = (14, 13, 32, 255)
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
