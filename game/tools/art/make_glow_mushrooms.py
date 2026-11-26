#!/usr/bin/env python3
"""Draws the glowing mushrooms of the cave background, game/assets/sprites/glow_mushrooms.png
(standard library only).

    python3 game/tools/art/make_glow_mushrooms.py
    python3 game/tools/art/make_glow_mushrooms.py --preview preview.png   # 5x enlarged, does not write the sheet

A grid of 48x64 cells: one row per `presentation::MushroomType` (a tall one, a wide one and a
cluster) and 8 columns of animation (`presentation::kMushroomFrames`). In a loop the light swells
and fades, the cap breathes a pixel up and down, the gills and spots shine with it and a few spores
drift upwards. Each mushroom stands on the bottom edge of its cell, centred.

They live far away: no outline, dim bodies and soft light, so they never compete with what matters.
"""

import math
import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from make_sprites import write_png  # noqa: E402

OUT = Path(__file__).resolve().parents[2] / "assets" / "sprites" / "glow_mushrooms.png"
CELL_W, CELL_H = 48, 64
FRAMES = 8

# Per type: cap body (top, bottom), light of gills and spots, halo, stalk.
PALETTES = [
    {"cap": ((30, 116, 128), (16, 66, 90)), "light": (136, 250, 232), "halo": (60, 220, 210), "stalk": (46, 92, 112)},
    {"cap": ((98, 50, 136), (56, 28, 92)), "light": (226, 164, 255), "halo": (180, 100, 255), "stalk": (84, 66, 120)},
    {"cap": ((34, 76, 150), (20, 44, 98)), "light": (150, 204, 255), "halo": (80, 150, 255), "stalk": (50, 76, 122)},
]
# The mushrooms of each type: (centre x, stalk height, cap half width, cap height, lean).
TYPES = [
    [(24, 38, 15, 15, 3)],
    [(23, 20, 21, 13, -2)],
    [(15, 22, 10, 10, -3), (31, 13, 9, 8, 2), (40, 6, 5, 5, 1)],
]


def over(under, rgb, alpha):
    """Paints `rgb` with `alpha` (0..255) over an RGBA pixel."""
    a, b = alpha / 255, under[3] / 255
    out = a + b * (1 - a)
    if out <= 0:
        return (0, 0, 0, 0)
    return tuple(round((rgb[i] * a + under[i] * b * (1 - a)) / out) for i in range(3)) + (round(out * 255),)


