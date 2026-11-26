#!/usr/bin/env python3
"""Draws the cave exit sprite of game/assets/sprites/exit.png (standard library only).

    python3 game/tools/art/make_exit.py
    python3 game/tools/art/make_exit.py --preview preview.png   # 10x enlarged, does not write the sheet

A row of 22x32 frames of a rough stone archway. Frames 0 and 1: open, with daylight pouring in;
the light brightens and dims between them. Frame 2: closed (the orbs are still missing), a
portcullis with three small sockets, one per orb colour. Frames 3 onwards: the portcullis rising
into the arch, shown once when the exit opens (see presentation::exitSpriteFrame).
The renderer centres it on the exit's hitbox and sits it on the floor.
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


GATE_STEPS = 12                  # frames of the portcullis rising (presentation::kGateFrames)
GATE_DARK = (22, 18, 40, 255)
BAR, BAR_LIGHT, BAR_DARK = (92, 88, 118, 255), (150, 144, 176, 255), (48, 44, 70, 255)


def doorway():
    """The pixels of the opening: everything the arch art marks as light."""
    return {(x, y) for y, art_row in enumerate(ARCH) for x, char in enumerate(art_row) if char == "L"}


def portcullis():
    """The grate as it hangs when closed: three bars, two crossbars, points at the bottom and the
    three orb sockets. Returned as {(x, y): colour}; the gaps are not included."""
    opening = doorway()
    bottom = max(y for _, y in opening)
    grate = {}
    for x in (5, 10, 15):                                # bars, ending in a point
        for y in range(bottom + 1):
            grate[(x, y)] = BAR_LIGHT
            if y < bottom - 1:
                grate[(x + 1, y)] = BAR
    for y in (8, bottom - 5):                            # crossbars
        for x in range(WIDTH):
            grate[(x, y)] = BAR_LIGHT
            grate[(x, y + 1)] = BAR_DARK
    sockets = [(5, (236, 82, 32, 255)), (10, (48, 188, 84, 255)), (15, (242, 190, 24, 255))]
    for x, colour in sockets:                            # a stone plate with a gem on each bar
        for dy in range(-2, 3):
            for dx in range(-1, 3):
                edge = abs(dy) == 2 or dx in (-1, 2)
                grate[(x + dx, 16 + dy)] = OUTLINE if edge else colour
    return grate


def gate_frame(lift):
    """The arch with the portcullis raised `lift` pixels: dark when closed, daylight once it moves."""
    opening = doorway()
    behind = frame(False)
    rows = [row[:] for row in behind]
    if lift == 0:
        for x, y in opening:
            rows[y][x] = GATE_DARK
    grate = portcullis()
    for x, y in opening:                                 # what rises past the arch is hidden by it
        if (x, y + lift) in grate:
            rows[y][x] = grate[(x, y + lift)]
    return rows


def lifts():
    """How far the grate is up in each rising frame: it starts slowly and speeds up."""
    top = max(y for _, y in doorway())
    return [max(1, round((top + 1) * (step / (GATE_STEPS + 0.6)) ** 1.5)) for step in range(1, GATE_STEPS + 1)]


def render():
    frames = [frame(False), frame(True), gate_frame(0)] + [gate_frame(lift) for lift in lifts()]
    sheet = [[(0, 0, 0, 0)] * (len(frames) * WIDTH) for _ in range(HEIGHT)]
    for index, art in enumerate(frames):
        for y, row in enumerate(art):
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
