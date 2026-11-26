#!/usr/bin/env python3
"""Draws the enemy sprites of game/assets/sprites/enemies.png (standard library only).

    python3 game/tools/art/make_enemies.py
    python3 game/tools/art/make_enemies.py --preview preview.png   # 10x enlarged, does not write the sheet

Five 16x16 frames of the cave beetle walker, facing RIGHT (the renderer mirrors it to face
left). Frames 0-3: its walk, a small domed body on long jointed legs. Frame 4: squashed flat on
the floor (shown for 1 s after a stomp). Its hitbox is the whole 16x16 frame. A light outline
and a pale shell, so it stands out against the dark cave.
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from make_sprites import write_png  # noqa: E402

OUT = Path(__file__).resolve().parents[2] / "assets" / "sprites" / "enemies.png"
OUTLINE = (238, 228, 255, 255)  # pale lilac: it separates the beetle from the dark background
# Soft violet shell, to read as hostile next to the warm coins and fruits.
PALETTE = {"a": (196, 168, 244, 255), "b": (156, 124, 214, 255), "d": (112, 84, 172, 255),
           "e": (255, 226, 120, 255), "E": (60, 30, 50, 255), "l": (222, 206, 250, 255),
           "x": (50, 30, 70, 255), "n": (150, 122, 196, 255)}

SIZE = 16
WALK_FRAMES = 4
GROUND = 15  # the row the feet stand on

# The body: a small dome. 'e' is the eye, a plain light square without a pupil.
BODY = [
    "....OOOOOO......",
    "..OOaaaabbOO....",
    ".OaaabObbbbbOO..",
    ".OaabbObbbbeeO..",
    "OabbbbObbbbeebO.",
    "OdddddOddddddbO.",
    ".OOOOOOOOOOOOO..",
]
BODY_TOP = 2

# Each leg: where it leaves the body (x), which way its knee sticks out, and its place in
# the step cycle. The far legs are darker and half a step out of time with the near ones.
LEGS = [  # hip x, knee side, phase, colour key (four legs read clearly at this size)
    (4, -1, 0, "l"),
    (11, 1, 2, "l"),
    (6, -1, 2, "n"),
    (9, 1, 0, "n"),
]
# Where a foot is in each quarter of its step: (sideways, lifted). It pushes back on the
# ground for three quarters and swings forward in the air for one.
STEP = [(2, 0), (0, 0), (-2, 0), (0, 2)]


def line(a, b):
    steps = max(abs(b[0] - a[0]), abs(b[1] - a[1]), 1)
    return [(round(a[0] + (b[0] - a[0]) * i / steps), round(a[1] + (b[1] - a[1]) * i / steps))
            for i in range(steps + 1)]


def put(rows, x, y, colour):
    if 0 <= x < SIZE and 0 <= y < SIZE:
        rows[y][x] = colour


def walk_frame(index):
    rows = [[(0, 0, 0, 0)] * SIZE for _ in range(SIZE)]
    bob = 1 if index % 2 else 0                      # the body dips as it steps
    body_bottom = BODY_TOP + len(BODY) - 1 + bob
    for hip, side, phase, key in sorted(LEGS, key=lambda leg: leg[3] != "l"):  # far legs first
        dx, lift = STEP[(index + phase) % WALK_FRAMES]
        top = (hip, body_bottom)
        foot = (hip + side + dx, GROUND - lift)
        knee = (hip + 2 * side + dx // 2, body_bottom + 2 - lift)
        for x, y in line(top, knee) + line(knee, foot):
            put(rows, x, y, PALETTE[key])
        put(rows, foot[0] + 1, foot[1], PALETTE[key])  # a little foot, pointing forward
    for y, art_row in enumerate(BODY):
        assert len(art_row) == SIZE, f"body row {y} must have {SIZE} characters"
        for x, char in enumerate(art_row):
            if char != ".":
                put(rows, x, BODY_TOP + bob + y, OUTLINE if char == "O" else PALETTE[char])
    return rows


SQUASHED = ["................"] * 11 + [
    "...OOOOOOOO.....",
    ".OOaaaabbbbOOO..",
    "OabbbbObbbxxbbO.",
    "OddddddddddddO..",
    "lOlOOl..lOOlOl..",
]


def squashed_frame():
    rows = [[(0, 0, 0, 0)] * SIZE for _ in range(SIZE)]
    assert len(SQUASHED) == SIZE
    for y, art_row in enumerate(SQUASHED):
        assert len(art_row) == SIZE, f"squashed row {y} must have {SIZE} characters"
        for x, char in enumerate(art_row):
            if char != ".":
                rows[y][x] = OUTLINE if char == "O" else PALETTE[char]
    return rows


def render():
    frames = [walk_frame(i) for i in range(WALK_FRAMES)] + [squashed_frame()]
    sheet = [[(0, 0, 0, 0)] * (SIZE * len(frames)) for _ in range(SIZE)]
    for index, frame in enumerate(frames):
        for y, row in enumerate(frame):
            for x, colour in enumerate(row):
                sheet[y][index * SIZE + x] = colour
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
