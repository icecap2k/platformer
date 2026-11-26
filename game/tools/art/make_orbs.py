#!/usr/bin/env python3
"""Draws the three orbs of game/assets/sprites/orbs.png (standard library only).

    python3 game/tools/art/make_orbs.py
    python3 game/tools/art/make_orbs.py --preview preview.png   # 8x enlarged, does not write the sheet

A grid of 26x26 cells: one row per `domain::OrbColour` (red-orange, green, yellow) and 13
columns. Columns 0-11 are the animation, a loop of `presentation::kOrbFrames`: the glass sphere
swells and shrinks once while the bubbles inside rise to the top. Column 12 is the dashed
silhouette that stays where an orb has been taken. Below the rows, at y = 78, six 11x11 icons for
the HUD: the three taken orbs and then the three dashed ones.

The sphere is shaded from the normal of a real ball lit from the top left (so it reads as round),
with a sharp glint, a faint reflected light along the bottom right rim and a soft glow around it.
"""

import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from make_sprites import write_png  # noqa: E402

OUT = Path(__file__).resolve().parents[2] / "assets" / "sprites" / "orbs.png"

CELL = 26
FRAMES = 12
CENTRE = CELL / 2
RADIUS, PULSE = 9.0, 0.8  # the radius goes from RADIUS - PULSE to RADIUS + PULSE

# Per colour: outline, then the shades from darkest to lightest, and the glow around it.
COLOURS = [
    {"outline": (74, 16, 22), "shades": [(140, 28, 30), (196, 46, 32), (236, 82, 32), (252, 130, 44), (255, 182, 86)],
     "glow": (255, 110, 50)},
    {"outline": (10, 54, 40), "shades": [(18, 96, 60), (26, 140, 72), (48, 188, 84), (104, 224, 100), (180, 248, 138)],
     "glow": (90, 240, 130)},
    {"outline": (92, 58, 8), "shades": [(160, 104, 10), (206, 146, 12), (242, 190, 24), (255, 222, 60), (255, 244, 140)],
     "glow": (255, 220, 80)},
]
LIGHT = (-0.52, -0.62, 0.59)

# Bubbles: '#' is the bright wall, 'o' a glint, '.' the barely lighter inside.
BIG = [".###.", "#oo.#", "#o..#", "#...#", ".###."]
MEDIUM = [".##.", "#o.#", "#..#", ".##."]
SMALL = ["o#", "##"]
TINY = ["#"]
STRENGTH = {"#": 0.80, "o": 1.0, ".": 0.16}
# Each: its shape, horizontal place (-1..1 of the radius), starting point of its climb and how
# many times it climbs in one loop.
BUBBLES = [(BIG, -0.30, 0.00, 1), (MEDIUM, 0.40, 0.45, 1), (SMALL, 0.06, 0.72, 2), (SMALL, -0.54, 0.30, 1),
           (TINY, 0.52, 0.90, 1)]

def blend(under, over, alpha):
    return tuple(round(u + (o - u) * alpha) for u, o in zip(under, over))


def inside(x, y, radius):
    return math.hypot(x + 0.5 - CENTRE, y + 0.5 - CENTRE) <= radius