def frame(kind, index):
    palette = PALETTES[kind]
    wave = math.sin(2 * math.pi * index / FRAMES)      # -1..1 over the loop
    shine = 0.5 + 0.5 * wave
    cell = [[(0, 0, 0, 0)] * CELL_W for _ in range(CELL_H)]

    def paint(x, y, rgb, alpha=255):
        if 0 <= x < CELL_W and 0 <= y < CELL_H:
            cell[y][x] = over(cell[y][x], rgb, alpha)

    mushrooms = TYPES[kind]
    caps = [(cx + lean, CELL_H - 1 - stalk - round(0.6 * wave), half, height)
            for cx, stalk, half, height, lean in mushrooms]
    # 1. The light around each cap and a pool of it at the foot.
    for (x0, y0, half, height), (cx, *_rest) in zip(caps, mushrooms):
        reach = min(x0, CELL_W - x0)                        # the glow must fade out inside the cell
        radius = min(reach, half * (1.45 + 0.2 * wave))
        for y in range(CELL_H):
            for x in range(CELL_W):
                distance = math.hypot((x + 0.5 - x0) / radius, (y + 0.5 - (y0 - height * 0.25)) / (radius * 0.9))
                if distance < 1:
                    paint(x, y, palette["halo"], round((34 + 46 * shine) * (1 - distance) ** 1.4))
                pool = math.hypot((x + 0.5 - cx) / min(half * 1.3, cx, CELL_W - cx), (y + 0.5 - CELL_H) / 7)
                if pool < 1:
                    paint(x, y, palette["halo"], round((40 + 50 * shine) * (1 - pool)))
    # 2. Stalks, curving up to their caps and lit from above.
    for (cx, stalk, half, height, lean), (x0, y0, _h, _t) in zip(mushrooms, caps):
        for y in range(y0, CELL_H):
            t = (CELL_H - 1 - y) / max(1, CELL_H - 1 - y0)      # 0 at the foot
            x = cx + lean * t * t
            wide = max(2, half // 4) + (1 if t < 0.12 else 0)
            for dx in range(-wide, wide):
                px = int(x + dx + 0.5)
                lit = max(0.0, t - 0.55) * (0.5 + 0.5 * shine)
                rgb = tuple(round(palette["stalk"][i] + (palette["light"][i] - palette["stalk"][i]) * lit)
                            for i in range(3))
                if dx >= wide - 1:
                    rgb = tuple(v * 2 // 3 for v in rgb)
                paint(px, y, rgb)
    # 3. Caps: a dome, darker underneath, with glowing gills along the rim and spots.
    rng = random.Random(100 + kind)
    for x0, y0, half, height in caps:
        for y in range(y0 - height, y0 + 4):
            for x in range(x0 - half, x0 + half + 1):
                u = (x + 0.5 - x0) / half
                v = (y + 0.5 - y0) / height
                if v <= 0 and u * u + v * v <= 1:
                    mix = min(1.0, max(0.0, 1 + v))             # 0 at the top, 1 at the rim
                    rgb = tuple(round(palette["cap"][0][i] + (palette["cap"][1][i] - palette["cap"][0][i]) * mix)
                                for i in range(3))
                    if u * u + v * v > 0.78 and v < -0.35:      # a thin lit edge on the dome
                        rgb = tuple(round(rgb[i] + (palette["light"][i] - rgb[i]) * 0.45) for i in range(3))
                    paint(x, y, rgb)
                elif 0 < v and u * u + (v * height / 3.5) ** 2 <= 1:
                    gill = (x - x0) % 3 == 0
                    paint(x, y, palette["light"], round((190 if gill else 90) + 50 * shine))
        for _ in range(max(3, half // 2)):                      # spots that shine with the loop
            sx = x0 + rng.randint(-half + 3, half - 3)
            sy = y0 - rng.randint(2, max(3, height - 3))
            big = rng.random() < 0.4
            for dx, dy in (((0, 0), (1, 0), (0, 1), (1, 1)) if big else ((0, 0),)):
                if ((sx + dx + 0.5 - x0) / half) ** 2 + ((sy + dy + 0.5 - y0) / height) ** 2 < 0.8:
                    paint(sx + dx, sy + dy, palette["light"], round(120 + 135 * shine))
    # 4. Spores rising from under the caps; they fade in and out over one loop.
    for n, (x0, y0, half, height) in enumerate(caps):
        for s in range(4 if half > 8 else 2):
            seed = random.Random(500 + kind * 40 + n * 10 + s)
            side = seed.choice((-1, 1))
            start = seed.random()
            rise = (start + index / FRAMES) % 1.0
            x = x0 + side * (half * seed.uniform(0.45, 1.1)) + 1.5 * math.sin(2 * math.pi * (rise + start))
            y = y0 + 2 - rise * (height + 16)
            paint(int(x), int(y), palette["light"], round(230 * math.sin(math.pi * rise)))
    return cell


def render():
    sheet = [[(0, 0, 0, 0)] * (CELL_W * FRAMES) for _ in range(CELL_H * len(TYPES))]
    for kind in range(len(TYPES)):
        for index in range(FRAMES):
            for y, row in enumerate(frame(kind, index)):
                for x, colour in enumerate(row):
                    sheet[kind * CELL_H + y][index * CELL_W + x] = colour
    return sheet


def write_preview(path, sheet, scale=5):
    background = (14, 13, 34)
    rows = []
    for row in sheet:
        wide = []
        for px in row:
            a = px[3] / 255
            wide.extend([tuple(round(px[i] * a + background[i] * (1 - a)) for i in range(3)) + (255,)] * scale)
        rows.extend([wide] * scale)
    write_png(path, rows)


if __name__ == "__main__":
    if "--preview" in sys.argv:
        write_preview(Path(sys.argv[sys.argv.index("--preview") + 1]), render())
    else:
        write_png(OUT, render())
        print(f"wrote {OUT}")
