#!/usr/bin/env python3
"""Draws the static cave background (standard library only).

    python3 game/tools/art/make_cave_background.py
    python3 game/tools/art/make_cave_background.py --preview preview.png   # 3x enlarged, does not write the image

Writes game/assets/background/cave.png (320x240). It is built from three layers
(far, middle, near) painted one over the other; they used to slide for a parallax
effect, but the background is now still, so only the composite is saved. The
layers keep a margin around the screen, which is cropped away in `composite`.

The drawing is deterministic (fixed random seeds), so regenerating gives the
same picture.
"""

import math
import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from make_sprites import write_png  # noqa: E402

VIEW_W, VIEW_H = 320, 240
VERTICAL_SHARE = 0.5
FACTORS = {"far": 0.05, "mid": 0.12, "near": 0.22}  # only decide each layer's margin now
OUT_DIR = Path(__file__).resolve().parents[2] / "assets" / "background"

CLEAR = (0, 0, 0, 0)


def margins(factor):
    return (math.ceil(VIEW_W / 2 * factor), math.ceil(VIEW_H / 2 * factor * VERTICAL_SHARE))


class Layer:
    def __init__(self, name):
        self.name = name
        mx, my = margins(FACTORS[name])
        self.mx, self.my = mx, my
        self.w, self.h = VIEW_W + 2 * mx, VIEW_H + 2 * my
        self.px = [[CLEAR] * self.w for _ in range(self.h)]

    def put(self, x, y, color):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.px[y][x] = color

    def spike(self, cx, length, base_width, from_top, color, rim, rng):
        """A stalactite (hanging from the top) or stalagmite (rising from the bottom)."""
        wobble = 0.0
        for d in range(length):
            t = d / length
            width = max(1.0, base_width * (1.0 - t) ** 0.85)
            wobble += rng.choice((-0.35, 0.0, 0.0, 0.35))
            wobble = max(-1.5, min(1.5, wobble))
            x0 = round(cx - width / 2 + wobble)
            x1 = max(x0, round(cx + width / 2 + wobble) - 1)
            y = d if from_top else self.h - 1 - d
            for x in range(x0, x1 + 1):
                self.put(x, y, rim if x == x0 else color)

    def crystals(self, x, y, color, glow, rng):
        """A tiny cluster of glowing crystals standing on (x, y)."""
        for dx, height in ((-2, 3), (0, 5), (2, 3)):
            height += rng.choice((0, 1))
            for h in range(height):
                self.put(x + dx, y - h, color if h < height - 1 else glow)
            self.put(x + dx + 1, y - 1, color)


# A 4x4 ordered-dither matrix: it breaks the steps of a dark gradient into a fine grain.
BAYER = ((0, 8, 2, 10), (12, 4, 14, 6), (3, 11, 1, 9), (15, 7, 13, 5))


def fill_gradient(layer, stops):
    """A vertical gradient through `stops`, pairs of (height 0..1, colour), from top to bottom."""
    for y in range(layer.h):
        t = y / (layer.h - 1)
        for (t0, c0), (t1, c1) in zip(stops, stops[1:]):
            if t0 <= t <= t1:
                break
        k = (t - t0) / (t1 - t0)
        k = k * k * (3 - 2 * k)                      # eases in and out of every stop
        exact = [c0[i] + (c1[i] - c0[i]) * k for i in range(3)]
        for x in range(layer.w):
            threshold = (BAYER[y % 4][x % 4] + 0.5) / 16
            layer.px[y][x] = tuple(int(v) + (1 if v - int(v) > threshold else 0) for v in exact) + (255,)


def spread(rng, count, width, margin=6, jitter=0.35):
    """`count` positions spread over `width` with some irregularity."""
    step = (width - 2 * margin) / count
    return [round(margin + step * (i + 0.5) + rng.uniform(-jitter, jitter) * step) for i in range(count)]


def make_far():
    rng = random.Random(11)
    layer = Layer("far")
    # Almost black under the roof, a cold violet half way and a teal haze near the floor.
    fill_gradient(layer, ((0.0, (2, 2, 8)), (0.45, (8, 7, 22)), (0.78, (16, 15, 40)), (1.0, (18, 34, 52))))
    # Distant rock teeth, lit from behind: very low contrast so they read as far away.
    dark, rim = (4, 3, 12, 255), (11, 9, 27, 255)
    for cx in spread(rng, 11, layer.w):
        layer.spike(cx, rng.randint(28, 70), rng.randint(14, 26), True, dark, rim, rng)
    teeth, tooth_rim = (17, 18, 42, 255), (25, 28, 56, 255)
    for cx in spread(rng, 9, layer.w):
        layer.spike(cx, rng.randint(24, 56), rng.randint(16, 28), False, teeth, tooth_rim, rng)
    # Glow worms / distant crystals: single pixels with a faint halo.
    for _ in range(46):
        x, y = rng.randint(4, layer.w - 5), rng.randint(20, layer.h - 40)
        bright = rng.choice(((56, 110, 142, 255), (106, 68, 140, 255), (68, 86, 132, 255)))
        layer.put(x, y, bright)
        if rng.random() < 0.35:
            halo = (bright[0] // 3 + 6, bright[1] // 3 + 5, bright[2] // 3 + 16, 255)
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                layer.put(x + dx, y + dy, halo)
    return layer


def make_mid():
    rng = random.Random(23)
    layer = Layer("mid")
    color, rim = (15, 13, 36, 255), (30, 27, 62, 255)
    for cx in spread(rng, 8, layer.w, jitter=0.4):
        layer.spike(cx, rng.randint(34, 86), rng.randint(14, 24), True, color, rim, rng)
    for cx in spread(rng, 7, layer.w, jitter=0.4):
        layer.spike(cx, rng.randint(26, 60), rng.randint(14, 22), False, color, rim, rng)
    for _ in range(3):
        layer.crystals(rng.randint(30, layer.w - 30), layer.h - rng.randint(70, 150),
                       (30, 94, 112, 255), (98, 176, 190, 255), rng)
    return layer


def make_near():
    rng = random.Random(37)
    layer = Layer("near")
    color, rim = (8, 7, 20, 255), (26, 23, 54, 255)
    for cx in spread(rng, 5, layer.w, jitter=0.45):
        layer.spike(cx, rng.randint(40, 100), rng.randint(18, 30), True, color, rim, rng)
    for cx in spread(rng, 4, layer.w, jitter=0.45):
        layer.spike(cx, rng.randint(30, 64), rng.randint(18, 28), False, color, rim, rng)
    return layer


def composite(layers):
    """The screen at rest (all offsets zero): far, then mid, then near."""
    screen = [[(0, 0, 0, 255)] * VIEW_W for _ in range(VIEW_H)]
    for layer in layers:
        for y in range(VIEW_H):
            for x in range(VIEW_W):
                px = layer.px[y + layer.my][x + layer.mx]
                if px[3]:
                    screen[y][x] = px
    return screen


def scaled(pixels, scale):
    rows = []
    for row in pixels:
        wide = [px for px in row for _ in range(scale)]
        rows.extend([wide] * scale)
    return rows


if __name__ == "__main__":
    layers = [make_far(), make_mid(), make_near()]
    if "--preview" in sys.argv:
        write_png(Path(sys.argv[sys.argv.index("--preview") + 1]), scaled(composite(layers), 3))
    else:
        OUT_DIR.mkdir(parents=True, exist_ok=True)
        path = OUT_DIR / "cave.png"
        write_png(path, composite(layers))
        print(f"wrote {path} ({VIEW_W}x{VIEW_H})")