def orb_frame(colour, frame):
    t = frame / FRAMES
    radius = RADIUS + PULSE * math.sin(2 * math.pi * t)
    cell = {}
    # Glow, outside the ball: it brightens as the ball swells.
    peak = 80 + 50 * math.sin(2 * math.pi * t)
    for y in range(CELL):
        for x in range(CELL):
            distance = math.hypot(x + 0.5 - CENTRE, y + 0.5 - CENTRE)
            if radius < distance < radius + 3.6:
                alpha = round(peak * (1 - (distance - radius) / 3.6))
                if alpha > 6:
                    cell[(x, y)] = colour["glow"] + (alpha,)
    # The ball: shade = how much the surface faces the light.
    shades = colour["shades"]
    for y in range(CELL):
        for x in range(CELL):
            if not inside(x, y, radius):
                continue
            nx, ny = (x + 0.5 - CENTRE) / radius, (y + 0.5 - CENTRE) / radius
            nz = math.sqrt(max(0.0, 1 - nx * nx - ny * ny))
            light = max(0.0, nx * LIGHT[0] + ny * LIGHT[1] + nz * LIGHT[2])
            level = light * (len(shades) - 1) * 1.04
            if (x + y) % 2 == 0:
                level += 0.22  # a light dither softens the bands
            else:
                level -= 0.22
            rgb = shades[max(0, min(len(shades) - 1, round(level)))]
            # Reflected light: a thin bright crescent along the bottom right rim.
            rim = math.hypot(nx, ny)
            if rim > 0.70 and (nx * 0.65 + ny * 0.76) > 0.62:
                rgb = blend(rgb, shades[3], 0.75)
            cell[(x, y)] = rgb + (255,)
    # Bubbles climbing and wobbling inside, clipped by the glass.
    for index, (shape, across, start, climbs) in enumerate(BUBBLES):
        rise = (start + t * climbs) % 1.0
        by = CENTRE + radius * (0.74 - 1.45 * rise)
        bx = CENTRE + radius * across + 0.7 * math.sin(2 * math.pi * (t * 2 + index * 0.27))
        fade = min(1.0, rise * 5, (1 - rise) * 5)  # they appear at the bottom and burst at the top
        left, top = round(bx - len(shape[0]) / 2), round(by - len(shape) / 2)
        for dy, row in enumerate(shape):
            for dx, char in enumerate(row):
                x, y = left + dx, top + dy
                inner = char == "." and row[:dx].strip(".") and row[dx:].strip(".")
                if (char != "." or inner) and inside(x, y, radius - 1.0):
                    colour_of = (255, 252, 232)
                    cell[(x, y)] = blend(cell[(x, y)][:3], colour_of, STRENGTH[char] * fade) + (255,)
    # The glass' own glint, on top of everything: this is what sells the round surface.
    gx, gy = CENTRE - radius * 0.48, CENTRE - radius * 0.52
    for y in range(CELL):
        for x in range(CELL):
            dx, dy = x + 0.5 - gx, y + 0.5 - gy
            # an ellipse tilted along the rim
            u, v = (dx - dy) * 0.707, (dx + dy) * 0.707
            if (u / 3.0) ** 2 + (v / 1.6) ** 2 <= 1 and inside(x, y, radius - 0.8):
                cell[(x, y)] = blend(cell[(x, y)][:3], (255, 255, 255), 0.9 if abs(u) < 1.7 else 0.6) + (255,)
    # Outline.
    for y in range(CELL):
        for x in range(CELL):
            if inside(x, y, radius) and any(not inside(x + dx, y + dy, radius)
                                            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                cell[(x, y)] = colour["outline"] + (255,)
    return cell


def silhouette(colour):
    """A dashed ring where the orb used to be."""
    cell = {}
    for y in range(CELL):
        for x in range(CELL):
            if inside(x, y, RADIUS) and any(not inside(x + dx, y + dy, RADIUS)
                                            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                angle = math.atan2(y + 0.5 - CENTRE, x + 0.5 - CENTRE) % (2 * math.pi)
                if int(angle / (2 * math.pi) * 16) % 2 == 0:  # 8 dashes, 8 gaps
                    cell[(x, y)] = colour["shades"][3] + (200,)
    return cell


ICON = 11


def icon(colour, taken):
    """A tiny orb for the HUD, drawn at its own size so that its dashes stay crisp."""
    cell, centre, radius = {}, ICON / 2, 5.0
    inner = lambda x, y: math.hypot(x + 0.5 - centre, y + 0.5 - centre) <= radius
    for y in range(ICON):
        for x in range(ICON):
            if not inner(x, y):
                continue
            edge = any(not inner(x + dx, y + dy) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
            if not taken:
                angle = math.atan2(y + 0.5 - centre, x + 0.5 - centre) % (2 * math.pi)
                if edge and int(angle / (2 * math.pi) * 8 + 0.5) % 2 == 0:
                    cell[(x, y)] = colour["shades"][3] + (255,)
            elif edge:
                cell[(x, y)] = colour["outline"] + (255,)
            else:
                nx, ny = (x + 0.5 - centre) / radius, (y + 0.5 - centre) / radius
                nz = math.sqrt(max(0.0, 1 - nx * nx - ny * ny))
                light = max(0.0, nx * LIGHT[0] + ny * LIGHT[1] + nz * LIGHT[2])
                cell[(x, y)] = colour["shades"][min(4, round(light * 4.2))] + (255,)
    if taken:
        for glint in ((3, 3), (4, 3), (3, 4)):
            cell[glint] = (255, 255, 255, 255)
    return cell


def render():
    columns = FRAMES + 1
    sheet = [[(0, 0, 0, 0)] * (CELL * columns) for _ in range(CELL * len(COLOURS) + ICON)]
    for row, colour in enumerate(COLOURS):
        cells = [orb_frame(colour, f) for f in range(FRAMES)] + [silhouette(colour)]
        for column, cell in enumerate(cells):
            for (x, y), rgba in cell.items():
                sheet[row * CELL + y][column * CELL + x] = rgba
    for index, (taken, colour) in enumerate((t, c) for t in (True, False) for c in COLOURS):
        for (x, y), rgba in icon(colour, taken).items():
            sheet[CELL * len(COLOURS) + y][index * ICON + x] = rgba
    return sheet


def write_preview(path, sheet, scale=8):
    background = (30, 28, 56)
    rows = []
    for row in sheet:
        wide = []
        for px in row:
            mixed = blend(background, px[:3], px[3] / 255) + (255,)
            wide.extend([mixed] * scale)
        rows.extend([wide] * scale)
    write_png(path, rows)


if __name__ == "__main__":
    if "--preview" in sys.argv:
        write_preview(Path(sys.argv[sys.argv.index("--preview") + 1]), render())
    else:
        write_png(OUT, render())
        print(f"wrote {OUT}")
